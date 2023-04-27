# Robot CPP

![master](https://github.com/developer239/robot-cpp/actions/workflows/ci.yml/badge.svg)

This library is inspired by older unmaintained libraries like [octalmage/robotjs](https://github.com/octalmage/robotjs)
and [Robot/robot-js](https://github.com/Robot/robot-js). The goal is to provide cross-platform controls for various
devices such as keyboard, mouse, and screen for C++ applications.

**Supported system:**

- MacOS 
- Windows **(not tested yet)**

In case of Linux, please, create issue and leave a star and I will implement support. Right now I want to focus on port to
Node.js using Node-API.

## Installation

Add this library as submodule:

```git
$ git submodule add https://github.com/developer239/robot-cpp externals/robot-cpp 
```

Update your CMake:

```CMake
add_subdirectory(externals/robot-cpp)
target_link_libraries(<your_target> PRIVATE RobotCPP)
```

## Types

### Point

`Point` is a structure that represents a 2D point with integer coordinates (x, y). It also provides a method to
calculate the distance between two points.

#### Attributes

- `int x;`
  The x-coordinate of the point.

- `int y;`
  The y-coordinate of the point.

#### Methods

- `double Distance(Point target) const;`
  Calculates and returns the Euclidean distance between the current point and the specified `target` point.

### Example Usage

```cpp
#include "robot.h"

int main() {
  Robot::Point p1{100, 200};
  Robot::Point p2{300, 400};
  double distance = p1.Distance(p2);
  std::cout << "Distance between p1 and p2: " << distance << std::endl;
}
```

## Mouse Class

The `Mouse` class provides a static interface for controlling the mouse cursor, simulating mouse clicks, and scrolling.

### Public Methods

- `static void Move(Robot::Point point);`
  Moves the mouse cursor to the specified point (x, y).

- `static void MoveSmooth(Robot::Point point, double speed = 1500);`
  Moves the mouse cursor smoothly to the specified point (x, y) at the given speed.

- `static void Drag(Robot::Point point, double speed = 1500);`
  Drags the mouse cursor to the specified point (x, y) at the given speed.

- `static Robot::Point GetPosition();`
  Returns the current position of the mouse cursor as a `Robot::Point`.

- `static void ToggleButton(bool down, MouseButton button, bool doubleClick = false);`
  Presses or releases the specified mouse button depending on the `down` argument. If `doubleClick` is set to true, it
  will perform a double click.

- `static void Click(MouseButton button);`
  Simulates a single click using the specified mouse button.

- `static void DoubleClick(MouseButton button);`
  Simulates a double click using the specified mouse button.

- `static void ScrollBy(int y, int x = 0);`
  Scrolls the mouse wheel by the specified x and y distances.

### Example Usage

```cpp
#include "robot.h"

int main() {
  Robot::Mouse::MoveSmooth({100, 200});
}
```

## Keyboard Class

The `Keyboard` class provides a static interface for simulating keyboard key presses, releases, and typing.

### Public Methods

- `static void Type(const std::string& query);`
  Types the given text as a string.

- `static void TypeHumanLike(const std::string& query);`
  Types the given text as a string with a human-like typing speed.

- `static void Click(char asciiChar);`
  Simulates a key press and release for the specified ASCII character.

- `static void Click(SpecialKey specialKey);`
  Simulates a key press and release for the specified special key.

- `static void Press(char asciiChar);`
  Simulates a key press for the specified ASCII character.

- `static void Press(SpecialKey specialKey);`
  Simulates a key press for the specified special key.

- `static void Release(char asciiChar);`
  Simulates a key release for the specified ASCII character.

- `static void Release(SpecialKey specialKey);`
  Simulates a key release for the specified special key.

### Example Usage

```cpp
#include "robot.h"

int main() {
  Robot::Keyboard::TypeHumanLike("Hello, World");
}
```

## Screen Class

The `Screen` class provides functionality to capture the screen, get pixel colors, and save the captured screen as a PNG image.

### Public Methods

- `Pixel GetPixelColor(int x, int y);`
  Returns the color of the pixel at the specified (x, y) coordinates as a `Pixel` structure.

- `DisplaySize GetScreenSize();`
  Returns the size of the screen as a `DisplaySize` structure containing the width and height.

- `void Capture(int x = 0, int y = 0, int width = -1, int height = -1);`
  Captures a rectangular area of the screen defined by the specified (x, y) coordinates and dimensions (width, height).

- `std::vector<Pixel> GetPixels() const;`
  Returns a vector of `Pixel` structures representing the captured screen.

- `void SaveAsPNG(const std::string &filename);`
  Saves the captured screen as a PNG image with the specified filename.

### Structures

#### DisplaySize

`DisplaySize` is a structure that represents the size of a display with integer dimensions (width, height).

##### Attributes

- `int width;`
  The width of the display.

- `int height;`
  The height of the display.

#### Pixel

`Pixel` is a structure that represents the color of a pixel with unsigned char values for red, green, and blue channels.

##### Attributes

- `unsigned char r;`
  The red channel value of the pixel.

- `unsigned char g;`
  The green channel value of the pixel.

- `unsigned char b;`
  The blue channel value of the pixel.

### Example Usage

```cpp
#include "robot.h"

int main() {
  Robot::Screen screen;
  screen.Capture(0, 0, 800, 600);
  Robot::Pixel pixel = screen.GetPixelColor(100, 200);
  screen.SaveAsPNG("screenshot.png");
}
```
