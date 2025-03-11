#include <gtest/gtest.h>
#include "RobotSDLTestFixture.h"
#include <chrono>
#include <thread>
#include "../../src/Utils.h"

namespace RobotTest {

// Test fixture for Mouse functionality tests
class MouseTest : public RobotSDLTest {
protected:
    void SetUp() override {
        RobotSDLTest::SetUp();

        // Set up standard test elements
        dragElement = createDragElement(
            100, 200,     // x, y
            100, 100,     // width, height
            Color::Yellow(),
            "Drag Test Element"
        );

        // Register mouse event handlers
        context_->addEventHandler([this](const SDL_Event& event) {
            HandleDragElementEvents(event);
        });
    }

    void HandleDragElementEvents(const SDL_Event& event) {
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
    }

    DragElement* dragElement = nullptr;

    // Helper method to get the center point of an element
    SDL_Point GetElementCenter(const TestElement* element) const {
        SDL_Rect rect = element->getRect();
        return {rect.x + rect.w / 2, rect.y + rect.h / 2};
    }

    // Helper method to verify position within tolerance
    void ExpectPositionNear(const SDL_Rect& actual, int expectedX, int expectedY, int tolerance = 0) {
        if (tolerance == 0) {
            tolerance = config_->positionTolerance;
        }

        EXPECT_NEAR(actual.x, expectedX, tolerance)
            << "Element X position should be near expected position";

        EXPECT_NEAR(actual.y, expectedY, tolerance)
            << "Element Y position should be near expected position";
    }

    // Helper methods for creating specialized test elements
    TestButton* createTestButton(
        int x, int y, int width, int height,
        Color color, const std::string& name);

    DoubleClickButton* createDoubleClickButton(
        int x, int y, int width, int height,
        Color color, const std::string& name);

    RightClickButton* createRightClickButton(
        int x, int y, int width, int height,
        Color color, const std::string& name);

    ScrollArea* createScrollArea(
        int x, int y, int width, int height,
        Color color, const std::string& name);
};

/**
 * Test mouse drag functionality
 *
 * Verifies that the mouse can drag an element from one position to another
 */
TEST_F(MouseTest, CanDragElementToNewPosition) {
    // Initial position of the element
    const SDL_Rect initialRect = dragElement->getRect();

    // Calculate drag points
    SDL_Point startPoint = GetElementCenter(dragElement);
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

    // Add extra render cycle before starting the operation
    processEventsFor(std::chrono::milliseconds(500));

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
    ExpectPositionNear(finalRect, expectedX, expectedY);
}

/**
 * Test smooth mouse drag functionality
 *
 * Verifies that the mouse can drag an element smoothly with precision
 */
TEST_F(MouseTest, CanDragElementSmoothly) {
    // Initial position of the element
    const SDL_Rect initialRect = dragElement->getRect();

    // Calculate drag points for a smaller, more precise movement
    SDL_Point startPoint = GetElementCenter(dragElement);
    SDL_Point endPoint = {
        startPoint.x + 50,  // smaller horizontal move
        startPoint.y + 30   // smaller vertical move
    };

    // Log test information
    std::cout << "Starting smooth mouse drag test" << std::endl;
    std::cout << "  Initial element position: ("
              << initialRect.x << ", " << initialRect.y << ")" << std::endl;

    // Process events before test
    processEventsFor(std::chrono::milliseconds(500));

    // Override default drag behavior to use DragSmooth specifically
    Robot::Point startPos = windowToScreen(startPoint.x, startPoint.y);
    Robot::Point endPos = windowToScreen(endPoint.x, endPoint.y);

    // Move to start position and perform smooth drag
    Robot::Mouse::MoveSmooth(startPos);
    Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
    // Use the correct delay function
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Robot::Mouse::MoveSmooth(endPos);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Robot::Mouse::ToggleButton(false, Robot::MouseButton::LEFT_BUTTON);

    // Process events to ensure drag is applied
    processEventsFor(std::chrono::milliseconds(1000));

    // Get the element's position after the drag
    const SDL_Rect finalRect = dragElement->getRect();

    // Calculate expected position
    const int expectedX = initialRect.x + 50;
    const int expectedY = initialRect.y + 30;

    // Verify the element moved to the expected position (within tolerance)
    ExpectPositionNear(finalRect, expectedX, expectedY);
}

/**
 * Test basic mouse movement functionality
 *
 * Verifies that the mouse can move to a specific position and actions are properly triggered
 */
TEST_F(MouseTest, CanMoveAndClickAtPosition) {
    // Create a clickable button for this test
    auto clickButton = createTestButton(
        300, 150,     // x, y
        120, 60,      // width, height
        Color::Blue(),
        "Click Test Button"
    );

    // Calculate the center point of the button
    SDL_Point buttonCenter = {
        clickButton->getRect().x + clickButton->getRect().w / 2,
        clickButton->getRect().y + clickButton->getRect().h / 2
    };

    // Log test information
    std::cout << "Starting mouse move and click test" << std::endl;
    std::cout << "  Button position: ("
              << clickButton->getRect().x << ", " << clickButton->getRect().y << ")" << std::endl;
    std::cout << "  Button center: ("
              << buttonCenter.x << ", " << buttonCenter.y << ")" << std::endl;

    // Process events before test
    processEventsFor(std::chrono::milliseconds(500));

    // Move mouse to button center
    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);

