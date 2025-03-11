#pragma once

#include <SDL.h>
#include <string>
#include <functional>
#include <string_view>
#include <optional>
#include <memory>
#include <chrono>

#include "TestElements.h"

namespace RobotTest {

// Extended button that tracks double clicks
class DoubleClickButton : public TestElement {
public:
    DoubleClickButton(SDL_Rect rect, Color color, std::string name)
        : rect_(rect), color_(color), name_(std::move(name)),
          clicked_(false), doubleClicked_(false),
          lastClickTime_(std::chrono::steady_clock::now() - std::chrono::seconds(10)) {}

    void draw(SDL_Renderer* renderer) const override {
        // Set color based on state
        Color drawColor = doubleClicked_ ? color_ :
                          clicked_ ? color_.darken(0.3f) : color_.darken(0.6f);

        SDL_SetRenderDrawColor(renderer, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        SDL_RenderFillRect(renderer, &rect_);

        // Draw border
        SDL_SetRenderDrawColor(renderer, Color::White().r, Color::White().g, Color::White().b, Color::White().a);
        SDL_RenderDrawRect(renderer, &rect_);
    }

    [[nodiscard]] bool isInside(int x, int y) const override {
        return (x >= rect_.x && x < rect_.x + rect_.w &&
                y >= rect_.y && y < rect_.y + rect_.h);
    }

    void handleClick() {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastClick = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastClickTime_).count();

        if (timeSinceLastClick < 300) { // Double-click threshold
            doubleClicked_ = true;
        } else {
            clicked_ = true;
            doubleClicked_ = false;
        }

        lastClickTime_ = now;
    }

    [[nodiscard]] bool wasClicked() const {
        return clicked_;
    }

    [[nodiscard]] bool wasDoubleClicked() const {
        return doubleClicked_;
    }

    void reset() override {
        clicked_ = false;
        doubleClicked_ = false;
        lastClickTime_ = std::chrono::steady_clock::now() - std::chrono::seconds(10);
    }

    [[nodiscard]] SDL_Rect getRect() const override {
        return rect_;
    }

    [[nodiscard]] std::string_view getName() const override {
        return name_;
    }

private:
    SDL_Rect rect_;
    Color color_;
    std::string name_;
    bool clicked_;
    bool doubleClicked_;
    std::chrono::time_point<std::chrono::steady_clock> lastClickTime_;
};

// Button that responds to right clicks
class RightClickButton : public TestElement {
public:
    RightClickButton(SDL_Rect rect, Color color, std::string name)
        : rect_(rect), color_(color), name_(std::move(name)), rightClicked_(false) {}

    void draw(SDL_Renderer* renderer) const override {
        // Set color based on state
        Color drawColor = rightClicked_ ? color_ : color_.darken(0.5f);

        SDL_SetRenderDrawColor(renderer, drawColor.r, drawColor.g, drawColor.b, drawColor.a);
        SDL_RenderFillRect(renderer, &rect_);

        // Draw border
        SDL_SetRenderDrawColor(renderer, Color::White().r, Color::White().g, Color::White().b, Color::White().a);
        SDL_RenderDrawRect(renderer, &rect_);
    }

    [[nodiscard]] bool isInside(int x, int y) const override {
        return (x >= rect_.x && x < rect_.x + rect_.w &&
                y >= rect_.y && y < rect_.y + rect_.h);
    }

    void handleRightClick() {
        rightClicked_ = true;
    }

    [[nodiscard]] bool wasRightClicked() const {
        return rightClicked_;
    }

    void reset() override {
        rightClicked_ = false;
    }

    [[nodiscard]] SDL_Rect getRect() const override {
        return rect_;
    }

    [[nodiscard]] std::string_view getName() const override {
        return name_;
    }

private:
    SDL_Rect rect_;
    Color color_;
    std::string name_;
    bool rightClicked_;
};

// Scrollable area for testing scroll functionality
class ScrollArea : public TestElement {
public:
    ScrollArea(SDL_Rect rect, Color color, std::string name)
        : rect_(rect), color_(color), name_(std::move(name)), scrollY_(0),
          contentHeight_(500) // Content is taller than visible area
    {}

