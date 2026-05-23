#pragma once

#include "AppScreen.h"
#include "Beatmap.h"
#include "BeatmapInfo.h"
#include "BeatmapInfoScreen.h"
#include "BeatmapStack.h"
#include "DifficultyEditorScreen.h"
#include "Editor.h"
#include "EditorContext.h"
#include "InputState.h"
#include "Renderer.h"

// Owns shared application objects and dispatches the active screen each frame.
class Application {
public:
	explicit Application(void* nativeWindowHandle = nullptr);

	void updateFrame(float currentTime);
	void runCurrentScreenFrame();
	void requestClose();
	bool shouldClose() const;

private:
	void initializeMetronome();
	void initializeHitsounds();
	void applyScreenResult(const ScreenResult& result);
	void transitionToScreen(AppScreen nextScreen);
	void closeApplication();
	void runScreenEnterHook(AppScreen screen);
	void runScreenExitHook(AppScreen screen);

	AppScreen currentScreen = AppScreen::BeatmapInfo;

	EditorContext editorContext;
	Renderer perspectiveRenderer;
	Renderer orthoRenderer;
	Beatmap beatmap;
	Editor editor;
	BeatmapStack beatmapStack;
	BeatmapInfo bmInfo;
	InputState inputState;
	void* nativeWindowHandle = nullptr;
	bool closeRequested = false;

	BeatmapInfoScreen beatmapInfoScreen;
	DifficultyEditorScreen difficultyEditorScreen;
};
