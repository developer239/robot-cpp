#pragma once

#include <SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>
#include <memory>
#include <string_view>
#include <format>

#include "TestElements.h"
#include "TestContext.h"
#include "../../src/Mouse.h"
#include "../../src/Utils.h"

namespace RobotTest {

// Test states for thread communication
enum class TestState {
    Idle,
    Initializing,
    MovingToStart,
    Clicking,
    PressingMouse,
    MovingToEnd,
    ReleasingMouse,
    Validating,
    Completed,
    Failed
};

/**
 * @brief Class for testing mouse functionality
 */
class MouseTests {
public:
    explicit MouseTests(TestContext& context)
        : context_(context),
          testState_(TestState::Idle),
          testPassed_(false),
          testNeedsRendering_(false) {

        // Initialize drag elements
        auto dragElement = createDragElement(
            100, 200, 100, 100,  // x, y, width, height
            Color::Yellow(),    // color
            "Drag Me"           // name
        );

        dragElements_.push_back(std::move(dragElement));

        // Log test information
        std::cout << "=====================================" << std::endl;
        std::cout << "MOUSE DRAG TEST" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "The yellow square can be dragged." << std::endl;
        std::cout << "In automatic test mode, the program will:" << std::endl;
        std::cout << "1. Move to the center of the square" << std::endl;
        std::cout << "2. Drag it 100px right and 50px down" << std::endl;
        std::cout << "3. Verify the square moved correctly" << std::endl;
        std::cout << "=====================================" << std::endl;

        // Register event handler
        context_.addEventHandler([this](const SDL_Event& event) {
            handleEvent(event);
        });
    }

    void draw() const {
        // Get renderer from context
        SDL_Renderer* renderer = context_.getRenderer();

        // Draw all drag elements
        for (const auto& element : dragElements_) {
            element->draw(renderer);
        }

        // Get window position
        int windowX, windowY;
        SDL_GetWindowPosition(context_.getWindow(), &windowX, &windowY);

        // Get global mouse position
        Robot::Point globalMousePos = Robot::Mouse::GetPosition();

        // Calculate local mouse position (relative to window)
        int localMouseX = globalMousePos.x - windowX;
        int localMouseY = globalMousePos.y - windowY;

        // Draw mouse position indicator - a red crosshair at the current mouse position
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, localMouseX-10, localMouseY, localMouseX+10, localMouseY);
        SDL_RenderDrawLine(renderer, localMouseX, localMouseY-10, localMouseX, localMouseY+10);

        // Draw test status box
        SDL_Rect statusRect = {10, 10, 280, 40};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &statusRect);

