#include <SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <atomic>

#include "TestElements.h"
#include "MouseTests.h"

// Include Robot library headers
#include "../../src/Mouse.h"
#include "../../src/Utils.h"

using namespace RobotTest;

class RobotTestApp {
public:
    RobotTestApp(int argc, char** argv, int width = 800, int height = 600, bool headless = false)
        : width(width), height(height), running(false), headless(headless) {

        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "Could not initialize SDL: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Create window - use appropriate flags for headless mode
        Uint32 windowFlags = SDL_WINDOW_SHOWN;
        if (headless) {
            // For headless mode, we can use minimized or hidden window
            windowFlags = SDL_WINDOW_HIDDEN;
            #ifdef ROBOT_HEADLESS_TESTS
            std::cout << "Running in headless mode with hidden window" << std::endl;
            #endif
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

        // Create renderer - no VSYNC in headless mode
        Uint32 rendererFlags = SDL_RENDERER_ACCELERATED;
        if (!headless) {
            rendererFlags |= SDL_RENDERER_PRESENTVSYNC;
        }

        renderer = SDL_CreateRenderer(window, -1, rendererFlags);

        if (!renderer) {
            std::cerr << "Could not create renderer: " << SDL_GetError() << std::endl;
            exit(1);
        }

        // Initialize mouse test module
        mouseTests = std::make_unique<MouseTests>(renderer, window);

        // In non-headless mode, make sure the window is visible and on top
        if (!headless) {
            SDL_RaiseWindow(window);
            SDL_SetWindowPosition(window, 50, 50);
        }
    }

    ~RobotTestApp() {
        // Clean up any running tests
        if (mouseTests) {
            mouseTests->cleanup();
        }

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

        // Make sure the window is properly initialized and visible (if not headless)
        prepareForTests();

        // Run mouse tests - only drag test
        std::cout << "\n----- Mouse Drag Test -----" << std::endl;

        // Start the test in a separate thread
        mouseTests->startDragTest();

        // Run SDL event loop while tests are executing
        auto startTime = std::chrono::steady_clock::now();
        auto timeout = std::chrono::seconds(30); // 30 seconds timeout

        std::cout << "Running SDL event loop during test execution..." << std::endl;

        // Keep going until the test is completed or timeout
        while (!mouseTests->isTestCompleted()) {
            // Process SDL events - THIS MUST BE ON MAIN THREAD
            handleEvents();

            // Update test state from main thread
            mouseTests->updateFromMainThread();

            // Render the screen
            render();

            // Check if we've timed out
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > timeout) {
                std::cout << "Test execution timed out!" << std::endl;
                break;
            }

            // Don't hog the CPU
            SDL_Delay(16); // ~60 FPS
        }

        // Get test result
        bool testPassed = mouseTests->getTestResult();

        if (!testPassed) {
            std::cout << "❌ Mouse drag test failed" << std::endl;
            allTestsPassed = false;
        } else {
            std::cout << "✅ Mouse drag test passed" << std::endl;
        }

        // Make sure we clean up the test thread
        mouseTests->cleanup();

        // Final results
        std::cout << "\n===== Test Results =====" << std::endl;
        std::cout << (allTestsPassed ? "✅ ALL TESTS PASSED" : "❌ TEST FAILED") << std::endl;

        return allTestsPassed;
    }

private:
    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Forward events to mouse test module
            mouseTests->handleEvent(event);
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

        // Draw mouse test elements
        mouseTests->draw();

        // Present render to the screen
        SDL_RenderPresent(renderer);
    }

    void prepareForTests() {
        std::cout << "Preparing test environment..." << std::endl;

        // In non-headless mode, make window visible and ensure focus
        if (!headless) {
            SDL_ShowWindow(window);
            SDL_SetWindowPosition(window, 50, 50);
            SDL_RaiseWindow(window);
        }

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

        // Get and display window position for debugging (in non-headless mode)
        if (!headless) {
            int x, y;
            SDL_GetWindowPosition(window, &x, &y);
            std::cout << "Window position: (" << x << ", " << y << ")" << std::endl;
        }
    }

    int width, height;
    bool running;
    bool headless;
    SDL_Window* window;
    SDL_Renderer* renderer;

    std::unique_ptr<MouseTests> mouseTests;
};

int main(int argc, char* argv[]) {
    bool runTests = false;
    bool headless = false;
    int waitTime = 2000; // Default wait time in ms before tests

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--run-tests") {
            runTests = true;
        }
        else if (arg == "--headless") {
            headless = true;
        }
        else if (arg == "--wait-time" && i + 1 < argc) {
            waitTime = std::stoi(argv[i + 1]);
            i++;
        }
    }

    // Create test application with appropriate headless setting
    // Pass the argc and argv to the constructor
    RobotTestApp app(argc, argv, 800, 600, headless);

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
