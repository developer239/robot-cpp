#include <gtest/gtest.h>
#include "RobotSDLTestFixture.h"
#include <chrono>
#include <thread>
#include "../../src/Utils.h"

namespace RobotTest {

class MouseTest : public RobotSDLTest {
protected:
    SDL_Point GetElementCenter(const TestElement* element) const {
        SDL_Rect rect = element->getRect();
        return {rect.x + rect.w / 2, rect.y + rect.h / 2};
    }

    void ExpectPositionNear(const SDL_Rect& actual, int expectedX, int expectedY, int tolerance = 0) {
        if (tolerance == 0) {
            tolerance = config_->positionTolerance;
        }

        EXPECT_NEAR(actual.x, expectedX, tolerance)
            << "Element X position should be near expected position";

        EXPECT_NEAR(actual.y, expectedY, tolerance)
            << "Element Y position should be near expected position";
    }
};

TEST_F(MouseTest, CanDragElementSmoothly) {
    // Create drag element specifically for this test
    auto dragElement = createDragElement(
        100, 200, 100, 100, Color::Yellow(), "Drag Test Element"
    );

    // Set up drag event handlers for this test
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

    const SDL_Rect initialRect = dragElement->getRect();
    SDL_Point startPoint = GetElementCenter(dragElement);
    SDL_Point endPoint = {
        startPoint.x + 50,
        startPoint.y + 30
    };

    std::cout << "Starting smooth mouse drag test" << std::endl;
    std::cout << "  Initial element position: ("
              << initialRect.x << ", " << initialRect.y << ")" << std::endl;

    processEventsFor(std::chrono::milliseconds(500));

    Robot::Point startPos = windowToScreen(startPoint.x, startPoint.y);
    Robot::Point endPos = windowToScreen(endPoint.x, endPoint.y);

    Robot::Mouse::MoveSmooth(startPos);
    Robot::Mouse::ToggleButton(true, Robot::MouseButton::LEFT_BUTTON);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Robot::Mouse::MoveSmooth(endPos);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    Robot::Mouse::ToggleButton(false, Robot::MouseButton::LEFT_BUTTON);

    processEventsFor(std::chrono::milliseconds(1000));

    const SDL_Rect finalRect = dragElement->getRect();
    const int expectedX = initialRect.x + 50;
    const int expectedY = initialRect.y + 30;

    ExpectPositionNear(finalRect, expectedX, expectedY);
}

TEST_F(MouseTest, CanMoveAndClickAtPosition) {
    auto clickButton = createTestButton(
        300, 150, 120, 60, Color::Blue(), "Click Test Button"
    );

    SDL_Point buttonCenter = {
        clickButton->getRect().x + clickButton->getRect().w / 2,
        clickButton->getRect().y + clickButton->getRect().h / 2
    };

    std::cout << "Starting mouse move and click test" << std::endl;
    std::cout << "  Button position: ("
              << clickButton->getRect().x << ", " << clickButton->getRect().y << ")" << std::endl;
    std::cout << "  Button center: ("
              << buttonCenter.x << ", " << buttonCenter.y << ")" << std::endl;

    processEventsFor(std::chrono::milliseconds(500));

    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);

    processEventsFor(std::chrono::milliseconds(500));
    Robot::Mouse::Click(Robot::MouseButton::LEFT_BUTTON);
    processEventsFor(std::chrono::milliseconds(500));

    EXPECT_TRUE(clickButton->wasClicked()) << "Button should have been clicked";
}

TEST_F(MouseTest, CanPerformPrecisionMovements) {
    std::vector<SDL_Point> targetPoints = {
        {50, 50},      // Top-left
        {700, 50},     // Top-right
        {50, 500},     // Bottom-left
        {700, 500},    // Bottom-right
        {400, 300}     // Center
    };

    std::cout << "Starting precision mouse movement test" << std::endl;

    for (const auto& point : targetPoints) {
        Robot::Point targetPos = windowToScreen(point.x, point.y);
        Robot::Mouse::Move(targetPos);

        processEventsFor(std::chrono::milliseconds(300));

        Robot::Point currentPos = Robot::Mouse::GetPosition();

        int windowX, windowY;
        SDL_GetWindowPosition(context_->getWindow(), &windowX, &windowY);
        int localX = currentPos.x - windowX;
        int localY = currentPos.y - windowY;

        std::cout << "  Target: (" << point.x << ", " << point.y
                  << "), Actual: (" << localX << ", " << localY << ")" << std::endl;

        EXPECT_NEAR(localX, point.x, config_->positionTolerance)
            << "Mouse X position should be near target";
        EXPECT_NEAR(localY, point.y, config_->positionTolerance)
            << "Mouse Y position should be near target";
    }
}

