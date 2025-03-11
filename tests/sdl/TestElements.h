#pragma once

#include <SDL.h>
#include <string>
#include <functional>
#include <string_view>
#include <optional>
#include <memory>

namespace RobotTest {

// Struct for consistent color representation
struct Color {
    uint8_t r, g, b, a;

    // Factory methods for common colors
    static constexpr Color White() { return {255, 255, 255, 255}; }
    static constexpr Color Black() { return {0, 0, 0, 255}; }
    static constexpr Color Red() { return {255, 0, 0, 255}; }
    static constexpr Color Green() { return {0, 255, 0, 255}; }
    static constexpr Color Blue() { return {0, 0, 255, 255}; }
    static constexpr Color Yellow() { return {255, 255, 0, 255}; }
    static constexpr Color Orange() { return {255, 165, 0, 255}; }

    // Darken color (factor between 0.0 and 1.0, where 0.0 is no change)
    [[nodiscard]] constexpr Color darken(float factor) const noexcept {
        const auto adjustment = [factor](uint8_t value) -> uint8_t {
            return static_cast<uint8_t>(static_cast<float>(value) * (1.0f - factor));
        };

        return {
            adjustment(r),
            adjustment(g),
            adjustment(b),
            a
        };
    }

    // Convert to SDL_Color
    [[nodiscard]] SDL_Color toSDL() const noexcept {
        return {r, g, b, a};
    }
};

// Interface for all test visual elements
class TestElement {
public:
    virtual ~TestElement() = default;

    virtual void draw(SDL_Renderer* renderer) const = 0;
    [[nodiscard]] virtual bool isInside(int x, int y) const = 0;
    virtual void reset() = 0;

    [[nodiscard]] virtual SDL_Rect getRect() const = 0;
    [[nodiscard]] virtual std::string_view getName() const = 0;
};

// A draggable element for testing drag operations
class DragElement : public TestElement {
public:
    DragElement(SDL_Rect rect, Color color, std::string name)
        : rect_(rect), originalRect_(rect), color_(color), name_(std::move(name)), dragging_(false) {}

    void draw(SDL_Renderer* renderer) const override {
        // Set fill color
        SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
        SDL_RenderFillRect(renderer, &rect_);

        // Draw border
        SDL_SetRenderDrawColor(renderer, Color::White().r, Color::White().g, Color::White().b, Color::White().a);
        SDL_RenderDrawRect(renderer, &rect_);
    }

    [[nodiscard]] bool isInside(int x, int y) const override {
        return (x >= rect_.x && x < rect_.x + rect_.w &&
                y >= rect_.y && y < rect_.y + rect_.h);
    }

    void startDrag() {
        dragging_ = true;
    }

    void stopDrag() {
        dragging_ = false;
    }

    void moveTo(int x, int y) {
        if (dragging_) {
            rect_.x = x - rect_.w/2;
            rect_.y = y - rect_.h/2;
        }
    }

    void reset() override {
        rect_ = originalRect_;
        dragging_ = false;
    }

    [[nodiscard]] SDL_Rect getRect() const override {
        return rect_;
    }

    [[nodiscard]] std::string_view getName() const override {
        return name_;
    }

    [[nodiscard]] bool isDragging() const {
        return dragging_;
    }

private:
    SDL_Rect rect_;
    SDL_Rect originalRect_;
    Color color_;
    std::string name_;
    bool dragging_;
};

// A clickable test button
class TestButton : public TestElement {
public:
    using ClickCallback = std::function<void()>;

    TestButton(SDL_Rect rect, Color color, std::string name,
               std::optional<ClickCallback> callback = std::nullopt)
        : rect_(rect), color_(color), name_(std::move(name)),
          clicked_(false), callback_(std::move(callback)) {}

    void draw(SDL_Renderer* renderer) const override {
        // Set color based on state
        const Color drawColor = clicked_ ? color_ : color_.darken(0.5f);

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
        clicked_ = !clicked_;
        if (callback_ && clicked_) {
            (*callback_)();
        }
    }

    [[nodiscard]] bool wasClicked() const {
        return clicked_;
    }

    void reset() override {
        clicked_ = false;
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
    std::optional<ClickCallback> callback_;
};

// Factory function to create a unique_ptr to a drag element
inline std::unique_ptr<DragElement> createDragElement(
    int x, int y, int width, int height,
    Color color, std::string name)
{
    return std::make_unique<DragElement>(
        SDL_Rect{x, y, width, height},
        color,
        std::move(name)
    );
}

// Factory function to create a unique_ptr to a button
inline std::unique_ptr<TestButton> createButton(
    int x, int y, int width, int height,
    Color color, std::string name,
    TestButton::ClickCallback callback = nullptr)
{
    return std::make_unique<TestButton>(
        SDL_Rect{x, y, width, height},
        color,
        std::move(name),
        callback
    );
}

} // namespace RobotTest
