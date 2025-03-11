#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <SDL.h>

#include "TestConfig.h"

int main(int argc, char** argv) {
    // Parse wait time
    int waitTime = 2000;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--wait-time" && i + 1 < argc) {
            waitTime = std::stoi(argv[i + 1]);
            for (int j = i; j < argc - 2; ++j) {
                argv[j] = argv[j + 2];
            }
            argc -= 2;
            break;
        }
        else if (std::string(argv[i]) == "--run-tests") {
            // Replace with gtest filter to exclude any specific tests if needed
            argv[i] = const_cast<char*>("--gtest_filter=*");
        }
    }

    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "Running automated tests..." << std::endl;
    std::cout << "Waiting " << waitTime / 1000.0 << " seconds before starting tests..." << std::endl;

    SDL_Delay(static_cast<Uint32>(waitTime));
    return RUN_ALL_TESTS();
}
