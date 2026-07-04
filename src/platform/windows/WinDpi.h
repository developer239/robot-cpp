#pragma once

namespace robot::win {

// Declare Per-Monitor-V2 DPI awareness for the process, exactly once, before any
// coordinate or capture call. Without this the OS virtualizes coordinates for a
// "system DPI aware" or unaware process: GetCursorPos / SetCursorPos and monitor
// rectangles are silently scaled, so on any display above 100% the old code
// operated in a lied-about coordinate space. Per-Monitor-V2 makes every metric
// physical and per-display honest, which the screen backend then pairs with each
// monitor's real DPI.
//
// Called from the backend factory. Idempotent and safe if the host application
// already set awareness via manifest (the call fails benignly in that case).
void ensurePerMonitorDpiAwareness();

}  // namespace robot::win