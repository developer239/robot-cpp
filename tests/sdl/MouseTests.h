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

        // Draw status box with info about mouse position
        SDL_Rect posRect = {10, 10, 180, 40};
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_RenderFillRect(renderer, &posRect);

        // Draw border around status box
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(renderer, &posRect);

        // Unfortunately, we can't draw text directly as we're not using SDL_ttf library
        // But we leave the box to show where coordinates would be displayed
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

        // Get window position
        int windowX, windowY;
        SDL_GetWindowPosition(window, &windowX, &windowY);
        std::cout << "Window position: (" << windowX << ", " << windowY << ")" << std::endl;

        // Start position (center of element) in window coordinates
        int startX = startRect.x + startRect.w/2;
        int startY = startRect.y + startRect.h/2;

        // Convert to screen coordinates
        Robot::Point startPos = windowToScreen(startX, startY);

        // End position (100px to the right) in screen coordinates
        Robot::Point endPos = windowToScreen(startX + 100, startY + 50);

        std::cout << "Start position (screen): (" << startPos.x << ", " << startPos.y << ")" << std::endl;
        std::cout << "End position (screen): (" << endPos.x << ", " << endPos.y << ")" << std::endl;

        // Move to start position
        Robot::Mouse::Move(startPos);
        Robot::delay(300);

        // click on the screen
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::delay(3000);

        // Perform drag operation
        Robot::Mouse::Drag(endPos);
        Robot::delay(500); // Give a bit more time for the drag to complete

        // Process events to register the drag
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Additional processing to ensure events are processed
        SDL_PumpEvents();
        Robot::delay(200);

        // Get new position
        SDL_Rect currentRect = dragElement.getRect();
        std::cout << "Element position after drag: (" << currentRect.x << ", " << currentRect.y << ")" << std::endl;

        // Check if element was dragged (should be close to the target position)
        const int tolerance = 20; // pixels (increased tolerance slightly)
        int expectedX = startRect.x + 100; // 100px to the right
        int expectedY = startRect.y + 50;  // 50px down

        if (abs(currentRect.x - expectedX) > tolerance ||
            abs(currentRect.y - expectedY) > tolerance) {
            std::cout << "Drag test failed. Expected pos: (" << expectedX << ", " << expectedY
                      << "), Actual: (" << currentRect.x << ", " << currentRect.y << ")" << std::endl;
            return false;
        }

        std::cout << "Mouse dragging test passed" << std::endl;
        return true;
    }

    bool runAllTests() {
        // Only run the drag test
        return testMouseDragging();
    }

private:
    SDL_Renderer* renderer;
    SDL_Window* window;
    std::vector<DragElement> dragElements;
};

} // namespace RobotTest
