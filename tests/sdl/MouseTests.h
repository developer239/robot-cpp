#pragma once

#include <SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>

#include "TestElements.h"
#include "../../src/Mouse.h"
#include "../../src/Utils.h"

namespace RobotTest {

class MouseTests {
public:
    MouseTests(SDL_Renderer* renderer, SDL_Window* window)
        : renderer(renderer), window(window) {

        // Initialize test elements
        buttons.push_back(TestButton(
            {100, 100, 100, 50},
            {255, 100, 100, 255},
            "RedButton"
        ));

        buttons.push_back(TestButton(
            {250, 100, 100, 50},
            {100, 255, 100, 255},
            "GreenButton"
        ));

        buttons.push_back(TestButton(
            {400, 100, 100, 50},
            {100, 100, 255, 255},
            "BlueButton"
        ));

        // Draggable elements
        dragElements.push_back(DragElement(
            {100, 200, 80, 80},
            {255, 200, 0, 255},
            "YellowBox"
        ));

        // Scroll area
        scrollArea = std::make_unique<ScrollArea>(
            SDL_Rect{100, 350, 400, 150},
            500, // Content height
            "MainScrollArea"
        );
    }

    void draw() {
        // Draw all test elements
        for (auto& button : buttons) {
            button.draw(renderer);
        }

        for (auto& dragElement : dragElements) {
            dragElement.draw(renderer);
        }

        if (scrollArea) {
            scrollArea->draw(renderer);
        }

        // Draw mouse position indicator
        SDL_Rect posRect = {10, 10, 150, 30};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &posRect);

