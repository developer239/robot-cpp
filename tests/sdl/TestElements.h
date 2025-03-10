#pragma once

#include <SDL.h>
#include <string>
#include <functional>

namespace RobotTest {

// A clickable test button
class TestButton {
public:
    TestButton(SDL_Rect rect, SDL_Color color, const std::string& name)
        : rect(rect), color(color), name(name), clicked(false) {}

    void draw(SDL_Renderer* renderer) {
        // Set color based on state
        if (clicked) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        } else {
            SDL_SetRenderDrawColor(renderer, color.r/2, color.g/2, color.b/2, color.a);
        }

        SDL_RenderFillRect(renderer, &rect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool isInside(int x, int y) const {
        return (x >= rect.x && x < rect.x + rect.w &&
                y >= rect.y && y < rect.y + rect.h);
    }

    void handleClick() {
        clicked = !clicked;
    }

    bool wasClicked() const { return clicked; }
    void reset() { clicked = false; }

    SDL_Rect getRect() const { return rect; }
    std::string getName() const { return name; }

private:
    SDL_Rect rect;
    SDL_Color color;
    std::string name;
    bool clicked;
};

// A draggable element for testing drag operations
class DragElement {
public:
    DragElement(SDL_Rect rect, SDL_Color color, const std::string& name)
        : rect(rect), originalRect(rect), color(color), name(name), dragging(false) {}

    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool isInside(int x, int y) const {
        return (x >= rect.x && x < rect.x + rect.w &&
                y >= rect.y && y < rect.y + rect.h);
    }

    void startDrag() {
        dragging = true;
    }

    void stopDrag() {
        dragging = false;
    }

    void moveTo(int x, int y) {
        if (dragging) {
            rect.x = x - rect.w/2;
            rect.y = y - rect.h/2;
        }
    }

    void reset() {
        rect = originalRect;
        dragging = false;
    }

    SDL_Rect getRect() const { return rect; }
    std::string getName() const { return name; }
    bool isDragging() const { return dragging; }

private:
    SDL_Rect rect;
    SDL_Rect originalRect;
    SDL_Color color;
    std::string name;
    bool dragging;
};

// A text input field for keyboard testing
class TextInput {
public:
    TextInput(SDL_Rect rect, const std::string& name)
        : rect(rect), name(name), text(""), active(false) {}

    void draw(SDL_Renderer* renderer) {
        // Background
        if (active) {
            SDL_SetRenderDrawColor(renderer, 70, 70, 90, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 50, 50, 70, 255);
        }
        SDL_RenderFillRect(renderer, &rect);

        // Border
        SDL_SetRenderDrawColor(renderer, 200, 200, 220, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }

    bool isInside(int x, int y) const {
        return (x >= rect.x && x < rect.x + rect.w &&
                y >= rect.y && y < rect.y + rect.h);
    }

    void activate() {
        active = true;
    }

    void deactivate() {
        active = false;
    }

    void addChar(char c) {
        text += c;
    }

    void removeChar() {
        if (!text.empty()) {
            text.pop_back();
        }
    }

    std::string getText() const { return text; }
    void setText(const std::string& newText) { text = newText; }
    void reset() { text = ""; active = false; }
    bool isActive() const { return active; }

    SDL_Rect getRect() const { return rect; }
    std::string getName() const { return name; }

private:
    SDL_Rect rect;
    std::string name;
    std::string text;
    bool active;
};

// A color area for screen capture testing
class ColorArea {
public:
    ColorArea(SDL_Rect rect, SDL_Color color, const std::string& name)
        : rect(rect), color(color), name(name) {}

    void draw(SDL_Renderer* renderer) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_Rect getRect() const { return rect; }
    SDL_Color getColor() const { return color; }
    std::string getName() const { return name; }

private:
    SDL_Rect rect;
    SDL_Color color;
    std::string name;
};

// A scrollable area for mouse scroll testing
class ScrollArea {
public:
    ScrollArea(SDL_Rect viewRect, int contentHeight, const std::string& name)
        : viewRect(viewRect), contentHeight(contentHeight), name(name), scrollY(0) {}

    void draw(SDL_Renderer* renderer) {
        // Draw background
        SDL_SetRenderDrawColor(renderer, 40, 40, 60, 255);
        SDL_RenderFillRect(renderer, &viewRect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 180, 180, 200, 255);
        SDL_RenderDrawRect(renderer, &viewRect);

        // Draw content stripes (visible based on scroll position)
        for (int y = 0; y < contentHeight; y += 40) {
            SDL_Rect stripe = {
                viewRect.x + 10,
                viewRect.y + 10 + y - scrollY,
                viewRect.w - 20,
                20
            };

            // Only draw if visible in the viewport
            if (stripe.y + stripe.h >= viewRect.y && stripe.y <= viewRect.y + viewRect.h) {
                // Alternate colors
                if ((y / 40) % 2 == 0) {
                    SDL_SetRenderDrawColor(renderer, 100, 100, 150, 255);
                } else {
                    SDL_SetRenderDrawColor(renderer, 150, 150, 200, 255);
                }
                SDL_RenderFillRect(renderer, &stripe);
            }
        }
    }

    void scroll(int amount) {
        scrollY += amount;

        // Limit scrolling
        if (scrollY < 0) {
            scrollY = 0;
        } else {
            int maxScroll = contentHeight - viewRect.h + 20;
            if (maxScroll > 0 && scrollY > maxScroll) {
                scrollY = maxScroll;
            }
        }
    }

    bool isInside(int x, int y) const {
        return (x >= viewRect.x && x < viewRect.x + viewRect.w &&
                y >= viewRect.y && y < viewRect.y + viewRect.h);
    }

    int getScrollY() const { return scrollY; }
    void reset() { scrollY = 0; }

    SDL_Rect getViewRect() const { return viewRect; }
    std::string getName() const { return name; }

private:
    SDL_Rect viewRect;
    int contentHeight;
    std::string name;
    int scrollY;
};

} // namespace RobotTest
