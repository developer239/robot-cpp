#include "WinWindowBackend.h"

#include <Windows.h>
#include <d3d11_4.h>
#include <dwmapi.h>
#include <dxgi.h>
#include <shellscalingapi.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <format>
#include <mutex>
#include <utility>

#include "robot/win/VideoFrame.h"

namespace robot::win {
namespace {

namespace capture = winrt::Windows::Graphics::Capture;
namespace direct3d = winrt::Windows::Graphics::DirectX::Direct3D11;

constexpr auto kPixelFormat =
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
constexpr std::int32_t kPoolFrames = 2;
constexpr double kDefaultDpi = 96.0;

std::string toUtf8(const std::wstring_view text) {
  const auto size = WideCharToMultiByte(
      CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr,
      nullptr
  );
  std::string utf8(static_cast<std::size_t>(size), '\0');

  WideCharToMultiByte(
      CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), size,
      nullptr, nullptr
  );

  return utf8;
}

// HWNDs only use their low 32 bits, including in 64-bit processes.
HWND toHwnd(const std::uint32_t windowId) {
  return reinterpret_cast<HWND>(static_cast<std::uintptr_t>(windowId));
}

std::uint32_t toWindowId(const HWND hwnd) {
  return static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(hwnd));
}

// The windows a user would call application windows, leaving out tool windows
// and the ones Windows keeps cloaked on other virtual desktops.
bool isApplicationWindow(const HWND hwnd) {
  if (IsWindowVisible(hwnd) == 0 || GetWindow(hwnd, GW_OWNER) != nullptr) {
    return false;
  }

  if ((GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) != 0 ||
      GetWindowTextLengthW(hwnd) == 0) {
    return false;
  }

  DWORD cloaked = 0;
  DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));

  return cloaked == 0;
}

template <typename TVisit>
void forEachTopLevelWindow(TVisit visit) {
  EnumWindows(
      [](const HWND hwnd, const LPARAM visitor) -> BOOL {
        return (*reinterpret_cast<TVisit*>(visitor))(hwnd) ? TRUE : FALSE;
      },
      reinterpret_cast<LPARAM>(&visit)
  );
}

LogicalRect logicalFrame(const HWND hwnd) {
  RECT rect{};
  GetWindowRect(hwnd, &rect);

  const auto scale = static_cast<double>(GetDpiForWindow(hwnd)) / kDefaultDpi;

  return LogicalRect{
      .origin =
          {.x = static_cast<double>(rect.left) / scale,
           .y = static_cast<double>(rect.top) / scale},
      .size =
          {.width = static_cast<double>(rect.right - rect.left) / scale,
           .height = static_cast<double>(rect.bottom - rect.top) / scale},
  };
}

std::string applicationName(const DWORD processId) {
  const auto process =
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
  if (process == nullptr) return {};

  std::wstring path(MAX_PATH, L'\0');
  auto size = static_cast<DWORD>(path.size());
  const auto didRead = QueryFullProcessImageNameW(process, 0, path.data(), &size);

  CloseHandle(process);

  if (didRead == 0) return {};

  path.resize(size);

  return toUtf8(std::filesystem::path(path).stem().wstring());
}

// Staging textures the GPU copies frames into. A frame keeps one mapped until
// its last copy is gone, which can be after the stream has stopped, so frames
// share ownership of this.
class StagingPool final {
 public:
  explicit StagingPool(winrt::com_ptr<ID3D11Device> device)
      : device_(std::move(device)) {
    device_->GetImmediateContext(context_.put());

    // Frames unmap on whichever thread releases them last.
    context_.as<ID3D11Multithread>()->SetMultithreadProtected(TRUE);
  }

  // Copies the texture and maps the copy, waiting for the GPU to finish.
  std::shared_ptr<void> copyFrame(
      ID3D11Texture2D* source, const std::shared_ptr<StagingPool>& self
  ) {
    D3D11_TEXTURE2D_DESC description{};
    source->GetDesc(&description);

    auto staging = acquire(description);
    context_->CopyResource(staging.get(), source);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    winrt::check_hresult(context_->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));

