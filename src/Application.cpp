#include "Application.h"

#include <filesystem>
#include <iostream>
#include <string>

Application::Application(void* nativeWindowHandle) :
	perspectiveRenderer("shaders/vertex.glsl", "shaders/fragment.glsl", true),
	orthoRenderer("shaders/vertex.glsl", "shaders/fragment.glsl", false),
	nativeWindowHandle(nativeWindowHandle),
	beatmapInfoScreen(
		editorContext,
		beatmap,
		bmInfo,
		beatmapStack,
		editor,
		nativeWindowHandle
	),
	difficultyEditorScreen(
		editorContext,
		beatmap,
		bmInfo,
		beatmapStack,
		editor,
		inputState,
		perspectiveRenderer,
		orthoRenderer
	) {
	editor.updateTimingState(editorContext, beatmap);
	initializeMetronome();
	initializeHitsounds();
}

void Application::updateFrame(float currentTime) {
	editorContext.deltaTime = currentTime - editorContext.lastTime;
	editorContext.lastTime = currentTime;

	inputState.updateInput();
}

void Application::runCurrentScreenFrame() {
	ScreenResult result;

	switch (currentScreen) {
	case AppScreen::ProjectSelect:
	case AppScreen::BeatmapInfo:
		result = beatmapInfoScreen.runFrame();
		break;
	case AppScreen::DifficultyEditor:
		result = difficultyEditorScreen.runFrame();
		break;
	}

	applyScreenResult(result);
}

void Application::requestClose() {
	if (closeRequested) {
		return;
	}

	switch (currentScreen) {
	case AppScreen::ProjectSelect:
	case AppScreen::BeatmapInfo:
		if (beatmapInfoScreen.hasUnsavedChanges()) {
			beatmapInfoScreen.requestCloseConfirmation();
		}
		else {
			closeApplication();
		}
		break;
	case AppScreen::DifficultyEditor:
		if (difficultyEditorScreen.hasUnsavedChanges()) {
			difficultyEditorScreen.requestCloseConfirmation();
		}
		else {
			closeApplication();
		}
		break;
	}
}

bool Application::shouldClose() const {
	return closeRequested;
}

void Application::initializeMetronome() {
	std::string clickAudioName = editorContext.soundEffectFolder + "/" + "click";
	for (const auto& filename : editorContext.audioExtensions) {
		std::string fullPath = clickAudioName + filename;
		if (std::filesystem::exists(fullPath)) {
			std::cout << "Loading metronome sound: " << fullPath << std::endl;
			if (editorContext.metronomePlayer.load(fullPath)) {
				break;
			}
		}
	}
	editorContext.metronomePlayer.setVolume(editorContext.metronomeVolume);
	if (!editorContext.metronomePlayer.isLoaded()) {
		std::cerr << "Failed to load metronome sounds.\n";
	}
}

void Application::initializeHitsounds() {
	std::string hitHandAudioName = editorContext.soundEffectFolder + "/" + "clap";
	std::string hitFootAudioName = editorContext.soundEffectFolder + "/" + "snare";

	for (const auto& filename : editorContext.audioExtensions) {
		std::string fullPathHand = hitHandAudioName + filename;
		if (std::filesystem::exists(fullPathHand)) {
			std::cout << "Loading hitsound of hand notes: " << fullPathHand << std::endl;
			if (editorContext.hitHandPlayer.load(fullPathHand)) {
				break;
			}
		}
	}
	editorContext.hitHandPlayer.setVolume(editorContext.hitSoundVolume);
	if (!editorContext.hitHandPlayer.isLoaded()) {
		std::cerr << "Failed to load hitsound of hand notes.\n";
	}

	for (const auto& filename : editorContext.audioExtensions) {
		std::string fullPathFoot = hitFootAudioName + filename;
		if (std::filesystem::exists(fullPathFoot)) {
			std::cout << "Loading hitsound of hand notes: " << fullPathFoot << std::endl;
			if (editorContext.hitFootPlayer.load(fullPathFoot)) {
				break;
			}
		}
	}
	editorContext.hitFootPlayer.setVolume(0.1f);
	if (!editorContext.hitFootPlayer.isLoaded()) {
		std::cerr << "Failed to load hitsound of Foot notes.\n";
	}
}

void Application::applyScreenResult(const ScreenResult& result) {
	if (result.hasApplicationCloseRequest) {
		closeApplication();
		return;
	}

	if (result.hasScreenChangeRequest) {
		transitionToScreen(result.nextScreen);
	}
}

void Application::transitionToScreen(AppScreen nextScreen) {
	if (currentScreen == nextScreen) {
		return;
	}

	runScreenExitHook(currentScreen);
	currentScreen = nextScreen;
	runScreenEnterHook(currentScreen);
}

void Application::closeApplication() {
	if (closeRequested) {
		return;
	}

	runScreenExitHook(currentScreen);
	closeRequested = true;
}

void Application::runScreenEnterHook(AppScreen screen) {
	switch (screen) {
	case AppScreen::ProjectSelect:
	case AppScreen::BeatmapInfo:
		beatmapInfoScreen.onEnterScreen();
		break;
	case AppScreen::DifficultyEditor:
		difficultyEditorScreen.onEnterScreen();
		break;
	}
}

void Application::runScreenExitHook(AppScreen screen) {
	switch (screen) {
	case AppScreen::ProjectSelect:
	case AppScreen::BeatmapInfo:
		beatmapInfoScreen.onExitScreen();
		break;
	case AppScreen::DifficultyEditor:
		difficultyEditorScreen.onExitScreen();
		break;
	}
}
