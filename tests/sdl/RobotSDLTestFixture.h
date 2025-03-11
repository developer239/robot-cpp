#pragma once

#include <gtest/gtest.h>
#include <functional>
#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <memory>

#include "TestContext.h"
#include "TestConfig.h"
#include "TestElements.h"
#include "ExtendedTestElements.h"
#include "../../src/Mouse.h"

namespace RobotTest {
    /**
     * @brief Test fixture for SDL-based Robot tests
     *
     * This fixture handles SDL initialization, window creation, and
     * cleanup for all Robot CPP SDL-based tests.
     */
    class RobotSDLTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Initialize test config with reasonable defaults
            config_ = std::make_unique<TestConfig>();

            // Initialize SDL, window, and renderer
            context_ = std::make_unique<TestContext>(*config_);

            // Wait for window to be ready
            context_->prepareForTests();
            SDL_Delay(static_cast<Uint32>(config_->setupDelay.count()));
        }

        void TearDown() override {
            // Clear all test elements
            testElements_.clear();

            // TestContext destructor will handle SDL cleanup
            context_.reset();
        }

        /**
         * @brief Creates a drag element and adds it to the test elements collection
         * @return Pointer to the created drag element (owned by the fixture)
         */
        DragElement *createDragElement(int x, int y, int width, int height,
                                       Color color, const std::string &name) {
            auto element = std::make_unique<DragElement>(
                SDL_Rect{x, y, width, height}, color, name);

            auto *rawPtr = element.get();
            testElements_.push_back(std::move(element));
            return rawPtr;
        }

        /**
         * @brief Creates a test button and adds it to the test elements collection
         * @return Pointer to the created button (owned by the fixture)
         */
        TestButton *createTestButton(int x, int y, int width, int height,
                                    Color color, const std::string &name) {
            auto button = std::make_unique<TestButton>(
                SDL_Rect{x, y, width, height}, color, name);

            // Add click event handler
            context_->addEventHandler([button = button.get()](const SDL_Event& event) {
                if (event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x;
                    int y = event.button.y;
                    if (button->isInside(x, y)) {
                        button->handleClick();
                    }
                }
            });

            auto *rawPtr = button.get();
            testElements_.push_back(std::move(button));
            return rawPtr;
        }

        /**
         * @brief Creates a double-click button and adds it to the test elements collection
         * @return Pointer to the created button (owned by the fixture)
         */
        DoubleClickButton *createDoubleClickButton(int x, int y, int width, int height,
                                                  Color color, const std::string &name) {
            auto button = std::make_unique<DoubleClickButton>(
                SDL_Rect{x, y, width, height}, color, name);

            // Add click event handler
            context_->addEventHandler([button = button.get()](const SDL_Event& event) {
                if (event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_LEFT) {
                    int x = event.button.x;
                    int y = event.button.y;
                    if (button->isInside(x, y)) {
                        button->handleClick();
                    }
                }
            });

            auto *rawPtr = button.get();
            testElements_.push_back(std::move(button));
            return rawPtr;
        }

        /**
         * @brief Creates a right-click button and adds it to the test elements collection
         * @return Pointer to the created button (owned by the fixture)
         */
        RightClickButton *createRightClickButton(int x, int y, int width, int height,
                                                Color color, const std::string &name) {
            auto button = std::make_unique<RightClickButton>(
                SDL_Rect{x, y, width, height}, color, name);

            // Add right-click event handler
            context_->addEventHandler([button = button.get()](const SDL_Event& event) {
                if (event.type == SDL_MOUSEBUTTONDOWN &&
                    event.button.button == SDL_BUTTON_RIGHT) {
                    int x = event.button.x;
                    int y = event.button.y;
                    if (button->isInside(x, y)) {
                        button->handleRightClick();
                    }
                }
            });

            auto *rawPtr = button.get();
            testElements_.push_back(std::move(button));
            return rawPtr;
        }

        /**
         * @brief Creates a scroll area and adds it to the test elements collection
         * @return Pointer to the created scroll area (owned by the fixture)
         */
        ScrollArea *createScrollArea(int x, int y, int width, int height,
                                    Color color, const std::string &name) {
            auto area = std::make_unique<ScrollArea>(
                SDL_Rect{x, y, width, height}, color, name);

            // Add scroll event handler
            context_->addEventHandler([area = area.get()](const SDL_Event& event) {
                if (event.type == SDL_MOUSEWHEEL) {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    if (area->isInside(mouseX, mouseY)) {
                        area->handleScroll(event.wheel.y);
                    }
                }
            });

            auto *rawPtr = area.get();
            testElements_.push_back(std::move(area));
            return rawPtr;
        }

        /**
         * @brief Runs the event loop for a specified duration
         * @param duration How long to process events
         */
        void processEventsFor(std::chrono::milliseconds duration) {
            auto startTime = std::chrono::steady_clock::now();
            bool running = true;

            while (running &&
                   (std::chrono::steady_clock::now() - startTime < duration)) {
                // Process pending SDL events
                context_->handleEvents(running);

                // Render current state
                context_->renderFrame([this](SDL_Renderer *renderer) {
                    renderTestElements(renderer);
                });

                // Limit frame rate
                SDL_Delay(static_cast<Uint32>(config_->frameDelay.count()));
            }
        }

        /**
         * @brief Converts window coordinates to screen coordinates
         */
        Robot::Point windowToScreen(int x, int y) const {
            int windowX, windowY;
            SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
            return {x + windowX, y + windowY};
        }

        /**
         * @brief Converts screen coordinates to window coordinates
         */
        SDL_Point screenToWindow(int x, int y) const {
            int windowX, windowY;
            SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
            return {x - windowX, y - windowY};
        }

        /**
         * @brief Performs a mouse drag operation
         * @param startPoint Starting point in window coordinates
         * @param endPoint Ending point in window coordinates
         */
        void performMouseDrag(const SDL_Point &startPoint, const SDL_Point &endPoint) {
            // Convert to screen coordinates
            Robot::Point startPos = windowToScreen(startPoint.x, startPoint.y);
            Robot::Point endPos = windowToScreen(endPoint.x, endPoint.y);

            std::cout << "Moving to start position: " << startPos.x << ", " << startPos.y << std::endl;

            // Move to start position
            Robot::Mouse::MoveSmooth(startPos);

            // Perform the drag operation using the library's DragSmooth method
            std::cout << "Performing smooth drag to end position: " << endPos.x << ", " << endPos.y << std::endl;
            Robot::Mouse::DragSmooth(endPos);

            // Process events to ensure drag is applied
            processEventsFor(std::chrono::milliseconds(1000));
        }

        /**
         * @brief Renders all test elements
         */
        void renderTestElements(SDL_Renderer *renderer) {
            for (const auto &element: testElements_) {
                element->draw(renderer);
            }

            // Draw mouse cursor position
            drawMousePosition(renderer);
        }

        /**
         * @brief Draws the current mouse position on screen
         */
        void drawMousePosition(SDL_Renderer *renderer) {
            // Get window position
            int windowX, windowY;
            SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);

            // Get global mouse position
            Robot::Point globalMousePos = Robot::Mouse::GetPosition();

            // Calculate local mouse position (relative to window)
            int localMouseX = globalMousePos.x - windowX;
            int localMouseY = globalMousePos.y - windowY;

            // Draw mouse position indicator - a red crosshair
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            SDL_RenderDrawLine(renderer, localMouseX - 10, localMouseY, localMouseX + 10, localMouseY);
            SDL_RenderDrawLine(renderer, localMouseX, localMouseY - 10, localMouseX, localMouseY + 10);

            // Draw coordinates text (would require SDL_ttf, so omitting)
        }

        std::unique_ptr<TestConfig> config_;
        std::unique_ptr<TestContext> context_;
        std::vector<std::unique_ptr<TestElement> > testElements_;
    };
} // namespace RobotTest