        // Draw border around status box
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &statusRect);

        // Draw test state
        // TODO: Add text rendering
    }

    void handleEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x;
                    int y = event.button.y;

                    // Handle drag starts
                    for (auto& element : dragElements_) {
                        if (element->isInside(x, y)) {
                            static_cast<DragElement*>(element.get())->startDrag();
                        }
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    // Stop any dragging
                    for (auto& element : dragElements_) {
                        auto* dragElement = static_cast<DragElement*>(element.get());
                        if (dragElement->isDragging()) {
                            dragElement->stopDrag();
                        }
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                {
                    int x = event.motion.x;
                    int y = event.motion.y;

                    // Update draggable elements
                    for (auto& element : dragElements_) {
                        auto* dragElement = static_cast<DragElement*>(element.get());
                        if (dragElement->isDragging()) {
                            dragElement->moveTo(x, y);
                        }
                    }
                }
                break;
        }
    }

    void reset() {
        for (auto& element : dragElements_) {
            element->reset();
        }
    }

    // Convert window coordinates to global screen coordinates
    [[nodiscard]] Robot::Point windowToScreen(int x, int y) const {
        int windowX, windowY;
        SDL_GetWindowPosition(context_.getWindow(), &windowX, &windowY);
        return {x + windowX, y + windowY};
    }

    // Start test in a separate thread and return immediately
    void startDragTest() {
        // Reset test state
        testState_ = TestState::Idle;
        testPassed_ = false;
        testNeedsRendering_ = true;

        // Start the test thread, joining any previous thread first
        if (testThread_.joinable()) {
            testThread_.join();
        }

        testThread_ = std::thread(&MouseTests::runDragTestThread, this);
    }

    // Run all tests
    bool runAllTests() {
        startDragTest();
        return true; // Return value not used - test status is checked separately
    }

    // Process any test-related events/updates in the main thread
    void updateFromMainThread() {
        if (testNeedsRendering_) {
            testNeedsRendering_ = false;
            // Main thread has now processed this state
        }
    }

    // Check if test is completed
    [[nodiscard]] bool isTestCompleted() const {
        return (testState_ == TestState::Completed || testState_ == TestState::Failed);
    }

    // Get test result
    [[nodiscard]] bool getTestResult() const {
        return testPassed_;
    }

    // Clean up test thread
    void cleanup() noexcept {
        try {
            if (testThread_.joinable()) {
                testThread_.join();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error cleaning up test thread: " << e.what() << std::endl;
        }
    }

private:
    // This function runs in a separate thread and performs the mouse actions
    void runDragTestThread() {
        try {
            std::cout << "Starting mouse drag test in a thread..." << std::endl;

            // Set initial state
            testState_ = TestState::Initializing;
            testNeedsRendering_ = true;

            // Wait for main thread to process this state
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Get first drag element position
            int startX = 0, startY = 0, expectedX = 0, expectedY = 0;

            {
                std::lock_guard<std::mutex> lock(testMutex_);

                if (dragElements_.empty()) {
                    std::cout << "No drag elements to test" << std::endl;
                    testState_ = TestState::Failed;
                    testNeedsRendering_ = true;
                    return;
                }

                auto& dragElement = dragElements_[0];
                SDL_Rect startRect = dragElement->getRect();

                // Start position (center of element) in window coordinates
                startX = startRect.x + startRect.w/2;
                startY = startRect.y + startRect.h/2;

                // Calculate expected end position
                const auto& config = context_.getConfig();
                expectedX = startRect.x + config.dragOffsetX;
                expectedY = startRect.y + config.dragOffsetY;
            }

            // End position for drag
            const auto& config = context_.getConfig();
            int endX = startX + config.dragOffsetX;
            int endY = startY + config.dragOffsetY;

            // Convert to screen coordinates
            Robot::Point startPos = windowToScreen(startX, startY);
            Robot::Point endPos = windowToScreen(endX, endY);

            std::cout << std::format("Start position (screen): ({}, {})", startPos.x, startPos.y) << std::endl;
            std::cout << std::format("End position (screen): ({}, {})", endPos.x, endPos.y) << std::endl;

            // Move to start position
            testState_ = TestState::MovingToStart;
            testNeedsRendering_ = true;
            std::cout << "Moving to start position..." << std::endl;
            Robot::Mouse::Move(startPos);
            Robot::delay(static_cast<unsigned int>(config.actionDelay.count()));

            // Click to ensure element is ready for dragging
            testState_ = TestState::Clicking;
            testNeedsRendering_ = true;
            std::cout << "Clicking to select drag element..." << std::endl;
            Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(static_cast<unsigned int>(config.actionDelay.count()));

            // Perform drag operation with states for main thread rendering
            std::cout << "Starting drag operation..." << std::endl;

            // Press the mouse button
            testState_ = TestState::PressingMouse;
            testNeedsRendering_ = true;
            Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(static_cast<unsigned int>(config.actionDelay.count()));

            // Move to the target position
            testState_ = TestState::MovingToEnd;
            testNeedsRendering_ = true;
            std::cout << "Moving to end position..." << std::endl;
            Robot::Mouse::Move(endPos);
            Robot::delay(static_cast<unsigned int>(config.actionDelay.count()));

            // Release the mouse button
            testState_ = TestState::ReleasingMouse;
            testNeedsRendering_ = true;
            Robot::Mouse::ToggleButton(false, Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(500); // Give time for the drag to complete

            // Validate results
            testState_ = TestState::Validating;
            testNeedsRendering_ = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Let the main thread process events before evaluating results
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Validate the results (in a thread-safe way)
            {
                std::lock_guard<std::mutex> lock(testMutex_);

                if (dragElements_.empty()) {
                    testPassed_ = false;
                    testState_ = TestState::Failed;
                    testNeedsRendering_ = true;
                    return;
                }

                auto& dragElement = dragElements_[0];
                SDL_Rect currentRect = dragElement->getRect();

                std::cout << std::format("Element position after drag: ({}, {})",
                    currentRect.x, currentRect.y) << std::endl;

                // Check if element was dragged (should be close to the target position)
                const int tolerance = config.positionTolerance;

                if (abs(currentRect.x - expectedX) > tolerance ||
                    abs(currentRect.y - expectedY) > tolerance) {
                    std::cout << std::format("Drag test failed. Expected pos: ({}, {}), Actual: ({}, {})",
                        expectedX, expectedY, currentRect.x, currentRect.y) << std::endl;
                    testPassed_ = false;
                    testState_ = TestState::Failed;
                } else {
                    std::cout << "Mouse dragging test passed" << std::endl;
                    testPassed_ = true;
                    testState_ = TestState::Completed;
                }
            }

            testNeedsRendering_ = true;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception in drag test: " << e.what() << std::endl;
            testPassed_ = false;
            testState_ = TestState::Failed;
            testNeedsRendering_ = true;
        }
    }

    TestContext& context_;
    std::vector<std::unique_ptr<TestElement>> dragElements_;
    std::thread testThread_;
    std::atomic<TestState> testState_;
    std::atomic<bool> testPassed_;
    std::atomic<bool> testNeedsRendering_;
    std::mutex testMutex_;
};

} // namespace RobotTest
