#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <optional>

namespace RobotTest {

    /**
     * @brief Configuration for tests with default values
     */
    struct TestConfig {
        // Window settings
        int windowWidth = 800;
        int windowHeight = 600;
        std::string windowTitle = "Robot CPP Testing Framework";

        // Test execution settings
        bool runTests = false;
        std::chrono::milliseconds initialWaitTime{2000};
        std::chrono::seconds testTimeout{30};

        // Delay settings for animation and visualization
        std::chrono::milliseconds frameDelay{16};  // ~60 FPS
        std::chrono::milliseconds setupDelay{500};
        std::chrono::milliseconds actionDelay{300};

        // Window positioning
        int windowX = 50;
        int windowY = 50;

        // Mouse test settings
        int dragOffsetX = 100;
        int dragOffsetY = 50;
        int positionTolerance = 20;  // Pixels

        // Parse command line arguments
        static TestConfig fromCommandLine(int argc, char** argv) {
            TestConfig config;

            for (int i = 1; i < argc; i++) {
                std::string arg = argv[i];

                if (arg == "--run-tests") {
                    config.runTests = true;
                }
                else if (arg == "--wait-time" && i + 1 < argc) {
                    config.initialWaitTime = std::chrono::milliseconds(std::stoi(argv[i + 1]));
                    i++;
                }
            }

            return config;
        }
    };

} // namespace RobotTest
