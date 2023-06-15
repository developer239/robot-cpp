#pragma once

#include <windows.h>
#include <iostream>
#include "./ActionRecorder.h"

namespace Robot {

  class EventHook {
    public:
      explicit EventHook(ActionRecorder& recorder) : recorder(recorder) {
        instance = this; // Store the instance pointer in the static variable
      }

      void StartRecording() {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        if (!hInstance) {
          std::cerr << "Failed to get instance handle" << std::endl;
          exit(1);
        }

        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
        keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);

        if (!mouseHook || !keyboardHook) {
          std::cerr << "Failed to set hooks" << std::endl;
          exit(1);
        }

        recordingThreadId = GetCurrentThreadId();

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
          if (msg.message == WM_QUIT) {
            break;
          }
          TranslateMessage(&msg);
          DispatchMessage(&msg);
        }
      }

      void StopRecording() {
        UnhookWindowsHookEx(mouseHook);
        UnhookWindowsHookEx(keyboardHook);

        PostThreadMessage(recordingThreadId, WM_QUIT, 0, 0);
      }

    private:
      DWORD recordingThreadId;

      ActionRecorder& recorder;
      HHOOK mouseHook;
      HHOOK keyboardHook;
      static EventHook* instance; // Static variable to store instance pointer

      static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && instance) {
          auto* mouseData = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
          switch (wParam) {
            case WM_MOUSEMOVE:
              instance->recorder.RecordMouseMove(mouseData->pt.x, mouseData->pt.y);
              break;
            case WM_LBUTTONDOWN:
              instance->recorder.RecordPressLeft(mouseData->pt.x, mouseData->pt.y);
              break;
            case WM_LBUTTONUP:
              instance->recorder.RecordReleaseLeft(mouseData->pt.x, mouseData->pt.y);
              break;
          }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
      }

      static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode == HC_ACTION && instance) {
          auto* keyboardData = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
          switch (wParam) {
            case WM_KEYDOWN:
              instance->recorder.RecordKeyPress(keyboardData->vkCode);
              break;
            case WM_KEYUP:
              instance->recorder.RecordKeyRelease(keyboardData->vkCode);
              break;
          }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
      }
  };

// Initialize the static instance pointer
  EventHook* EventHook::instance = nullptr;

} // namespace Robot
