#include "MacWindowBackend.h"

#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>

namespace robot::mac {

// Shared between the stream handle and the ObjC output object. The output
// keeps its own shared_ptr, so callbacks that race stop() (the sample queue is
// not drained synchronously) find a deactivated channel instead of dangling
// pointers, and the caller-supplied callbacks stay alive as long as any
// callback could still reference them.
struct StreamChannel {
  VideoFrameCallback onFrame;
  StreamErrorCallback onError;
  dispatch_queue_t queue = nil;  // sample handler queue; serializes onError too
  std::uint32_t windowId = 0;
  std::atomic<bool> active{false};
};

}  // namespace robot::mac

// Receives frames on the stream's sample handler queue. The handler only
// retains the pixel buffer and forwards it through the channel - consumers are
// documented to hand the frame off immediately.
@interface RobotWindowStreamOutput : NSObject <SCStreamOutput, SCStreamDelegate> {
 @public
  std::shared_ptr<robot::mac::StreamChannel> channel;
}
@end

@implementation RobotWindowStreamOutput

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type {
  const auto chan = self->channel;
  if (chan == nullptr || !chan->active.load()) return;
  if (type != SCStreamOutputTypeScreen) return;
  if (!CMSampleBufferIsValid(sampleBuffer)) return;

  // ScreenCaptureKit delivers idle/blank samples too; only complete frames
  // carry new content. The same attachment dictionary carries the scale and
  // content-rect metadata needed to map capture pixels back to window points.
  double scaleFactor = 0.0;
  double contentScale = 0.0;
  CGRect contentRect = CGRectZero;
  CFArrayRef attachments =
      CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
  if (attachments != nullptr && CFArrayGetCount(attachments) > 0) {
    const auto* attachment = static_cast<CFDictionaryRef>(
        CFArrayGetValueAtIndex(attachments, 0)
    );
    const auto* statusNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(
        attachment, (__bridge CFStringRef)SCStreamFrameInfoStatus
    ));
    if (statusNumber != nullptr) {
      std::int32_t status = 0;
      CFNumberGetValue(statusNumber, kCFNumberSInt32Type, &status);
      if (status != SCFrameStatusComplete) return;
    }
    const auto* scaleNumber = static_cast<CFNumberRef>(CFDictionaryGetValue(
        attachment, (__bridge CFStringRef)SCStreamFrameInfoScaleFactor
    ));
    if (scaleNumber != nullptr) {
      CFNumberGetValue(scaleNumber, kCFNumberFloat64Type, &scaleFactor);
    }
    const auto* contentScaleNumber =
        static_cast<CFNumberRef>(CFDictionaryGetValue(
            attachment, (__bridge CFStringRef)SCStreamFrameInfoContentScale
        ));
    if (contentScaleNumber != nullptr) {
      CFNumberGetValue(
          contentScaleNumber, kCFNumberFloat64Type, &contentScale
      );
    }
    const auto* contentRectDict =
        static_cast<CFDictionaryRef>(CFDictionaryGetValue(
            attachment, (__bridge CFStringRef)SCStreamFrameInfoContentRect
        ));
    if (contentRectDict != nullptr) {
      CGRectMakeWithDictionaryRepresentation(contentRectDict, &contentRect);
    }
  }

  CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (pixels == nullptr) return;

  // Stamp the frame with when it was presented, not when this callback ran -
  // a sample delayed inside ScreenCaptureKit must not look fresh to
  // freshness-gated consumers. Presentation time and the host clock share the
  // same time base, so the age subtraction is exact.
  auto capturedAt = std::chrono::steady_clock::now();
  const CMTime presentation =
      CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
  if (CMTIME_IS_VALID(presentation)) {
    const CMTime hostNow = CMClockGetTime(CMClockGetHostTimeClock());
    const double ageSeconds =
        CMTimeGetSeconds(CMTimeSubtract(hostNow, presentation));
    capturedAt -= std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(ageSeconds)
    );
  }

  CVPixelBufferRetain(pixels);
  std::shared_ptr<void> buffer(pixels, [](void* pointer) {
    CVPixelBufferRelease(static_cast<CVPixelBufferRef>(pointer));
  });

  chan->onFrame(robot::VideoFrame(
      robot::VideoFrameInfo{
          .windowId = chan->windowId,
          .size =
              {.width =
                   static_cast<std::int32_t>(CVPixelBufferGetWidth(pixels)),
               .height =
                   static_cast<std::int32_t>(CVPixelBufferGetHeight(pixels))},
          .scaleFactor = scaleFactor,
          .contentScale = contentScale,
          .contentRect =
              {.origin = {.x = contentRect.origin.x, .y = contentRect.origin.y},
               .size =
                   {.width = contentRect.size.width,
                    .height = contentRect.size.height}},
          .capturedAt = capturedAt,
      },
      std::move(buffer)
  ));
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error {
  const auto chan = self->channel;
  if (chan == nullptr || chan->queue == nil) return;
  // Delegate callbacks arrive on ScreenCaptureKit's own queue; hop onto the
  // sample handler queue so error delivery is serialized with frame delivery
  // and covered by stop()'s drain. Only report deaths the caller did not
  // initiate - stop() deactivates the channel first.
  NSString* message = error.localizedDescription;
  dispatch_async(chan->queue, ^{
    if (chan->active.exchange(false) && chan->onError != nullptr) {
      chan->onError(robot::Error::captureFailed(message.UTF8String));
    }
  });
}

