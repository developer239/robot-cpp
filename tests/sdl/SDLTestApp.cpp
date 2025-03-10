#include <SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>

#include "TestElements.h"
#include "MouseTests.h"
#include "KeyboardTests.h"
#include "ScreenTests.h"

// Include Robot library headers
#include "../../src/Mouse.h"
#include "../../src/Keyboard.h"
#include "../../src/Screen.h"
#include "../../src/Utils.h"

using namespace RobotTest;

class RobotTestApp {
public:
    RobotTestApp(int width = 800, int height = 600, bool headless = false)
        : width(width), height(height), running(false), headless(headless) {

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Create window
        Uint32 windowFlags = SDL_WINDOW_SHOWN;
        if (headless) {
            windowFlags |= SDL_WINDOW_HIDDEN;
        }

        window = SDL_CreateWindow(
            "Robot CPP Testing Framework",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            width, height,
            windowFlags
        );

        if (!window) {
            std::cerr << "Could not create window: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Create renderer
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        if (!renderer) {
            std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Initialize test modules
        mouseTests = std::make_unique<MouseTests>(renderer, window);
        keyboardTests = std::make_unique<KeyboardTests>(renderer, window);
        screenTests = std::make_unique<ScreenTests>(renderer, window);
    }

    ~RobotTestApp() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void run() {
        running = true;

        while (running) {
            handleEvents();
            render();
            SDL_Delay(16); // ~60 FPS
        }
    }

    bool runTests() {
        bool allTestsPassed = true;

        std::cout << "===== Robot CPP Test Suite =====" << std::endl;

        // Make the window visible for tests even in headless mode
        // This helps ensure the window is properly composited
        if (headless) {
            SDL_ShowWindow(window);
        }

        // Make sure the window is front and center
        SDL_RaiseWindow(window);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

        // Wait a bit for window to be fully shown and composited
        Robot::delay(1000);

        // Run mouse tests
        std::cout << "\n----- Mouse Tests -----" << std::endl;
        if (!mouseTests->runAllTests()) {
            std::cout << "❌ Some mouse tests failed" << std::endl;
            allTestsPassed = false;
        } else {
            std::cout << "✅ All mouse tests passed" << std::endl;
        }

        // Run keyboard tests
        std::cout << "\n----- Keyboard Tests -----" << std::endl;
        if (!keyboardTests->runAllTests()) {
            std::cout << "❌ Some keyboard tests failed" << std::endl;
            allTestsPassed = false;
        } else {
            std::cout << "✅ All keyboard tests passed" << std::endl;
        }

        // Run screen tests
        std::cout << "\n----- Screen Tests -----" << std::endl;
        if (!screenTests->runAllTests()) {
            std::cout << "❌ Some screen tests failed" << std::endl;
            allTestsPassed = false;
        } else {
            std::cout << "✅ All screen tests passed" << std::endl;
        }

        // Final results
        std::cout << "\n===== Test Results =====" << std::endl;
        std::cout << (allTestsPassed ? "✅ ALL TESTS PASSED" : "❌ SOME TESTS FAILED") << std::endl;

        // Hide window again if in headless mode
        if (headless) {
            SDL_HideWindow(window);
        }

        return allTestsPassed;
    }

private:
    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Forward events to test modules
            mouseTests->handleEvent(event);
            keyboardTests->handleEvent(event);
            screenTests->handleEvent(event);
        }
    }

    void render() {
        // Clear screen
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderClear(renderer);

        // Draw title
        SDL_Rect titleRect = {0, 10, width, 40};
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &titleRect);

        // Draw module elements
        mouseTests->draw();
        keyboardTests->draw();
        screenTests->draw();

        // Present
        SDL_RenderPresent(renderer);
    }

    int width, height;
    bool running;
    bool headless;
    SDL_Window* window;
    SDL_Renderer* renderer;

    std::unique_ptr<MouseTests> mouseTests;
    std::unique_ptr<KeyboardTests> keyboardTests;
    std::unique_ptr<ScreenTests> screenTests;
};

int main(int argc, char* argv[]) {
    bool headless = false;
    bool runTests = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            headless = true;
        }
        else if (arg == "--run-tests") {
            runTests = true;
        }
    }

    // Create test application
    RobotTestApp app(800, 600, headless);

    // Either run tests or interactive mode
    if (runTests) {
        bool success = app.runTests();
        return success ? 0 : 1;
    } else {
        app.run();
        return 0;
    }
}