        Robot::Point mousePos = Robot::Mouse::GetPosition();
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, mousePos.x-10, mousePos.y, mousePos.x+10, mousePos.y);
        SDL_RenderDrawLine(renderer, mousePos.x, mousePos.y-10, mousePos.x, mousePos.y+10);
    }

    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;

                // Handle button clicks
                for (auto& button : buttons) {
                    if (button.isInside(x, y)) {
                        button.handleClick();
                    }
                }

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
        else if (event.type == SDL_MOUSEWHEEL) {
            // Get mouse position
            int x, y;
            SDL_GetMouseState(&x, &y);

            // Check if over scroll area
            if (scrollArea && scrollArea->isInside(x, y)) {
                scrollArea->scroll(-event.wheel.y * 20); // Adjust speed as needed
            }
        }
    }

    void reset() {
        for (auto& button : buttons) {
            button.reset();
        }

        for (auto& dragElement : dragElements) {
            dragElement.reset();
        }

        if (scrollArea) {
            scrollArea->reset();
        }
    }

    // Test mouse movement accuracy
    bool testMouseMovement() {
        std::cout << "Testing mouse movement..." << std::endl;
        reset();

        // Move to specific positions and verify
        std::vector<Robot::Point> testPoints = {
            {100, 100}, // Top-left button
            {300, 100}, // Middle button
            {450, 100}, // Right button
            {140, 240}  // Drag element
        };

        for (const auto& point : testPoints) {
            Robot::Mouse::Move(point);
            Robot::delay(300); // Give time for move to complete

            Robot::Point actualPos = Robot::Mouse::GetPosition();

            // Check if position is within tolerance
            const int tolerance = 5; // pixels
            if (abs(actualPos.x - point.x) > tolerance || abs(actualPos.y - point.y) > tolerance) {
                std::cout << "Move test failed. Expected: (" << point.x << ", " << point.y
                          << "), Actual: (" << actualPos.x << ", " << actualPos.y << ")" << std::endl;
                return false;
            }
        }

        std::cout << "Mouse movement test passed" << std::endl;
        return true;
    }

    // Test mouse clicking
    bool testMouseClicking() {
        std::cout << "Testing mouse clicking..." << std::endl;
        reset();

        // Process pending SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Click each button and verify
        for (auto& button : buttons) {
            SDL_Rect rect = button.getRect();
            Robot::Point center = {rect.x + rect.w/2, rect.y + rect.h/2};

            // Move to button
            Robot::Mouse::Move(center);
            Robot::delay(300);

            // Click button
            Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
            Robot::delay(300);

            // Process the SDL events to register the click
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                handleEvent(event);
            }

            // Verify button state changed
            if (!button.wasClicked()) {
                std::cout << "Click test failed for button: " << button.getName() << std::endl;
                return false;
            }
        }

        std::cout << "Mouse clicking test passed" << std::endl;
        return true;
    }

    // Test drag functionality
    bool testMouseDragging() {
        std::cout << "Testing mouse dragging..." << std::endl;
        reset();

        if (dragElements.empty()) {
            std::cout << "No drag elements to test" << std::endl;
            return false;
        }

        // Get first drag element
        auto& dragElement = dragElements[0];
        SDL_Rect startRect = dragElement.getRect();

        // Process pending SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Start position (center of element)
        Robot::Point startPos = {
            startRect.x + startRect.w/2,
            startRect.y + startRect.h/2
        };

        // End position (100px to the right)
        Robot::Point endPos = {
            startPos.x + 100,
            startPos.y + 50
        };

        // Move to start position
        Robot::Mouse::Move(startPos);
        Robot::delay(300);

        // Perform drag operation
        Robot::Mouse::Drag(endPos);
        Robot::delay(300);

        // Process events to register the drag
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Get new position
        SDL_Rect currentRect = dragElement.getRect();

        // Check if element was dragged (should be close to the target position)
        const int tolerance = 15; // pixels
        int expectedX = endPos.x - startRect.w/2;
        int expectedY = endPos.y - startRect.h/2;

        if (abs(currentRect.x - expectedX) > tolerance ||
            abs(currentRect.y - expectedY) > tolerance) {
            std::cout << "Drag test failed. Expected pos: (" << expectedX << ", " << expectedY
                      << "), Actual: (" << currentRect.x << ", " << currentRect.y << ")" << std::endl;
            return false;
        }

        std::cout << "Mouse dragging test passed" << std::endl;
        return true;
    }

    // Test mouse smooth movement
    bool testMouseSmoothMovement() {
        std::cout << "Testing mouse smooth movement..." << std::endl;
        reset();

        // Start position
        Robot::Point startPos = {100, 300};

        // End position (diagonal movement)
        Robot::Point endPos = {400, 400};

        // Move to start
        Robot::Mouse::Move(startPos);
        Robot::delay(300);

        // Perform smooth move
        Robot::Mouse::MoveSmooth(endPos);
        Robot::delay(300);

        // Check final position
        Robot::Point actualPos = Robot::Mouse::GetPosition();

        // Verify we reached the destination
        const int tolerance = 5; // pixels
        if (abs(actualPos.x - endPos.x) > tolerance || abs(actualPos.y - endPos.y) > tolerance) {
            std::cout << "Smooth move test failed. Expected: (" << endPos.x << ", " << endPos.y
                      << "), Actual: (" << actualPos.x << ", " << actualPos.y << ")" << std::endl;
            return false;
        }

        std::cout << "Mouse smooth movement test passed" << std::endl;
        return true;
    }

    // Test mouse scrolling
    bool testMouseScrolling() {
        std::cout << "Testing mouse scrolling..." << std::endl;

        if (!scrollArea) {
            std::cout << "No scroll area to test" << std::endl;
            return false;
        }

        // Reset scroll position
        scrollArea->reset();

        // Process pending SDL events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Move to scroll area
        SDL_Rect viewRect = scrollArea->getViewRect();
        Robot::Point scrollCenter = {
            viewRect.x + viewRect.w/2,
            viewRect.y + viewRect.h/2
        };

        Robot::Mouse::Move(scrollCenter);
        Robot::delay(300);

        // Initial scroll position
        int initialScroll = scrollArea->getScrollY();

        // Scroll down
        Robot::Mouse::ScrollBy(-3); // Negative y is down in Robot API
        Robot::delay(300);

        // Process events to register the scroll
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify scroll position changed
        int newScroll = scrollArea->getScrollY();
        if (newScroll <= initialScroll) {
            std::cout << "Scroll test failed. Scroll position didn't increase." << std::endl;
            return false;
        }

        // Scroll back up
        Robot::Mouse::ScrollBy(6); // Positive y is up in Robot API
        Robot::delay(300);

        // Process events
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify scroll changed back
        int finalScroll = scrollArea->getScrollY();
        if (finalScroll >= newScroll) {
            std::cout << "Scroll test failed. Scroll position didn't decrease." << std::endl;
            return false;
        }

        std::cout << "Mouse scrolling test passed" << std::endl;
        return true;
    }

    bool runAllTests() {
        bool allPassed = true;

        // Run all mouse tests
        allPassed &= testMouseMovement();
        allPassed &= testMouseClicking();
        allPassed &= testMouseDragging();
        allPassed &= testMouseSmoothMovement();
        allPassed &= testMouseScrolling();

        return allPassed;
    }

private:
    SDL_Renderer* renderer;
    SDL_Window* window;

    std::vector<TestButton> buttons;
    std::vector<DragElement> dragElements;
    std::unique_ptr<ScrollArea> scrollArea;
};

} // namespace RobotTest