    const auto buffer = new PixelBuffer{
        .pixels = static_cast<const std::uint8_t*>(mapped.pData),
        .width = static_cast<int>(description.Width),
        .height = static_cast<int>(description.Height),
        .bytesPerRow = static_cast<int>(mapped.RowPitch),
    };

    return std::shared_ptr<void>(buffer, [self, staging](void* released) {
      delete static_cast<PixelBuffer*>(released);
      self->context_->Unmap(staging.get(), 0);
      self->release(staging);
    });
  }

 private:
  winrt::com_ptr<ID3D11Texture2D> acquire(D3D11_TEXTURE2D_DESC description) {
    {
      const std::scoped_lock lock(mutex_);

      while (!free_.empty()) {
        auto candidate = std::move(free_.back());
        free_.pop_back();

        D3D11_TEXTURE2D_DESC existing{};
        candidate->GetDesc(&existing);

        if (existing.Width == description.Width &&
            existing.Height == description.Height) {
          return candidate;
        }
      }
    }

    description.Usage = D3D11_USAGE_STAGING;
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    description.MiscFlags = 0;

    winrt::com_ptr<ID3D11Texture2D> staging;
    winrt::check_hresult(
        device_->CreateTexture2D(&description, nullptr, staging.put())
    );

    return staging;
  }

  void release(const winrt::com_ptr<ID3D11Texture2D>& staging) {
    const std::scoped_lock lock(mutex_);

    free_.push_back(staging);
  }

  winrt::com_ptr<ID3D11Device> device_;
  winrt::com_ptr<ID3D11DeviceContext> context_;
  std::mutex mutex_;
  std::vector<winrt::com_ptr<ID3D11Texture2D>> free_;
};

class WinWindowStream final : public backend::IWindowStreamBackend {
 public:
  // Throws winrt::hresult_error, which the caller converts.
  WinWindowStream(
      const capture::GraphicsCaptureItem& item, const std::uint32_t windowId,
      const double scaleFactor, const WindowStreamOptions& options,
      VideoFrameCallback onFrame, StreamErrorCallback onError
  )
      : item_(item),
        windowId_(windowId),
        scaleFactor_(scaleFactor),
        minFrameInterval_(std::chrono::duration<double>(1.0 / options.maxFps)),
        onFrame_(std::move(onFrame)),
        onError_(std::move(onError)) {
    winrt::com_ptr<ID3D11Device> device;
    winrt::check_hresult(D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr
    ));