@end

namespace robot::mac {

namespace {

// Borrowed CFDictionary values; the owning array keeps them alive.
std::int32_t intValue(CFDictionaryRef dict, CFStringRef key) {
  const auto* number =
      static_cast<CFNumberRef>(CFDictionaryGetValue(dict, key));
  if (number == nullptr) return 0;
  std::int32_t value = 0;
  CFNumberGetValue(number, kCFNumberSInt32Type, &value);
  return value;
}

std::expected<SCShareableContent*, Error> fetchShareableContent() {
  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  __block SCShareableContent* content = nil;
  __block NSError* error = nil;
  [SCShareableContent
      getShareableContentExcludingDesktopWindows:YES
                             onScreenWindowsOnly:YES
                               completionHandler:^(
                                   SCShareableContent* result, NSError* err
                               ) {
                                 content = result;
                                 error = err;
                                 dispatch_semaphore_signal(semaphore);
                               }];

  const auto deadline =
      dispatch_time(DISPATCH_TIME_NOW, 5 * static_cast<int64_t>(NSEC_PER_SEC));
  if (dispatch_semaphore_wait(semaphore, deadline) != 0) {
    return std::unexpected(
        Error::captureFailed("timed out enumerating shareable content")
    );
  }
  if (error != nil) {
    if (error.code == SCStreamErrorUserDeclined) {
      return std::unexpected(Error::permissionDenied(
          "Screen Recording is required for window enumeration and capture; "
          "grant it in System Settings > Privacy & Security > Screen Recording"
      ));
    }
    return std::unexpected(
        Error::captureFailed(error.localizedDescription.UTF8String)
    );
  }
  return content;
}

// Owns the stream, its ObjC output, and the delivery queue. stop() deactivates
// the channel, stops capture, then drains the delivery queue, upholding the
// WindowStream contract that no callback is executing once stop() returns.
class MacWindowStream final : public backend::IWindowStreamBackend {
 public:
  MacWindowStream(
      SCStream* stream, RobotWindowStreamOutput* output,
      dispatch_queue_t queue, std::shared_ptr<StreamChannel> channel
  )
      : stream_(stream),
        output_(output),
        queue_(queue),
        channel_(std::move(channel)) {}

  ~MacWindowStream() override { stop(); }

