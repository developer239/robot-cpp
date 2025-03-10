#pragma once

#include <SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <atomic>
#include <mutex>

#include "TestElements.h"
#include "../../src/Mouse.h"
#include "../../src/Utils.h"

namespace RobotTest {

// Test states for thread communication
enum class TestState {
    IDLE,
    INITIALIZING,
    MOVING_TO_START,
    CLICKING,
    PRESSING_MOUSE,
    MOVING_TO_END,
    RELEASING_MOUSE,
    VALIDATING,
    COMPLETED,
    FAILED
};

class MouseTests {
public:
    MouseTests(SDL_Renderer* renderer, SDL_Window* window, bool ciMode = false)
        : renderer(renderer), window(window), testPassed(false),
          testState(TestState::IDLE), testNeedsRendering(false),
          ciMode(ciMode) {

        if (ciMode) {
            std::cout << "MouseTests running in CI mode - will use simulated mouse events" << std::endl;
        }

        // Initialize drag elements for testing - make it larger and more visible
        dragElements.push_back(DragElement(
            {100, 200, 100, 100},
            {255, 200, 0, 255},
            "Drag Me"
        ));

        // Add a heading with instructions
        std::cout << "=====================================" << std::endl;
        std::cout << "MOUSE DRAG TEST" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "The yellow square can be dragged." << std::endl;
        std::cout << "In automatic test mode, the program will:" << std::endl;
        std::cout << "1. Move to the center of the square" << std::endl;
        std::cout << "2. Drag it 100px right and 50px down" << std::endl;
        std::cout << "3. Verify the square moved correctly" << std::endl;
        std::cout << "=====================================" << std::endl;
    }

    void draw() {
        // Draw drag elements
        for (auto& dragElement : dragElements) {
            dragElement.draw(renderer);
        }

        // In CI mode, we don't need to draw mouse position
        if (!ciMode) {
            // Get window position
            int windowX, windowY;
            SDL_GetWindowPosition(window, &windowX, &windowY);

            // Get global mouse position
            Robot::Point globalMousePos = Robot::Mouse::GetPosition();

            // Calculate local mouse position (relative to window)
            int localMouseX = globalMousePos.x - windowX;
            int localMouseY = globalMousePos.y - windowY;

            // Draw mouse position indicator - a red crosshair at the current mouse position
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer, localMouseX-10, localMouseY, localMouseX+10, localMouseY);
            SDL_RenderDrawLine(renderer, localMouseX, localMouseY-10, localMouseX, localMouseY+10);
        }

        // Draw status box with info about test state
        SDL_Rect posRect = {10, 10, 280, 40};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &posRect);