    winrt::com_ptr<::IInspectable> inspectable;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(
        device.as<IDXGIDevice>().get(), inspectable.put()
    ));

    staging_ = std::make_shared<StagingPool>(device);
    device_ = inspectable.as<direct3d::IDirect3DDevice>();
    size_ = item_.Size();

    // Free threaded, because the calling thread need not pump messages.
    pool_ = capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
        device_, kPixelFormat, kPoolFrames, size_
    );
    session_ = pool_.CreateCaptureSession(item_);
    session_.IsCursorCaptureEnabled(options.showsCursor);

    closedToken_ = item_.Closed([this](const auto&, const auto&) {
      fail("the capture target went away");
    });
    frameToken_ = pool_.FrameArrived([this](const auto& pool, const auto&) {
      handleFrame(pool);
    });

    isRunning_ = true;
    session_.StartCapture();
  }

  ~WinWindowStream() override { stop(); }

  void stop() override {
    const std::scoped_lock lock(mutex_);

    if (isStopped_) return;

    isStopped_ = true;
    isRunning_ = false;

    item_.Closed(closedToken_);
    pool_.FrameArrived(frameToken_);
    session_.Close();
    pool_.Close();
  }

  [[nodiscard]] bool isRunning() const override { return isRunning_; }

 private:
  // Holding the mutex through the callback is what lets stop() promise that no
  // callback is running once it returns.
  void handleFrame(const capture::Direct3D11CaptureFramePool& pool) {
    const std::scoped_lock lock(mutex_);

    if (isStopped_ || !isRunning_) return;

    try {
      const auto frame = pool.TryGetNextFrame();
      if (frame == nullptr) return;

      const auto size = frame.ContentSize();

      if (size.Width != size_.Width || size.Height != size_.Height) {
        size_ = size;
        pool.Recreate(device_, kPixelFormat, kPoolFrames, size_);
        return;
      }

      const auto now = std::chrono::steady_clock::now();

      if (now - lastFrameAt_ < minFrameInterval_) return;

      lastFrameAt_ = now;
      deliver(frame);
    } catch (const winrt::hresult_error& error) {
      reportFailure(toUtf8(error.message()));
    }
  }

  void deliver(const capture::Direct3D11CaptureFrame& frame) {
    winrt::com_ptr<ID3D11Texture2D> texture;
    winrt::check_hresult(
        frame.Surface()
            .as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>()
            ->GetInterface(IID_PPV_ARGS(texture.put()))
    );

    const auto width = static_cast<double>(size_.Width) / scaleFactor_;
    const auto height = static_cast<double>(size_.Height) / scaleFactor_;

    // Both clocks count the same performance counter from boot.
    const VideoFrameInfo info{
        .windowId = windowId_,
        .size = {.width = size_.Width, .height = size_.Height},
        .scaleFactor = scaleFactor_,
        .contentScale = 1.0,
        .contentRect = {.origin = {}, .size = {.width = width, .height = height}},
        .capturedAt = std::chrono::steady_clock::time_point(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                frame.SystemRelativeTime()
            )
        ),
    };

    onFrame_(VideoFrame(info, staging_->copyFrame(texture.get(), staging_)));
  }

  void fail(const std::string_view detail) {
    const std::scoped_lock lock(mutex_);

    if (!isStopped_ && isRunning_) reportFailure(detail);
  }

  void reportFailure(const std::string_view detail) {
    isRunning_ = false;
    onError_(Error::captureFailed(detail));
  }

  capture::GraphicsCaptureItem item_;
  std::uint32_t windowId_;
  double scaleFactor_;
  std::chrono::duration<double> minFrameInterval_;
  VideoFrameCallback onFrame_;
  StreamErrorCallback onError_;

  std::shared_ptr<StagingPool> staging_;
  direct3d::IDirect3DDevice device_{nullptr};
  capture::Direct3D11CaptureFramePool pool_{nullptr};
  capture::GraphicsCaptureSession session_{nullptr};
  winrt::Windows::Graphics::SizeInt32 size_{};
  winrt::event_token closedToken_{};
  winrt::event_token frameToken_{};
  std::chrono::steady_clock::time_point lastFrameAt_{};

  std::mutex mutex_;
  bool isStopped_ = false;
  std::atomic<bool> isRunning_ = false;
};

// WinRT needs the process in an apartment. This joins the multithreaded one
// whatever the calling thread already chose, so it cannot clash with SDL or
// anything else that initialized COM on that thread.
void ensureWinRtApartment() {
  static const auto cookie = [] {
    CO_MTA_USAGE_COOKIE usage{};
    winrt::check_hresult(CoIncrementMTAUsage(&usage));

    return usage;
  }();

  (void)cookie;
}

void excludeOwnWindowsFromCapture() {
  const auto ownProcess = GetCurrentProcessId();

  forEachTopLevelWindow([ownProcess](const HWND hwnd) {
    DWORD process = 0;
    GetWindowThreadProcessId(hwnd, &process);

    if (process == ownProcess &&
        SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE) == 0) {
      winrt::throw_last_error();
    }

    return true;
  });
}

HMONITOR monitorById(const std::uint32_t monitorId) {
  struct Search {
    std::uint32_t remaining;
    HMONITOR found;
  } search{.remaining = monitorId, .found = nullptr};

  // Monitor ids are their position in this same enumeration, counting from 1.
  EnumDisplayMonitors(
      nullptr, nullptr,
      [](const HMONITOR monitor, HDC, LPRECT, const LPARAM data) -> BOOL {
        auto& state = *reinterpret_cast<Search*>(data);
        state.remaining--;

        if (state.remaining != 0) return TRUE;

        state.found = monitor;

        return FALSE;
      },
      reinterpret_cast<LPARAM>(&search)
  );

  return search.found;
}

double monitorScale(const HMONITOR monitor) {
  UINT dpiX = 0;
  UINT dpiY = 0;
  winrt::check_hresult(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY));

  return static_cast<double>(dpiX) / kDefaultDpi;
}

}  // namespace

