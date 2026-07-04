#pragma once

// Umbrella header: pulls in the entire public API. Include individual headers
// instead when you want to keep translation-unit dependencies tight.
//
// Nothing here transitively includes an OS header - the platform boundary lives
// entirely behind the backend interfaces, which this public surface only
// forward-declares.

#include "robot/Capabilities.h"
#include "robot/Error.h"
#include "robot/Event.h"
#include "robot/EventTap.h"
#include "robot/Geometry.h"
#include "robot/Image.h"
#include "robot/Key.h"
#include "robot/Keyboard.h"
#include "robot/Modifiers.h"
#include "robot/Monitor.h"
#include "robot/Mouse.h"
#include "robot/MouseButton.h"
#include "robot/Recorder.h"
#include "robot/Scroll.h"
#include "robot/Screen.h"
#include "robot/Session.h"