    void draw(SDL_Renderer* renderer) const override {
        // Draw visible area background
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
        SDL_RenderFillRect(renderer, &rect_);

        // Draw border
        SDL_SetRenderDrawColor(renderer, Color::Black().r, Color::Black().g, Color::Black().b, Color::Black().a);
        SDL_RenderDrawRect(renderer, &rect_);

        // Set up a clipping rectangle for the content area
        SDL_Rect clipRect = rect_;
        SDL_RenderSetClipRect(renderer, &clipRect);

        // Draw content (series of colored lines)
        const int lineHeight = 20;
        const int numLines = contentHeight_ / lineHeight;

        for (int i = 0; i < numLines; ++i) {
            // Calculate line Y position with scroll offset
            int lineY = rect_.y + (i * lineHeight) - scrollY_;

            // Skip if line is outside visible area
            if (lineY + lineHeight < rect_.y || lineY > rect_.y + rect_.h) {
                continue;
            }

            // Alternate colors
            Color lineColor = (i % 2 == 0) ? Color::Blue() : Color::Green();
            SDL_SetRenderDrawColor(renderer, lineColor.r, lineColor.g, lineColor.b, lineColor.a);

            SDL_Rect lineRect = {rect_.x + 2, lineY, rect_.w - 4, lineHeight};
            SDL_RenderFillRect(renderer, &lineRect);

            // Draw line number
            // Text rendering would go here if we had SDL_ttf
        }

        // Draw scrollbar track
        SDL_Rect scrollTrack = {
            rect_.x + rect_.w - 15,
            rect_.y,
            15,
            rect_.h
        };
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &scrollTrack);

        // Draw scrollbar thumb
        float visibleRatio = static_cast<float>(rect_.h) / contentHeight_;
        int thumbHeight = static_cast<int>(rect_.h * visibleRatio);
        int thumbY = rect_.y + static_cast<int>((static_cast<float>(scrollY_) /
                    (contentHeight_ - rect_.h)) * (rect_.h - thumbHeight));

        SDL_Rect scrollThumb = {
            rect_.x + rect_.w - 15,
            thumbY,
            15,
            thumbHeight
        };
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderFillRect(renderer, &scrollThumb);

        // Reset clipping rectangle
        SDL_RenderSetClipRect(renderer, nullptr);
    }

    [[nodiscard]] bool isInside(int x, int y) const override {
        return (x >= rect_.x && x < rect_.x + rect_.w &&
                y >= rect_.y && y < rect_.y + rect_.h);
    }

    void handleScroll(int scrollAmount) {
        scrollY_ += scrollAmount * 15; // Scale the scroll amount

        // Clamp scrollY to valid range
        if (scrollY_ < 0) {
            scrollY_ = 0;
        }

        int maxScroll = contentHeight_ - rect_.h;
        if (maxScroll < 0) {
            maxScroll = 0;
        }

        if (scrollY_ > maxScroll) {
            scrollY_ = maxScroll;
        }
    }

    [[nodiscard]] int getScrollY() const {
        return scrollY_;
    }

    void reset() override {
        scrollY_ = 0;
    }

    [[nodiscard]] SDL_Rect getRect() const override {
        return rect_;
    }

    [[nodiscard]] std::string_view getName() const override {
        return name_;
    }

private:
    SDL_Rect rect_;
    Color color_;
    std::string name_;
    int scrollY_;
    int contentHeight_;
};

// Factory function to create a double-click button
inline std::unique_ptr<DoubleClickButton> createDoubleClickButton(
    int x, int y, int width, int height,
    Color color, std::string name)
{
    return std::make_unique<DoubleClickButton>(
        SDL_Rect{x, y, width, height},
        color,
        std::move(name)
    );
}

// Factory function to create a right-click button
inline std::unique_ptr<RightClickButton> createRightClickButton(
    int x, int y, int width, int height,
    Color color, std::string name)
{
    return std::make_unique<RightClickButton>(
        SDL_Rect{x, y, width, height},
        color,
        std::move(name)
    );
}

// Factory function to create a scroll area
inline std::unique_ptr<ScrollArea> createScrollArea(
    int x, int y, int width, int height,
    Color color, std::string name)
{
    return std::make_unique<ScrollArea>(
        SDL_Rect{x, y, width, height},
        color,
        std::move(name)
    );
}

} // namespace RobotTest