    // Give time for the mouse to arrive
    processEventsFor(std::chrono::milliseconds(500));

    // Click the button
    Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);

    // Process events after click
    processEventsFor(std::chrono::milliseconds(500));

    // Verify button was clicked
    EXPECT_TRUE(clickButton->wasClicked()) << "Button should have been clicked";
}

/**
 * Test mouse precision movements
 *
 * Verifies that the mouse can move precisely to various positions
 */
TEST_F(MouseTest, CanPerformPrecisionMovements) {
    // Set up target points in different regions of the window
    std::vector<SDL_Point> targetPoints = {
        {50, 50},      // Top-left
        {700, 50},     // Top-right
        {50, 500},     // Bottom-left
        {700, 500},    // Bottom-right
        {400, 300}     // Center
    };

    // Log test information
    std::cout << "Starting precision mouse movement test" << std::endl;

    for (const auto& point : targetPoints) {
        // Move mouse to target point
        Robot::Point targetPos = windowToScreen(point.x, point.y);
        Robot::Mouse::Move(targetPos);

        // Give time for the mouse to arrive
        processEventsFor(std::chrono::milliseconds(300));

        // Get actual mouse position
        Robot::Point currentPos = Robot::Mouse::GetPosition();

        // Convert back to window coordinates for comparison
        int windowX, windowY;
        SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
        int localX = currentPos.x - windowX;
        int localY = currentPos.y - windowY;

        // Log positions
        std::cout << "  Target: (" << point.x << ", " << point.y
                  << "), Actual: (" << localX << ", " << localY << ")" << std::endl;

        // Verify position (with tolerance)
        EXPECT_NEAR(localX, point.x, config_->positionTolerance)
            << "Mouse X position should be near target";
        EXPECT_NEAR(localY, point.y, config_->positionTolerance)
            << "Mouse Y position should be near target";
    }
}

/**
 * Test mouse double-click functionality
 *
 * Verifies that the mouse can perform a double-click action
 */
TEST_F(MouseTest, CanPerformDoubleClick) {
    // Create a button that tracks double clicks
    auto doubleClickButton = createDoubleClickButton(
        200, 300,     // x, y
        150, 80,      // width, height
        Color::Green(),
        "Double-Click Button"
    );

    // Calculate the center point of the button
    SDL_Point buttonCenter = {
        doubleClickButton->getRect().x + doubleClickButton->getRect().w / 2,
        doubleClickButton->getRect().y + doubleClickButton->getRect().h / 2
    };

    // Log test information
    std::cout << "Starting mouse double-click test" << std::endl;

    // Process events before test
    processEventsFor(std::chrono::milliseconds(500));

    // Move mouse to button center
    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);

    // Give time for the mouse to arrive
    processEventsFor(std::chrono::milliseconds(500));

    // Perform double-click
    Robot::Mouse::DoubleClick(Robot::MouseButton::LEFT_BUTTON);

    // Process events after double-click
    processEventsFor(std::chrono::milliseconds(500));

    // Verify double-click was registered
    EXPECT_TRUE(doubleClickButton->wasDoubleClicked())
        << "Button should have registered a double-click";
}

/**
 * Test right-click functionality
 *
 * Verifies that the mouse can perform a right-click action
 */
