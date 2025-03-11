#pragma once

#include <SDL.h>
#include <memory>
#include <stdexcept>
#include "TestConfig.h"

namespace RobotTest {

/**
 * @brief RAII Wrapper for SDL initialization and window/renderer creation
 */
class TestContext {
public:
    explicit TestContext(const TestConfig& config) : config_(config) {
        // Initialize SDL
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            throw std::runtime_error(
                std::string("Could not initialize SDL: ") + SDL_GetError());
        }

        initialized_ = true;

        // Create window
        window_ = SDL_CreateWindow(
            config_.windowTitle.c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            config_.windowWidth, config_.windowHeight,
            SDL_WINDOW_SHOWN
        );

        if (!window_) {
            throw std::runtime_error(
                std::string("Could not create window: ") + SDL_GetError());
        }

        // Create renderer with VSYNC
        renderer_ = SDL_CreateRenderer(
            window_, -1,
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
        );

        if (!renderer_) {
            throw std::runtime_error(
                std::string("Could not create renderer: ") + SDL_GetError());
        }

        // Make sure the window is visible and focused
        SDL_RaiseWindow(window_);
        SDL_SetWindowPosition(window_, config_.windowX, config_.windowY);
    }

    ~TestContext() {
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }

        if (window_) {
            SDL_DestroyWindow(window_);
        }

        if (initialized_) {
            SDL_Quit();
        }
    }

    // Prevent copying
    TestContext(const TestContext&) = delete;
    TestContext& operator=(const TestContext&) = delete;

    // Allow moving
    TestContext(TestContext&& other) noexcept
        : window_(other.window_),
          renderer_(other.renderer_),
          initialized_(other.initialized_),
          config_(other.config_) {
        other.window_ = nullptr;
        other.renderer_ = nullptr;
        other.initialized_ = false;
    }

    TestContext& operator=(TestContext&& other) noexcept {
        if (this != &other) {
            if (renderer_) SDL_DestroyRenderer(renderer_);
            if (window_) SDL_DestroyWindow(window_);
            if (initialized_) SDL_Quit();

            window_ = other.window_;
            renderer_ = other.renderer_;
            initialized_ = other.initialized_;
            config_ = other.config_;

            other.window_ = nullptr;
            other.renderer_ = nullptr;
            other.initialized_ = false;
        }
        return *this;
    }

    SDL_Renderer* getRenderer() const { return renderer_; }
    SDL_Window* getWindow() const { return window_; }
    const TestConfig& getConfig() const { return config_; }

    // Prepare window for tests
    void prepareForTests() {
        // Make window visible and ensure focus
        SDL_ShowWindow(window_);
        SDL_SetWindowPosition(window_, config_.windowX, config_.windowY);
        SDL_RaiseWindow(window_);

        // Render several frames to ensure the window is properly displayed
        for (int i = 0; i < 5; i++) {
            renderFrame([](SDL_Renderer* renderer) {
                // Just clear to show a black window
                SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
                SDL_RenderClear(renderer);
            });
            SDL_Delay(100);
        }

        // Process any pending events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Just drain the event queue
        }

        // Additional delay to ensure window is ready
        SDL_Delay(static_cast<Uint32>(config_.setupDelay.count()));

        // Display window position for debugging
        int x, y;
        SDL_GetWindowPosition(window_, &x, &y);
        printf("Window position: (%d, %d)\n", x, y);
    }

    // Handle all pending SDL events
    void handleEvents(bool& running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            // Additional event handlers can be attached via callback
            for (const auto& handler : eventHandlers_) {
                handler(event);
            }
        }
    }

    // Render a frame with custom rendering logic
    void renderFrame(const std::function<void(SDL_Renderer*)>& renderFunction) {
        // Clear screen
        SDL_SetRenderDrawColor(renderer_, 40, 40, 40, 255);
        SDL_RenderClear(renderer_);

        // Call custom render function
        renderFunction(renderer_);

        // Present renderer
        SDL_RenderPresent(renderer_);
    }

    // Add event handler
    void addEventHandler(std::function<void(const SDL_Event&)> handler) {
        eventHandlers_.push_back(std::move(handler));
    }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool initialized_ = false;
    TestConfig config_;
    std::vector<std::function<void(const SDL_Event&)>> eventHandlers_;
};

} // namespace RobotTest
