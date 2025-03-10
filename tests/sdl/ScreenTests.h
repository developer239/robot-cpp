#pragma once

#include <SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <cmath>
#include <filesystem>

#include "TestElements.h"
#include "../../src/Screen.h"
#include "../../src/Utils.h"

namespace RobotTest {

class ScreenTests {
public:
    ScreenTests(SDL_Renderer* renderer, SDL_Window* window)
        : renderer(renderer), window(window) {

        // Initialize color areas for pixel testing
        colorAreas.push_back(ColorArea(
            {100, 400, 100, 100},
            {255, 0, 0, 255},
            "RedArea"
        ));

        colorAreas.push_back(ColorArea(
            {250, 400, 100, 100},
            {0, 255, 0, 255},
            "GreenArea"
        ));

        colorAreas.push_back(ColorArea(
            {400, 400, 100, 100},
            {0, 0, 255, 255},
            "BlueArea"
        ));

        // Create test pattern
        createTestPattern();
    }

    void draw() {
        // Draw all color areas
        for (auto& area : colorAreas) {
            area.draw(renderer);
        }

        // Draw test pattern
        drawTestPattern();
    }

    void handleEvent(const SDL_Event& event) {
        // No event handling needed for screen tests
    }

    void reset() {
        // No reset needed for screen tests
    }

    // Test pixel color reading
    bool testPixelColors() {
        std::cout << "Testing pixel color reading..." << std::endl;

        if (colorAreas.empty()) {
            std::cout << "No color areas to test" << std::endl;
            return false;
        }

        // Make sure we render before capturing
        SDL_RenderPresent(renderer);
        Robot::delay(300);

        // Create screen capture object
        Robot::Screen screen;

        // Test each color area
        for (const auto& area : colorAreas) {
            SDL_Rect rect = area.getRect();
            SDL_Color expectedColor = area.getColor();

            // Capture center of the area
            int x = rect.x + rect.w/2;
            int y = rect.y + rect.h/2;

            screen.Capture();
            Robot::Pixel pixel = screen.GetPixelColor(x, y);

            // Check if color matches (with tolerance due to rendering differences)
            const int tolerance = 30; // Relatively high tolerance because of window rendering
            if (abs(pixel.r - expectedColor.r) > tolerance ||
                abs(pixel.g - expectedColor.g) > tolerance ||
                abs(pixel.b - expectedColor.b) > tolerance) {
                std::cout << "Pixel color test failed for " << area.getName() << endl;
                std::cout << "Expected: RGB(" << (int)expectedColor.r << ", "
                          << (int)expectedColor.g << ", " << (int)expectedColor.b << ")" << std::endl;
                std::cout << "Actual: RGB(" << (int)pixel.r << ", "
                          << (int)pixel.g << ", " << (int)pixel.b << ")" << std::endl;
                return false;
            }
        }

        std::cout << "Pixel color test passed" << std::endl;
        return true;
    }

    // Test screen capture
    bool testScreenCapture() {
        std::cout << "Testing screen capture..." << std::endl;

        // Make sure we render before capturing
        SDL_RenderPresent(renderer);
        Robot::delay(300);

        // Get window position and size
        int windowX, windowY, windowWidth, windowHeight;
        SDL_GetWindowPosition(window, &windowX, &windowY);
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        // Create screen capture object
        Robot::Screen screen;

        // Capture full window
        screen.Capture(windowX, windowY, windowWidth, windowHeight);

        // Save screenshot
        std::string filename = "test_capture_full.png";
        screen.SaveAsPNG(filename);

        // Check if file was created
        if (!std::filesystem::exists(filename)) {
            std::cout << "Screen capture test failed - could not save PNG file" << std::endl;
            return false;
        }

        // Capture just the test pattern area
        if (!testPatternRect.has_value()) {
            std::cout << "No test pattern area defined" << std::endl;
            return false;
        }

        SDL_Rect patternRect = testPatternRect.value();
        screen.Capture(windowX + patternRect.x, windowY + patternRect.y,
                      patternRect.w, patternRect.h);

        // Save pattern screenshot
        std::string patternFilename = "test_capture_pattern.png";
        screen.SaveAsPNG(patternFilename);

        // Check if file was created
        if (!std::filesystem::exists(patternFilename)) {
            std::cout << "Pattern capture test failed - could not save PNG file" << std::endl;
            return false;
        }

        std::cout << "Screen capture test passed" << std::endl;
        return true;
    }

    // Test screen size
    bool testScreenSize() {
        std::cout << "Testing screen size retrieval..." << std::endl;

        Robot::Screen screen;
        Robot::DisplaySize size = screen.GetScreenSize();

        // Display sizes should be non-zero
        if (size.width <= 0 || size.height <= 0) {
            std::cout << "Screen size test failed. Got width=" << size.width
                      << ", height=" << size.height << std::endl;
            return false;
        }

        std::cout << "Screen size: " << size.width << "x" << size.height << std::endl;
        std::cout << "Screen size test passed" << std::endl;
        return true;
    }

    bool runAllTests() {
        bool allPassed = true;

        // Run all screen tests
        allPassed &= testPixelColors();
        allPassed &= testScreenCapture();
        allPassed &= testScreenSize();

        return allPassed;
    }

private:
    SDL_Renderer* renderer;
    SDL_Window* window;

    std::vector<ColorArea> colorAreas;
    std::optional<SDL_Rect> testPatternRect;
    std::vector<SDL_Rect> patternRects;

    void createTestPattern() {
        // Create a checkered pattern for testing screen capture
        testPatternRect = SDL_Rect{550, 400, 120, 120};

        const int squareSize = 20;
        for (int y = 0; y < 6; y++) {
            for (int x = 0; x < 6; x++) {
                patternRects.push_back(SDL_Rect{
                    testPatternRect->x + x * squareSize,
                    testPatternRect->y + y * squareSize,
                    squareSize,
                    squareSize
                });
            }
        }
    }

    void drawTestPattern() {
        if (!testPatternRect.has_value()) return;

        // Draw pattern background
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &testPatternRect.value());

        // Draw checkered pattern
        for (size_t i = 0; i < patternRects.size(); i++) {
            // Alternate colors
            if ((i / 6 + i % 6) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            SDL_RenderFillRect(renderer, &patternRects[i]);
        }
    }
};

} // namespace RobotTest