std::expected<std::vector<Window>, Error> WinWindowBackend::enumerateWindows() {
  std::vector<Window> windows;

  forEachTopLevelWindow([&windows](const HWND hwnd) {
    if (!isApplicationWindow(hwnd)) return true;

    std::wstring title(static_cast<std::size_t>(GetWindowTextLengthW(hwnd)) + 1, L'\0');
    title.resize(static_cast<std::size_t>(
        GetWindowTextW(hwnd, title.data(), static_cast<int>(title.size()))
    ));

    DWORD process = 0;
    GetWindowThreadProcessId(hwnd, &process);

    windows.push_back(Window{
        .id = toWindowId(hwnd),
        .ownerPid = static_cast<std::int32_t>(process),
        .appName = applicationName(process),
        .title = toUtf8(title),
        .frame = logicalFrame(hwnd),
    });

    return true;
  });

  return windows;
}

std::expected<LogicalRect, Error> WinWindowBackend::windowFrame(
    const std::uint32_t windowId
) {
  const auto hwnd = toHwnd(windowId);

  if (IsWindow(hwnd) == 0) return std::unexpected(Error::windowNotFound(windowId));

  return logicalFrame(hwnd);
}

std::expected<std::optional<std::uint32_t>, Error>
WinWindowBackend::topmostWindowId() {
  std::optional<std::uint32_t> topmost;

  // EnumWindows walks the windows front to back.
  forEachTopLevelWindow([&topmost](const HWND hwnd) {
    if (!isApplicationWindow(hwnd)) return true;

    topmost = toWindowId(hwnd);

    return false;
  });

  return topmost;
}

std::expected<void, Error> WinWindowBackend::activateApplicationForWindow(
    const std::uint32_t windowId
) {
  const auto hwnd = toHwnd(windowId);

  if (IsWindow(hwnd) == 0) return std::unexpected(Error::windowNotFound(windowId));

  if (SetForegroundWindow(hwnd) == 0) {
    return std::unexpected(Error::platformError("SetForegroundWindow"));
  }

  return {};
}

std::expected<std::unique_ptr<backend::IWindowStreamBackend>, Error>
WinWindowBackend::startStream(
    const std::uint32_t windowId, const WindowStreamOptions& options,
    VideoFrameCallback onFrame, StreamErrorCallback onError
) {
  const auto hwnd = toHwnd(windowId);

  if (IsWindow(hwnd) == 0) return std::unexpected(Error::windowNotFound(windowId));

  try {
    ensureWinRtApartment();

    const auto factory = winrt::get_activation_factory<
        capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    capture::GraphicsCaptureItem item{nullptr};
    winrt::check_hresult(factory->CreateForWindow(
        hwnd, winrt::guid_of<capture::GraphicsCaptureItem>(), winrt::put_abi(item)
    ));

    return std::make_unique<WinWindowStream>(
        item, windowId, static_cast<double>(GetDpiForWindow(hwnd)) / kDefaultDpi,
        options, std::move(onFrame), std::move(onError)
    );
  } catch (const winrt::hresult_error& error) {
    return std::unexpected(Error::captureFailed(toUtf8(error.message())));
  }
}

std::expected<std::unique_ptr<backend::IWindowStreamBackend>, Error>
WinWindowBackend::startMonitorStream(
    const std::uint32_t monitorId, const WindowStreamOptions& options,
    VideoFrameCallback onFrame, StreamErrorCallback onError
) {
  const auto monitor = monitorById(monitorId);

  if (monitor == nullptr) return std::unexpected(Error::monitorNotFound(monitorId));

  try {
    ensureWinRtApartment();

    if (options.excludesOwnWindows) excludeOwnWindowsFromCapture();

    const auto factory = winrt::get_activation_factory<
        capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    capture::GraphicsCaptureItem item{nullptr};
    winrt::check_hresult(factory->CreateForMonitor(
        monitor, winrt::guid_of<capture::GraphicsCaptureItem>(), winrt::put_abi(item)
    ));

    return std::make_unique<WinWindowStream>(
        item, 0, monitorScale(monitor), options, std::move(onFrame),
        std::move(onError)
    );
  } catch (const winrt::hresult_error& error) {
    return std::unexpected(Error::captureFailed(toUtf8(error.message())));
  }
}

}  // namespace robot::win