  void stop() override {
    if (stream_ == nil) return;
    channel_->active.store(false);

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    [stream_ stopCaptureWithCompletionHandler:^(NSError* err) {
      (void)err;  // already stopping; nothing to do differently on error
      dispatch_semaphore_signal(semaphore);
    }];
    const auto deadline = dispatch_time(
        DISPATCH_TIME_NOW, 2 * static_cast<int64_t>(NSEC_PER_SEC)
    );
    dispatch_semaphore_wait(semaphore, deadline);

    // Drain: callbacks run serially on queue_, so once this empty block has
    // run, no callback is executing and the deactivated channel stops any
    // new one from doing work.
    dispatch_sync(queue_, ^{
                  });

    stream_ = nil;
    output_ = nil;
    queue_ = nil;
  }

  [[nodiscard]] bool isRunning() const override {
    return channel_->active.load();
  }

 private:
  SCStream* stream_ = nil;
  RobotWindowStreamOutput* output_ = nil;
  dispatch_queue_t queue_ = nil;
  std::shared_ptr<StreamChannel> channel_;
};

}  // namespace

std::expected<std::vector<Window>, Error> MacWindowBackend::enumerateWindows() {
  auto contentResult = fetchShareableContent();
  if (!contentResult) return std::unexpected(contentResult.error());

  std::vector<Window> windows;
  for (SCWindow* window in (*contentResult).windows) {
    if (window.windowLayer != 0) continue;
    if (window.title == nil || window.title.length == 0) continue;
    windows.push_back(Window{
        .id = window.windowID,
        .ownerPid = window.owningApplication != nil
                        ? window.owningApplication.processID
                        : 0,
        .appName = window.owningApplication != nil
                       ? window.owningApplication.applicationName.UTF8String
                       : "",
        .title = window.title.UTF8String,
        .frame =
            LogicalRect{
                .origin =
                    {.x = window.frame.origin.x, .y = window.frame.origin.y},
                .size =
                    {.width = window.frame.size.width,
                     .height = window.frame.size.height},
            },
    });
  }

  return windows;
}

std::expected<LogicalRect, Error> MacWindowBackend::windowFrame(
    const std::uint32_t windowId
) {
  CFArrayRef windows = CGWindowListCopyWindowInfo(
      kCGWindowListOptionIncludingWindow, windowId
  );
  if (windows == nullptr) {
    return std::unexpected(Error::windowNotFound(windowId));
  }

  std::optional<LogicalRect> frame;
  if (CFArrayGetCount(windows) > 0) {
    const auto* info =
        static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, 0));
    const auto* boundsDict = static_cast<CFDictionaryRef>(
        CFDictionaryGetValue(info, kCGWindowBounds)
    );
    CGRect bounds = CGRectZero;
    if (boundsDict != nullptr &&
        CGRectMakeWithDictionaryRepresentation(boundsDict, &bounds)) {
      frame = LogicalRect{
          .origin = {.x = bounds.origin.x, .y = bounds.origin.y},
          .size = {.width = bounds.size.width, .height = bounds.size.height},
      };
    }
  }
  CFRelease(windows);

  if (!frame) return std::unexpected(Error::windowNotFound(windowId));
  return *frame;
}

std::expected<std::optional<std::uint32_t>, Error>
MacWindowBackend::topmostWindowId() {
  CFArrayRef windows = CGWindowListCopyWindowInfo(
      kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
      kCGNullWindowID
  );
  if (windows == nullptr) {
    return std::unexpected(
        Error::platformError("CGWindowListCopyWindowInfo returned null")
    );
  }

  std::optional<std::uint32_t> windowId;
  for (CFIndex i = 0; i < CFArrayGetCount(windows); ++i) {
    const auto* info =
        static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, i));
    if (intValue(info, kCGWindowLayer) != 0) continue;
    windowId = static_cast<std::uint32_t>(intValue(info, kCGWindowNumber));
    break;
  }

  CFRelease(windows);
  return windowId;
}

