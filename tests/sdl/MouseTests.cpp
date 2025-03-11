#include <gtest/gtest.h>
#include "RobotSDLTestFixture.h"

namespace RobotTest {

// Test fixture for Mouse functionality tests
class MouseDragTest : public RobotSDLTest {};

/**
 * Test that verifies mouse drag functionality
 *
 * This test creates a draggable element, drags it to a new position,
 * and verifies that it moved to the expected location.
 */
TEST_F(MouseDragTest, CanDragElementToNewPosition) {
    // Create a draggable element
    DragElement* dragElement = createDragElement(
        100, 200,     // x, y
        100, 100,     // width, height
        Color::Yellow(),
        "Drag Test Element"
    );

    // Initial position of the element
    const SDL_Rect initialRect = dragElement->getRect();

    // Register mouse event handler
    context_->addEventHandler([dragElement](const SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                int x = event.button.x;
                int y = event.button.y;
                if (dragElement->isInside(x, y)) {
                    dragElement->startDrag();
                }
            }
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            if (event.button.button == SDL_BUTTON_LEFT) {
                dragElement->stopDrag();
            }
        } else if (event.type == SDL_MOUSEMOTION) {
            int x = event.motion.x;
            int y = event.motion.y;
            if (dragElement->isDragging()) {
                dragElement->moveTo(x, y);
            }
        }
    });

    // Define start and end points for the drag operation (in window coordinates)
    SDL_Point startPoint = {
        initialRect.x + initialRect.w / 2,
        initialRect.y + initialRect.h / 2
    };

    SDL_Point endPoint = {
        startPoint.x + config_->dragOffsetX,
        startPoint.y + config_->dragOffsetY
    };

    // Log test information
    std::cout << "Starting mouse drag test" << std::endl;
    std::cout << "  Initial element position: ("
              << initialRect.x << ", " << initialRect.y << ")" << std::endl;
    std::cout << "  Drag start point: ("
              << startPoint.x << ", " << startPoint.y << ")" << std::endl;
    std::cout << "  Drag end point: ("
              << endPoint.x << ", " << endPoint.y << ")" << std::endl;

    // Perform the drag operation
    performMouseDrag(startPoint, endPoint);

    // Get the element's position after the drag
    const SDL_Rect finalRect = dragElement->getRect();
    std::cout << "  Final element position: ("
              << finalRect.x << ", " << finalRect.y << ")" << std::endl;

    // Calculate expected position
    const int expectedX = initialRect.x + config_->dragOffsetX;
    const int expectedY = initialRect.y + config_->dragOffsetY;
    std::cout << "  Expected position: ("
              << expectedX << ", " << expectedY << ")" << std::endl;

    // Verify the element moved to the expected position (within tolerance)
    const int tolerance = config_->positionTolerance;

    EXPECT_NEAR(finalRect.x, expectedX, tolerance)
        << "Element should be dragged horizontally by " << config_->dragOffsetX << " pixels";

    EXPECT_NEAR(finalRect.y, expectedY, tolerance)
        << "Element should be dragged vertically by " << config_->dragOffsetY << " pixels";
}

/**
 * Test that verifies mouse click functionality
 *
 * This test creates a clickable button element, clicks it,
 * and verifies that it registers the click correctly.
 */
TEST_F(MouseDragTest, DISABLED_InteractiveMode) {
    // Create some interactive elements
    createDragElement(100, 200, 100, 100, Color::Yellow(), "Drag Me");
    createDragElement(250, 200, 100, 100, Color::Orange(), "Also Draggable");

    // Register event handlers for the elements
    context_->addEventHandler([this](const SDL_Event& event) {
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            int x = event.button.x;
            int y = event.button.y;

            for (auto& element : testElements_) {
                if (auto* dragElement = dynamic_cast<DragElement*>(element.get())) {
                    if (dragElement->isInside(x, y)) {
                        dragElement->startDrag();
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
            for (auto& element : testElements_) {
                if (auto* dragElement = dynamic_cast<DragElement*>(element.get())) {
                    if (dragElement->isDragging()) {
                        dragElement->stopDrag();
                    }
                }
            }
        }
        else if (event.type == SDL_MOUSEMOTION) {
            int x = event.motion.x;
            int y = event.motion.y;

            for (auto& element : testElements_) {
                if (auto* dragElement = dynamic_cast<DragElement*>(element.get())) {
                    if (dragElement->isDragging()) {
                        dragElement->moveTo(x, y);
                    }
                }
            }
        }
    });

    // Run interactive mode for 60 seconds or until window is closed
    std::cout << "Running in interactive mode. Close window to exit." << std::endl;
    processEventsFor(std::chrono::seconds(60));

    // This test doesn't actually assert anything since it's interactive
    SUCCEED() << "Interactive mode completed";
}

} // namespace RobotTest
