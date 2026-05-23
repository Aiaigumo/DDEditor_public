#pragma once

#include "AppScreen.h"

// Reports screen-level actions requested during a frame.
struct ScreenResult {
	bool hasScreenChangeRequest = false;
	AppScreen nextScreen = AppScreen::DifficultyEditor;
	bool hasApplicationCloseRequest = false;
};