TEST_F(MouseTest, CanPerformRightClick) {
    // Create a button that responds to right clicks
    auto rightClickButton = createRightClickButton(
        450, 250,     // x, y
        140, 70,      // width, height
        Color::Orange(),
        "Right-Click Button"
    );

    // Calculate the center point of the button
    SDL_Point buttonCenter = {
        rightClickButton->getRect().x + rightClickButton->getRect().w / 2,
        rightClickButton->getRect().y + rightClickButton->getRect().h / 2
    };

    // Log test information
    std::cout << "Starting mouse right-click test" << std::endl;

    // Process events before test
    processEventsFor(std::chrono::milliseconds(500));

    // Move mouse to button center
    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);

    // Give time for the mouse to arrive
    processEventsFor(std::chrono::milliseconds(500));

    // Perform right-click
    Robot::Mouse::Click(Robot::MouseButton::RIGHT_BUTTON);

    // Process events after right-click
    processEventsFor(std::chrono::milliseconds(500));

    // Verify right-click was registered
    EXPECT_TRUE(rightClickButton->wasRightClicked())
        << "Button should have registered a right-click";
}

/**
 * Test scroll functionality
 *
 * Verifies that the mouse can perform scroll operations
 */
TEST_F(MouseTest, CanPerformScroll) {
    // Create a scrollable area
    auto scrollArea = createScrollArea(
        300, 200,     // x, y
        200, 150,     // width, height
        Color::White(),
        "Scroll Test Area"
    );

    // Calculate the center point of the scroll area
    SDL_Point areaCenter = {
        scrollArea->getRect().x + scrollArea->getRect().w / 2,
        scrollArea->getRect().y + scrollArea->getRect().h / 2
    };

    // Set up a flag to track if a wheel event was received
    bool wheelEventReceived = false;

    // Add a specialized event monitor to detect wheel events
    context_->addEventHandler([&wheelEventReceived, scrollArea](const SDL_Event& event) {
        if (event.type == SDL_MOUSEWHEEL) {
            wheelEventReceived = true;
            std::cout << "  SDL wheel event detected! Amount: " << event.wheel.y << std::endl;

            // Get mouse position
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            std::cout << "  Mouse position during wheel event: (" << mouseX << ", " << mouseY << ")" << std::endl;

            // Check if mouse is inside the scroll area
            if (scrollArea->isInside(mouseX, mouseY)) {
                std::cout << "  Mouse is inside scroll area" << std::endl;
            } else {
                std::cout << "  Mouse is outside scroll area" << std::endl;
            }
        }
    });

    // Get initial scroll position
    int initialScrollY = scrollArea->getScrollY();

    // Log test information
    std::cout << "Starting mouse scroll test" << std::endl;
    std::cout << "  Initial scroll position: " << initialScrollY << std::endl;

    // Process events before test
    processEventsFor(std::chrono::milliseconds(500));

    // Move mouse to scroll area center
    Robot::Point targetPos = windowToScreen(areaCenter.x, areaCenter.y);
    Robot::Mouse::Move(targetPos);

    // Give time for the mouse to arrive
    processEventsFor(std::chrono::milliseconds(500));

    // Perform vertical scroll down - use a larger amount for better chance of capture
    std::cout << "  Performing Robot::Mouse::ScrollBy(20)" << std::endl;
    Robot::Mouse::ScrollBy(20);

    // Process events with a longer time to ensure we capture the wheel event
    processEventsFor(std::chrono::milliseconds(1000));

    // Verify that a wheel event was received
    EXPECT_TRUE(wheelEventReceived)
        << "Robot::Mouse::ScrollBy should generate a wheel event captured by SDL";

    // Check if scroll position has changed
    int newScrollY = scrollArea->getScrollY();
    std::cout << "  New scroll position after scrolling: " << newScrollY << std::endl;

    // If wheel events are being properly propagated, we should see a scroll position change
    if (wheelEventReceived) {
        EXPECT_GT(newScrollY, initialScrollY)
            << "When wheel events are captured, scroll position should increase";
    }
}

// Helper methods for creating specialized test elements
TestButton* MouseTest::createTestButton(
    int x, int y, int width, int height,
    Color color, const std::string& name)
{
    return RobotSDLTest::createTestButton(x, y, width, height, color, name);
}

DoubleClickButton* MouseTest::createDoubleClickButton(
    int x, int y, int width, int height,
    Color color, const std::string& name)
{
    return RobotSDLTest::createDoubleClickButton(x, y, width, height, color, name);
}

RightClickButton* MouseTest::createRightClickButton(
    int x, int y, int width, int height,
    Color color, const std::string& name)
{
    return RobotSDLTest::createRightClickButton(x, y, width, height, color, name);
}

ScrollArea* MouseTest::createScrollArea(
    int x, int y, int width, int height,
    Color color, const std::string& name)
{
    return RobotSDLTest::createScrollArea(x, y, width, height, color, name);
}

} // namespace RobotTest