TEST_F(MouseTest, CanPerformDoubleClick) {
    auto doubleClickButton = createDoubleClickButton(
        200, 300, 150, 80, Color::Green(), "Double-Click Button"
    );

    SDL_Point buttonCenter = {
        doubleClickButton->getRect().x + doubleClickButton->getRect().w / 2,
        doubleClickButton->getRect().y + doubleClickButton->getRect().h / 2
    };

    std::cout << "Starting mouse double-click test" << std::endl;
    processEventsFor(std::chrono::milliseconds(500));

    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);
    processEventsFor(std::chrono::milliseconds(500));

    Robot::Mouse::DoubleClick(Robot::MouseButton::LEFT_BUTTON);
    processEventsFor(std::chrono::milliseconds(500));

    EXPECT_TRUE(doubleClickButton->wasDoubleClicked())
        << "Button should have registered a double-click";
}

TEST_F(MouseTest, CanPerformRightClick) {
    auto rightClickButton = createRightClickButton(
        450, 250, 140, 70, Color::Orange(), "Right-Click Button"
    );

    SDL_Point buttonCenter = {
        rightClickButton->getRect().x + rightClickButton->getRect().w / 2,
        rightClickButton->getRect().y + rightClickButton->getRect().h / 2
    };

    std::cout << "Starting mouse right-click test" << std::endl;
    processEventsFor(std::chrono::milliseconds(500));

    Robot::Point targetPos = windowToScreen(buttonCenter.x, buttonCenter.y);
    Robot::Mouse::Move(targetPos);
    processEventsFor(std::chrono::milliseconds(500));

    Robot::Mouse::Click(Robot::MouseButton::RIGHT_BUTTON);
    processEventsFor(std::chrono::milliseconds(500));

    EXPECT_TRUE(rightClickButton->wasRightClicked())
        << "Button should have registered a right-click";
}

TEST_F(MouseTest, CanPerformScroll) {
    auto scrollArea = createScrollArea(
        300, 200, 200, 150, Color::White(), "Scroll Test Area"
    );

    SDL_Point areaCenter = {
        scrollArea->getRect().x + scrollArea->getRect().w / 2,
        scrollArea->getRect().y + scrollArea->getRect().h / 2
    };

    bool wheelEventReceived = false;

    context_->addEventHandler([&wheelEventReceived, scrollArea](const SDL_Event& event) {
        if (event.type == SDL_MOUSEWHEEL) {
            wheelEventReceived = true;
            std::cout << "  SDL wheel event detected! Amount: " << event.wheel.y << std::endl;

            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            std::cout << "  Mouse position during wheel event: (" << mouseX << ", " << mouseY << ")" << std::endl;

            if (scrollArea->isInside(mouseX, mouseY)) {
                std::cout << "  Mouse is inside scroll area" << std::endl;
            } else {
                std::cout << "  Mouse is outside scroll area" << std::endl;
            }
        }
    });

    int initialScrollY = scrollArea->getScrollY();

    std::cout << "Starting mouse scroll test" << std::endl;
    std::cout << "  Initial scroll position: " << initialScrollY << std::endl;

    processEventsFor(std::chrono::milliseconds(500));

    Robot::Point targetPos = windowToScreen(areaCenter.x, areaCenter.y);
    Robot::Mouse::Move(targetPos);
    processEventsFor(std::chrono::milliseconds(500));

    std::cout << "  Performing Robot::Mouse::ScrollBy(20)" << std::endl;
    Robot::Mouse::ScrollBy(20);
    processEventsFor(std::chrono::milliseconds(1000));

    EXPECT_TRUE(wheelEventReceived)
        << "Robot::Mouse::ScrollBy should generate a wheel event captured by SDL";

    int newScrollY = scrollArea->getScrollY();
    std::cout << "  New scroll position after scrolling: " << newScrollY << std::endl;

    if (wheelEventReceived) {
        EXPECT_GT(newScrollY, initialScrollY)
            << "When wheel events are captured, scroll position should increase";
    }
}

} // namespace RobotTest