std::expected<void, Error> MacWindowBackend::activateApplicationForWindow(
    const std::uint32_t windowId
) {
  CFArrayRef windows = CGWindowListCopyWindowInfo(
      kCGWindowListOptionIncludingWindow, windowId
  );
  std::int32_t pid = 0;
  if (windows != nullptr) {
    if (CFArrayGetCount(windows) > 0) {
      const auto* info =
          static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(windows, 0));
      pid = intValue(info, kCGWindowOwnerPID);
    }
    CFRelease(windows);
  }
  if (pid == 0) return std::unexpected(Error::windowNotFound(windowId));

  NSRunningApplication* app =
      [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
  if (app == nil || app.terminated) {
    return std::unexpected(Error::windowNotFound(windowId));
  }
  if (![app activateWithOptions:0]) {
    return std::unexpected(
        Error::platformError("NSRunningApplication activation was refused")
    );
  }
  return {};
}

std::expected<std::unique_ptr<backend::IWindowStreamBackend>, Error>
MacWindowBackend::startStream(
    const std::uint32_t windowId, const WindowStreamOptions& options,
    VideoFrameCallback onFrame, StreamErrorCallback onError
) {
  auto contentResult = fetchShareableContent();
  if (!contentResult) return std::unexpected(contentResult.error());

  SCWindow* scWindow = nil;
  for (SCWindow* window in (*contentResult).windows) {
    if (window.windowID == windowId) {
      scWindow = window;
      break;
    }
  }
  if (scWindow == nil) return std::unexpected(Error::windowNotFound(windowId));

  SCContentFilter* filter =
      [[SCContentFilter alloc] initWithDesktopIndependentWindow:scWindow];
  SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
  const double scale = static_cast<double>(filter.pointPixelScale);
  config.width = static_cast<size_t>(filter.contentRect.size.width * scale);
  config.height = static_cast<size_t>(filter.contentRect.size.height * scale);
  config.pixelFormat = kCVPixelFormatType_32BGRA;
  config.minimumFrameInterval =
      CMTimeMake(1, static_cast<std::int32_t>(options.maxFps));
  config.queueDepth = 3;
  config.showsCursor = options.showsCursor ? YES : NO;

  dispatch_queue_t queue =
      dispatch_queue_create("robot.window-stream", DISPATCH_QUEUE_SERIAL);
  auto channel = std::make_shared<StreamChannel>();
  channel->onFrame = std::move(onFrame);
  channel->onError = std::move(onError);
  channel->queue = queue;
  channel->windowId = windowId;
  channel->active.store(true);

  RobotWindowStreamOutput* output = [[RobotWindowStreamOutput alloc] init];
  output->channel = channel;
  SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                        configuration:config
                                             delegate:output];

  NSError* addError = nil;
  [stream addStreamOutput:output
                     type:SCStreamOutputTypeScreen
       sampleHandlerQueue:queue
                    error:&addError];
  if (addError != nil) {
    return std::unexpected(
        Error::captureFailed(addError.localizedDescription.UTF8String)
    );
  }

  dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
  __block NSError* startError = nil;
  [stream startCaptureWithCompletionHandler:^(NSError* err) {
    startError = err;
    dispatch_semaphore_signal(semaphore);
  }];
  const auto deadline =
      dispatch_time(DISPATCH_TIME_NOW, 5 * static_cast<int64_t>(NSEC_PER_SEC));
  if (dispatch_semaphore_wait(semaphore, deadline) != 0) {
    // Startup may still complete later; deactivate the channel and issue a
    // fire-and-forget stop so a late success never delivers frames.
    // startError is deliberately not read here - the completion handler may
    // still be about to write it.
    channel->active.store(false);
    [stream stopCaptureWithCompletionHandler:^(NSError*) {
    }];
    return std::unexpected(Error::captureFailed("timed out starting capture"));
  }
  if (startError != nil) {
    channel->active.store(false);
    return std::unexpected(
        Error::captureFailed(startError.localizedDescription.UTF8String)
    );
  }

  return std::make_unique<MacWindowStream>(
      stream, output, queue, std::move(channel)
  );
}

}  // namespace robot::mac