        // Draw border around status box
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &posRect);
    }

    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;

                // Handle drag starts
                for (auto& dragElement : dragElements) {
                    if (dragElement.isInside(x, y)) {
                        dragElement.startDrag();
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                // Stop any dragging
                for (auto& dragElement : dragElements) {
                    if (dragElement.isDragging()) {
                        dragElement.stopDrag();
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEMOTION) {
            int x = event.motion.x;
            int y = event.motion.y;

            // Update draggable elements
            for (auto& dragElement : dragElements) {
                if (dragElement.isDragging()) {
                    dragElement.moveTo(x, y);
                }
            }
        }
    }

    void reset() {
        for (auto& dragElement : dragElements) {
            dragElement.reset();
        }
    }

    // Convert window coordinates to global screen coordinates
    Robot::Point windowToScreen(int x, int y) {
        int windowX, windowY;
        SDL_GetWindowPosition(window, &windowX, &windowY);
        return {x + windowX, y + windowY};
    }

    // Directly inject mouse events for CI mode
    void injectMouseEvent(int type, int x, int y, int button = SDL_BUTTON_LEFT) {
        SDL_Event event;

        switch (type) {
            case SDL_MOUSEBUTTONDOWN:
                event.type = SDL_MOUSEBUTTONDOWN;
                event.button.button = button;
                event.button.x = x;
                event.button.y = y;
                event.button.state = SDL_PRESSED;
                break;

            case SDL_MOUSEBUTTONUP:
                event.type = SDL_MOUSEBUTTONUP;
                event.button.button = button;
                event.button.x = x;
                event.button.y = y;
                event.button.state = SDL_RELEASED;
                break;

            case SDL_MOUSEMOTION:
                event.type = SDL_MOUSEMOTION;
                event.motion.x = x;
                event.motion.y = y;
                event.motion.state = SDL_PRESSED;
                break;
        }

        SDL_PushEvent(&event);
    }

    // This function runs in a separate thread and performs the mouse actions
    void runDragTestThread() {
        std::cout << "Starting mouse drag test in a thread..." << std::endl;

        // Set initial state
        testState = TestState::INITIALIZING;
        testNeedsRendering = true;

        // Wait for main thread to process this state
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Get first drag element position
        int startX = 0, startY = 0, expectedX = 0, expectedY = 0;
        {
            std::lock_guard<std::mutex> lock(testMutex);

            if (dragElements.empty()) {
                std::cout << "No drag elements to test" << std::endl;
                testState = TestState::FAILED;
                testNeedsRendering = true;
                return;
            }

            auto& dragElement = dragElements[0];
            SDL_Rect startRect = dragElement.getRect();

            // Start position (center of element) in window coordinates
            startX = startRect.x + startRect.w/2;
            startY = startRect.y + startRect.h/2;

            // Calculate expected end position
            expectedX = startRect.x + 100; // 100px to the right
            expectedY = startRect.y + 50;  // 50px down
        }

        // End position for drag
        int endX = startX + 100;
        int endY = startY + 50;

        if (ciMode) {
            // In CI mode, directly inject SDL events
            std::cout << "CI Mode: Using simulated mouse events" << std::endl;

            testState = TestState::MOVING_TO_START;
            testNeedsRendering = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            testState = TestState::CLICKING;
            testNeedsRendering = true;
            injectMouseEvent(SDL_MOUSEMOTION, startX, startY);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            injectMouseEvent(SDL_MOUSEBUTTONDOWN, startX, startY);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            injectMouseEvent(SDL_MOUSEBUTTONUP, startX, startY);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            testState = TestState::PRESSING_MOUSE;
            testNeedsRendering = true;
            injectMouseEvent(SDL_MOUSEBUTTONDOWN, startX, startY);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            testState = TestState::MOVING_TO_END;
            testNeedsRendering = true;
            injectMouseEvent(SDL_MOUSEMOTION, endX, endY);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));

            testState = TestState::RELEASING_MOUSE;
            testNeedsRendering = true;
            injectMouseEvent(SDL_MOUSEBUTTONUP, endX, endY);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        } else {
            // Normal mode - use Robot library for real mouse automation
            // Convert to screen coordinates
            Robot::Point startPos = windowToScreen(startX, startY);
            Robot::Point endPos = windowToScreen(endX, endY);

            std::cout << "Start position (screen): (" << startPos.x << ", " << startPos.y << ")" << std::endl;
            std::cout << "End position (screen): (" << endPos.x << ", " << endPos.y << ")" << std::endl;

            // Move to start position
            testState = TestState::MOVING_TO_START;
            testNeedsRendering = true;
            std::cout << "Moving to start position..." << std::endl;
            Robot::Mouse::Move(startPos);
            Robot::delay(300);

            // Click to ensure element is ready for dragging
            testState = TestState::CLICKING;
            testNeedsRendering = true;
            std::cout << "Clicking to select drag element..." << std::endl;
            Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(300);

            // Perform drag operation with states for main thread rendering
            std::cout << "Starting drag operation..." << std::endl;

            // Press the mouse button
            testState = TestState::PRESSING_MOUSE;
            testNeedsRendering = true;
            Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(300);

            // Move to the target position
            testState = TestState::MOVING_TO_END;
            testNeedsRendering = true;
            std::cout << "Moving to end position..." << std::endl;
            Robot::Mouse::Move(endPos);
            Robot::delay(300);

            // Release the mouse button
            testState = TestState::RELEASING_MOUSE;
            testNeedsRendering = true;
            Robot::Mouse::ToggleButton(false, Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(500); // Give time for the drag to complete
        }

        // Validate results
        testState = TestState::VALIDATING;
        testNeedsRendering = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Let the main thread process events before evaluating results
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Validate the results (in a thread-safe way)
        {
            std::lock_guard<std::mutex> lock(testMutex);

            if (dragElements.empty()) {
                testPassed = false;
                testState = TestState::FAILED;
                testNeedsRendering = true;
                return;
            }

            auto& dragElement = dragElements[0];
            SDL_Rect currentRect = dragElement.getRect();

            std::cout << "Element position after drag: (" << currentRect.x << ", " << currentRect.y << ")" << std::endl;

            // Check if element was dragged (should be close to the target position)
            const int tolerance = 20; // pixels

            if (abs(currentRect.x - expectedX) > tolerance ||
                abs(currentRect.y - expectedY) > tolerance) {
                std::cout << "Drag test failed. Expected pos: (" << expectedX << ", " << expectedY
                          << "), Actual: (" << currentRect.x << ", " << currentRect.y << ")" << std::endl;
                testPassed = false;
                testState = TestState::FAILED;
            } else {
                std::cout << "Mouse dragging test passed" << std::endl;
                testPassed = true;
                testState = TestState::COMPLETED;
            }
        }

        testNeedsRendering = true;
    }

    // Start test in a separate thread and return immediately
    void startDragTest() {
        // Reset test state
        testState = TestState::IDLE;
        testPassed = false;
        testNeedsRendering = true;

        // Start the test thread
        if (testThread.joinable()) {
            testThread.join();
        }

        testThread = std::thread(&MouseTests::runDragTestThread, this);
    }

    // Process any test-related events/updates in the main thread
    void updateFromMainThread() {
        // No SDL API calls in test thread - just handle any pending state changes
        if (testNeedsRendering) {
            testNeedsRendering = false;
            // Main thread has now processed this state
        }
    }

    // Check if test is completed
    bool isTestCompleted() const {
        return (testState == TestState::COMPLETED || testState == TestState::FAILED);
    }

    // Get test result
    bool getTestResult() const {
        return testPassed;
    }

    // Clean up test thread
    void cleanup() {
        if (testThread.joinable()) {
            testThread.join();
        }
    }

    bool runAllTests() {
        startDragTest();

        // Main thread will handle SDL events and rendering
        // This function will be used by RobotTestApp

        return true; // Return value not used - test status is checked separately
    }

private:
    SDL_Renderer* renderer;
    SDL_Window* window;
    std::vector<DragElement> dragElements;
    std::thread testThread;
    std::atomic<bool> testPassed;
    std::atomic<TestState> testState;
    std::atomic<bool> testNeedsRendering;
    std::mutex testMutex;
    bool ciMode; // Flag for CI environment
};

} // namespace RobotTest
