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
#include "../../src/Mouse.h"

namespace RobotTest {

class RobotSDLTest : public ::testing::Test {
protected:
    void SetUp() override {
        config_ = std::make_unique<TestConfig>();
        context_ = std::make_unique<TestContext>(*config_);
        context_->prepareForTests();
        SDL_Delay(static_cast<Uint32>(config_->setupDelay.count()));
    }

    void TearDown() override {
        testElements_.clear();
        context_.reset();
    }

    DragElement* createDragElement(int x, int y, int width, int height,
                                   Color color, const std::string& name) {
        auto element = std::make_unique<DragElement>(
            SDL_Rect{x, y, width, height}, color, name);

        auto* rawPtr = element.get();
        testElements_.push_back(std::move(element));
        return rawPtr;
    }

    TestButton* createTestButton(int x, int y, int width, int height,
                                Color color, const std::string& name) {
        auto button = std::make_unique<TestButton>(
            SDL_Rect{x, y, width, height}, color, name);

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

        auto* rawPtr = button.get();
        testElements_.push_back(std::move(button));
        return rawPtr;
    }

    DoubleClickButton* createDoubleClickButton(int x, int y, int width, int height,
                                              Color color, const std::string& name) {
        auto button = std::make_unique<DoubleClickButton>(
            SDL_Rect{x, y, width, height}, color, name);

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

        auto* rawPtr = button.get();
        testElements_.push_back(std::move(button));
        return rawPtr;
    }

    RightClickButton* createRightClickButton(int x, int y, int width, int height,
                                            Color color, const std::string& name) {
        auto button = std::make_unique<RightClickButton>(
            SDL_Rect{x, y, width, height}, color, name);

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

        auto* rawPtr = button.get();
        testElements_.push_back(std::move(button));
        return rawPtr;
    }

    ScrollArea* createScrollArea(int x, int y, int width, int height,
                                Color color, const std::string& name) {
        auto area = std::make_unique<ScrollArea>(
            SDL_Rect{x, y, width, height}, color, name);

        context_->addEventHandler([area = area.get()](const SDL_Event& event) {
            if (event.type == SDL_MOUSEWHEEL) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                if (area->isInside(mouseX, mouseY)) {
                    area->handleScroll(event.wheel.y);
                }
            }
        });

        auto* rawPtr = area.get();
        testElements_.push_back(std::move(area));
        return rawPtr;
    }

    void processEventsFor(std::chrono::milliseconds duration) {
        auto startTime = std::chrono::steady_clock::now();
        bool running = true;

        while (running &&
               (std::chrono::steady_clock::now() - startTime < duration)) {
            context_->handleEvents(running);
            context_->renderFrame([this](SDL_Renderer* renderer) {
                renderTestElements(renderer);
            });
            SDL_Delay(static_cast<Uint32>(config_->frameDelay.count()));
        }
    }

    Robot::Point windowToScreen(int x, int y) const {
        int windowX, windowY;
        SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
        return {x + windowX, y + windowY};
    }

    SDL_Point screenToWindow(int x, int y) const {
        int windowX, windowY;
        SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
        return {x - windowX, y - windowY};
    }

    void performMouseDrag(const SDL_Point& startPoint, const SDL_Point& endPoint) {
        Robot::Point startPos = windowToScreen(startPoint.x, startPoint.y);
        Robot::Point endPos = windowToScreen(endPoint.x, endPoint.y);

        std::cout << "Moving to start position: " << startPos.x << ", " << startPos.y << std::endl;
        Robot::Mouse::MoveSmooth(startPos);

        std::cout << "Performing smooth drag to end position: " << endPos.x << ", " << endPos.y << std::endl;
        Robot::Mouse::DragSmooth(endPos);

        processEventsFor(std::chrono::milliseconds(1000));
    }

    void renderTestElements(SDL_Renderer* renderer) {
        for (const auto& element : testElements_) {
            element->draw(renderer);
        }
        drawMousePosition(renderer);
    }

    void drawMousePosition(SDL_Renderer* renderer) {
        int windowX, windowY;
        SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);

        Robot::Point globalMousePos = Robot::Mouse::GetPosition();
        int localMouseX = globalMousePos.x - windowX;
        int localMouseY = globalMousePos.y - windowY;

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_RenderDrawLine(renderer, localMouseX - 10, localMouseY, localMouseX + 10, localMouseY);
        SDL_RenderDrawLine(renderer, localMouseX, localMouseY - 10, localMouseX, localMouseY + 10);
    }

    std::unique_ptr<TestConfig> config_;
    std::unique_ptr<TestContext> context_;
    std::vector<std::unique_ptr<TestElement>> testElements_;
};

} // namespace RobotTest
