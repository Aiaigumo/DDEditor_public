#pragma once

#include <filesystem>
#include <string>

#include "Beatmap.h"
#include "BeatmapInfo.h"
#include "BeatmapStack.h"
#include "Editor.h"
#include "EditorContext.h"
#include "InputState.h"
#include "Renderer.h"
#include "ScreenResult.h"

// Draws the current difficulty editor screen and owns UI-local editor screen state.
class DifficultyEditorScreen {
public:
	DifficultyEditorScreen(
		EditorContext& editorContext,
		Beatmap& beatmap,
		BeatmapInfo& bmInfo,
		BeatmapStack& beatmapStack,
		Editor& editor,
		InputState& inputState,
		Renderer& perspectiveRenderer,
		Renderer& orthoRenderer
	);

	ScreenResult runFrame();
	void onEnterScreen();
	void onExitScreen();
	bool hasUnsavedChanges() const;
	void requestCloseConfirmation();

private:
	EditorContext& editorContext;
	Beatmap& beatmap;
	BeatmapInfo& bmInfo;
	BeatmapStack& beatmapStack;
	Editor& editor;
	InputState& inputState;
	Renderer& perspectiveRenderer;
	Renderer& orthoRenderer;

	bool dirty = false;
	bool closeConfirmationRequested = false;
	std::string saveStatusMessage;
	std::string addTimingPolicyMessage;
	std::string bpmModeConsistencyMessage;
	std::string bpmValueConsistencyMessage;
	bool canEditInViewer3d = false;
	bool canEditInViewer2d = false;

	void syncPlaybackAndTiming();
	void markDifficultyDirty();
	void clearDifficultyDirty();
	bool tryGetCurrentDifficultyPath(std::filesystem::path& path) const;
	bool saveCurrentDifficulty();
	bool isProjectSingleBpmMode() const;
	const char* getProjectBpmModeText() const;
	const char* getDifficultyBpmModeText() const;
	bool isDifficultyBpmModeConsistentWithProject() const;
	void refreshBpmConsistencyMessages();
	bool canAcceptViewerEditInput() const;
	void drawCloseConfirmationModal(ScreenResult& result);
};
