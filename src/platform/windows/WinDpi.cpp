#include "WinDpi.h"

#include <Windows.h>

namespace robot::win {

void ensurePerMonitorDpiAwareness() {
  // SetProcessDpiAwarenessContext is the modern (Windows 10 1703+) entry point
  // and the only one that grants Per-Monitor-V2. Resolve it dynamically so the
  // library links without a hard dependency on a specific Windows SDK import lib
  // and degrades to a clear no-op on older systems rather than failing to load.
  using SetCtxFn = BOOL(WINAPI*)(DPI_AWARENESS_CONTEXT);
  if (HMODULE user32 = GetModuleHandleW(L"user32.dll"); user32 != nullptr) {
    auto set = reinterpret_cast<SetCtxFn>(
        reinterpret_cast<void*>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"))
    );
    if (set != nullptr) {
      set(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
  }
}

}  // namespace robot::win