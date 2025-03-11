#include <SDL.h>
#include <iostream>
#include <memory>
#include <chrono>
#include <stdexcept>
#include <format>

#include "TestElements.h"
#include "MouseTests.h"
#include "TestContext.h"
#include "TestConfig.h"

#include "../../src/Mouse.h"
#include "../../src/Utils.h"

#include <gtest/gtest.h>

using namespace RobotTest;

/**
 * @brief Main application class for Robot CPP testing
 */
class RobotTestApp {
public:
    explicit RobotTestApp(const TestConfig& config)
        : config_(config),
          running_(false) {

        try {
            // Initialize testing context (SDL, window, renderer)
            context_ = std::make_unique<TestContext>(config_);

            // Initialize test modules
            mouseTests_ = std::make_unique<MouseTests>(*context_);
        }
        catch (const std::exception& e) {
            std::cerr << "Failed to initialize application: " << e.what() << std::endl;
            throw;
        }
    }

    ~RobotTestApp() {
        // Cleanup in reverse order of creation
        if (mouseTests_) {
            mouseTests_->cleanup();
        }

        // TestContext destructor will handle SDL cleanup
    }

    // Run in interactive mode - event loop
    void run() {
        running_ = true;

        std::cout << "Running in interactive mode. Close window to exit." << std::endl;

        while (running_) {
            // Process events
            context_->handleEvents(running_);

            // Render frame
            context_->renderFrame([this](SDL_Renderer* renderer) {
                renderUI(renderer);
            });

            // Cap frame rate
            SDL_Delay(static_cast<Uint32>(config_.frameDelay.count()));
        }
    }

    // Run automated tests
    bool runTests() {
        bool allTestsPassed = true;

        std::cout << "===== Robot CPP Test Suite =====" << std::endl;

        // Prepare the window for testing
        context_->prepareForTests();

        // Run mouse tests
        std::cout << "\n----- Mouse Drag Test -----" << std::endl;

        // Start the test in a separate thread
        mouseTests_->startDragTest();

        // Run SDL event loop while tests are executing
        auto startTime = std::chrono::steady_clock::now();

        std::cout << "Running SDL event loop during test execution..." << std::endl;

        // Keep going until the test is completed or timeout
        while (!mouseTests_->isTestCompleted()) {
            // Process SDL events - THIS MUST BE ON MAIN THREAD
            context_->handleEvents(running_);

            // Update test state from main thread
            mouseTests_->updateFromMainThread();

            // Render the screen with all test elements
            context_->renderFrame([this](SDL_Renderer* renderer) {
                renderUI(renderer);
            });

            // Check if we've timed out
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (elapsed > config_.testTimeout) {
                std::cout << "Test execution timed out!" << std::endl;
                break;
            }

            // Don't hog the CPU
            SDL_Delay(static_cast<Uint32>(config_.frameDelay.count()));
        }

        // Get test result
        bool testPassed = mouseTests_->getTestResult();

        if (!testPassed) {
            std::cout << "❌ Mouse drag test failed" << std::endl;
            allTestsPassed = false;
        } else {
            std::cout << "✅ Mouse drag test passed" << std::endl;
        }

        // Make sure we clean up the test thread
        mouseTests_->cleanup();

        // Final results
        std::cout << "\n===== Test Results =====" << std::endl;
        std::cout << (allTestsPassed ? "✅ ALL TESTS PASSED" : "❌ TEST FAILED") << std::endl;

        return allTestsPassed;
    }

private:
    // Render all UI elements
    void renderUI(SDL_Renderer* renderer) {
        // Draw title bar
        SDL_Rect titleRect = {0, 10, config_.windowWidth, 40};
        SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
        SDL_RenderFillRect(renderer, &titleRect);

        // Draw mouse test elements
        mouseTests_->draw();
    }

    TestConfig config_;
    std::unique_ptr<TestContext> context_;
    std::unique_ptr<MouseTests> mouseTests_;
    bool running_;
};

// Main entry point
int main(int argc, char* argv[]) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    try {
        // Parse command line config
        TestConfig config = TestConfig::fromCommandLine(argc, argv);

        // Create the test application
        RobotTestApp app(config);

        // Either run tests or interactive mode
        if (config.runTests) {
            std::cout << "Initializing test window..." << std::endl;

            // Wait before starting tests to ensure window is ready
            std::cout << std::format("Waiting {:.1f} seconds before starting tests...",
                                    config.initialWaitTime.count() / 1000.0) << std::endl;
            SDL_Delay(static_cast<Uint32>(config.initialWaitTime.count()));

            bool success = app.runTests();
            return success ? 0 : 1;
        } else {
            app.run();
            return 0;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
