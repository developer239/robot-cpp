#pragma once

#include <SDL.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <string>

#include "TestElements.h"
#include "../../src/Keyboard.h"
#include "../../src/Utils.h"
#include "../../src/Mouse.h"

namespace RobotTest {

class KeyboardTests {
public:
    KeyboardTests(SDL_Renderer* renderer, SDL_Window* window)
        : renderer(renderer), window(window), activeTextField(nullptr) {

        // Initialize text input fields
        textFields.push_back(TextInput(
            {100, 200, 300, 30},
            "StandardField"
        ));

        textFields.push_back(TextInput(
            {100, 250, 300, 30},
            "HumanLikeField"
        ));

        textFields.push_back(TextInput(
            {100, 300, 300, 30},
            "SpecialKeysField"
        ));
    }

    void draw() {
        // Draw field labels
        // (In a real implementation, we'd use SDL_ttf for text rendering)

        // Draw all text fields
        for (auto& field : textFields) {
            field.draw(renderer);
        }

        // Draw keyboard state indicator
        SDL_Rect stateRect = {450, 200, 30, 30};
        if (capsLockOn) {
            SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        }
        SDL_RenderFillRect(renderer, &stateRect);

        // Draw label for capslock indicator
        SDL_Rect labelRect = {485, 200, 100, 30};
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        SDL_RenderFillRect(renderer, &labelRect);
    }

    void handleEvent(const SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;

                // Deactivate current field
                if (activeTextField) {
                    activeTextField->deactivate();
                    activeTextField = nullptr;
                }

                // Check for clicks on text fields
                for (auto& field : textFields) {
                    if (field.isInside(x, y)) {
                        field.activate();
                        activeTextField = &field;
                    }
                }
            }
        }
        else if (event.type == SDL_KEYDOWN) {
            // Update the CAPS lock state
            if (event.key.keysym.sym == SDLK_CAPSLOCK) {
                capsLockOn = !capsLockOn;
            }

            // Handle key presses for active text field
            if (activeTextField) {
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    activeTextField->removeChar();
                }
                else if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 126) {
                    // ASCII printable characters
                    char c = static_cast<char>(event.key.keysym.sym);
                    activeTextField->addChar(c);
                }
            }
        }
    }

    void reset() {
        for (auto& field : textFields) {
            field.reset();
        }
        activeTextField = nullptr;
        capsLockOn = false;
    }

    // Test basic typing
    bool testBasicTyping() {
        std::cout << "Testing basic typing..." << std::endl;
        reset();

        if (textFields.empty()) {
            std::cout << "No text fields to test" << std::endl;
            return false;
        }

        // Select the first text field
        auto& field = textFields[0];
        SDL_Rect rect = field.getRect();
        Robot::Point center = {rect.x + rect.w/2, rect.y + rect.h/2};

        // Click on the field to activate it
        Robot::Mouse::Move(center);
        Robot::delay(300);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::delay(300);

        // Process events to register the click
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Type test string
        std::string testString = "Hello Robot";
        Robot::Keyboard::Type(testString);
        Robot::delay(500);

        // Process events to register the typing
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify the text was typed
        if (field.getText() != testString) {
            std::cout << "Basic typing test failed. Expected: '" << testString
                      << "', Actual: '" << field.getText() << "'" << std::endl;
            return false;
        }

        std::cout << "Basic typing test passed" << std::endl;
        return true;
    }

    // Test human-like typing
    bool testHumanLikeTyping() {
        std::cout << "Testing human-like typing..." << std::endl;
        reset();

        if (textFields.size() < 2) {
            std::cout << "Not enough text fields to test" << std::endl;
            return false;
        }

        // Select the second text field
        auto& field = textFields[1];
        SDL_Rect rect = field.getRect();
        Robot::Point center = {rect.x + rect.w/2, rect.y + rect.h/2};

        // Click on the field to activate it
        Robot::Mouse::Move(center);
        Robot::delay(300);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::delay(300);

        // Process events to register the click
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Type test string with human-like timing
        std::string testString = "Human typing";
        Robot::Keyboard::TypeHumanLike(testString);
        Robot::delay(1000); // Give more time for human-like typing to complete

        // Process events to register the typing
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify the text was typed
        if (field.getText() != testString) {
            std::cout << "Human-like typing test failed. Expected: '" << testString
                      << "', Actual: '" << field.getText() << "'" << std::endl;
            return false;
        }

        std::cout << "Human-like typing test passed" << std::endl;
        return true;
    }

    // Test special keys
    bool testSpecialKeys() {
        std::cout << "Testing special keys..." << std::endl;
        reset();

        if (textFields.size() < 3) {
            std::cout << "Not enough text fields to test" << std::endl;
            return false;
        }

        // Select the third text field
        auto& field = textFields[2];
        SDL_Rect rect = field.getRect();
        Robot::Point center = {rect.x + rect.w/2, rect.y + rect.h/2};

        // Click on the field to activate it
        Robot::Mouse::Move(center);
        Robot::delay(300);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::delay(300);

        // Process events to register the click
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Type some text first
        Robot::Keyboard::Type("test");
        Robot::delay(300);

        // Process events
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Test backspace key
        Robot::Keyboard::Click(Robot::Keyboard::BACKSPACE);
        Robot::delay(300);

        // Process events
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify backspace worked
        if (field.getText() != "tes") {
            std::cout << "Backspace test failed. Expected: 'tes', Actual: '"
                      << field.getText() << "'" << std::endl;
            return false;
        }

        // Test other special keys like ENTER
        Robot::Keyboard::Click(Robot::Keyboard::ENTER);
        Robot::delay(300);

        // Process events
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        std::cout << "Special keys test passed" << std::endl;
        return true;
    }

    // Test modifier keys
    bool testModifierKeys() {
        std::cout << "Testing modifier keys..." << std::endl;
        reset();

        if (textFields.empty()) {
            std::cout << "No text fields to test" << std::endl;
            return false;
        }

        // Select the first text field
        auto& field = textFields[0];
        SDL_Rect rect = field.getRect();
        Robot::Point center = {rect.x + rect.w/2, rect.y + rect.h/2};

        // Click on the field to activate it
        Robot::Mouse::Move(center);
        Robot::delay(300);
        Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
        Robot::delay(300);

        // Process events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Test SHIFT + a (should produce 'A')
        Robot::Keyboard::HoldStart(Robot::Keyboard::SHIFT);
        Robot::delay(300);
        Robot::Keyboard::Click('a');
        Robot::delay(300);
        Robot::Keyboard::HoldStop(Robot::Keyboard::SHIFT);
        Robot::delay(300);

        // Process events
        while (SDL_PollEvent(&event)) {
            handleEvent(event);
        }

        // Verify uppercase 'A' was typed
        // Note: This test may be platform-dependent as some OS handle shift differently
        if (field.getText().empty() || field.getText()[0] != 'A') {
            std::cout << "Shift modifier test failed. Expected: 'A', Actual: '"
                      << (field.getText().empty() ? "" : std::string(1, field.getText()[0]))
                      << "'" << std::endl;
            return false;
        }

        std::cout << "Modifier keys test passed" << std::endl;
        return true;
    }

    bool runAllTests() {
        bool allPassed = true;

        // Run all keyboard tests
        allPassed &= testBasicTyping();
        allPassed &= testHumanLikeTyping();
        allPassed &= testSpecialKeys();
        allPassed &= testModifierKeys();

        return allPassed;
    }

private:
    SDL_Renderer* renderer;
    SDL_Window* window;

    std::vector<TextInput> textFields;
    TextInput* activeTextField;

    bool capsLockOn = false;
};

} // namespace RobotTest
