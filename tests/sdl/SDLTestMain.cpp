#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <SDL.h>

#include "TestConfig.h"

/**
 * Custom main function for SDL tests
 *
 * This main function provides additional functionality:
 * - Handles custom command-line arguments before passing control to Google Test
 * - Adds a delay before tests to ensure window is ready
 * - Provides special handling for interactive mode
 */
int main(int argc, char** argv) {
    // Check for interactive mode flag
    bool interactiveMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--interactive") {
            interactiveMode = true;
            // Replace with gtest filter to run only the interactive test
            argv[i] = const_cast<char*>("--gtest_filter=*InteractiveMode");
            break;
        }
        else if (std::string(argv[i]) == "--run-tests") {
            // Replace with gtest filter to exclude interactive tests
            argv[i] = const_cast<char*>("--gtest_filter=-*InteractiveMode");
        }
    }

    // Parse wait time
    int waitTime = 2000; // Default 2 seconds
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--wait-time" && i + 1 < argc) {
            waitTime = std::stoi(argv[i + 1]);
            // Remove these args as they're not for gtest
            for (int j = i; j < argc - 2; ++j) {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
            break;
        }
    }

    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);

    // Add a brief message
    if (interactiveMode) {
        std::cout << "Running in interactive mode..." << std::endl;
    } else {
        std::cout << "Running automated tests..." << std::endl;
        std::cout << "Waiting " << waitTime / 1000.0 << " seconds before starting tests..." << std::endl;
    }

    // Wait for a moment to ensure the window is ready
    SDL_Delay(static_cast<Uint32>(waitTime));

    // Run the tests
    return RUN_ALL_TESTS();
}
