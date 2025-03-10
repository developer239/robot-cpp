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

        // Create window - IMPORTANT: Use SDL_WINDOW_SHOWN flag to ensure the window is visible
        Uint32 windowFlags = SDL_WINDOW_SHOWN;

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

        // Create renderer with VSYNC to prevent rendering issues
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!renderer) {
            std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Initialize test modules
        mouseTests = std::make_unique<MouseTests>(renderer, window);
        keyboardTests = std::make_unique<KeyboardTests>(renderer, window);
        screenTests = std::make_unique<ScreenTests>(renderer, window);

        // Force the window to be on top
        SDL_RaiseWindow(window);

        // Position window consistently
        SDL_SetWindowPosition(window, 50, 50);
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

        // Make sure the window is properly initialized and visible
        prepareForTests();

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
        // Clear screen with a dark gray background
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

        // Present render to the screen
        SDL_RenderPresent(renderer);
    }

    void prepareForTests() {
        std::cout << "Preparing test environment..." << std::endl;

        // Make sure window is visible
        SDL_ShowWindow(window);

        // Ensure window is positioned correctly
        SDL_SetWindowPosition(window, 50, 50);

        // Make sure the window is on top
        SDL_RaiseWindow(window);

        // Render several frames to ensure the window is properly displayed
        for (int i = 0; i < 5; i++) {
            render();
            SDL_Delay(100);
        }

        // Process any pending events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Just drain the event queue
        }

        // Additional delay to ensure window is ready
        SDL_Delay(500);

        // Get and display window position for debugging
        int x, y;
        SDL_GetWindowPosition(window, &x, &y);
        std::cout << "Window position: (" << x << ", " << y << ")" << std::endl;
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
    bool runTests = false;
    int waitTime = 2000; // Default wait time in ms before tests

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--run-tests") {
            runTests = true;
        }
        else if (arg == "--wait-time" && i + 1 < argc) {
            waitTime = std::stoi(argv[i + 1]);
            i++;
        }
    }

    // Create test application (never headless to ensure window is visible)
    RobotTestApp app(800, 600, false);

    // Either run tests or interactive mode
    if (runTests) {
        std::cout << "Initializing test window..." << std::endl;

        // Wait before starting tests to ensure window is ready
        std::cout << "Waiting " << waitTime/1000.0 << " seconds before starting tests..." << std::endl;
        SDL_Delay(waitTime);

        bool success = app.runTests();
        return success ? 0 : 1;
    } else {
        app.run();
        return 0;
    }
}
