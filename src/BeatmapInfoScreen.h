#pragma once

#include <array>
#include <filesystem>
#include <string>

#include "Beatmap.h"
#include "BeatmapInfo.h"
#include "BeatmapStack.h"
#include "Editor.h"
#include "EditorContext.h"
#include "ScreenResult.h"

// Draws the beatmap folder and BeatmapInfo screen before entering a difficulty editor.
class BeatmapInfoScreen {
public:
	BeatmapInfoScreen(
		EditorContext& editorContext,
		Beatmap& beatmap,
		BeatmapInfo& bmInfo,
		BeatmapStack& beatmapStack,
		Editor& editor,
		void* nativeWindowHandle
	);

	ScreenResult runFrame();
	void onEnterScreen();
	void onExitScreen();
	bool hasUnsavedChanges() const;
	void requestCloseConfirmation();

	enum class ProjectBpmConsistency {
		Unknown,
		Consistent,
		Mismatch
	};

	enum class DifficultyBpmConsistency {
		Missing,
		LoadFailed,
		Consistent,
		Mismatch
	};

private:
	struct EditState {
		std::string songName;
		std::string artistName;
		std::string mapperName;
		std::string songPath;
		std::string songLength;
		float songPreviewSeconds = 0.0f;
		float bpm = 100.0f;
		ProjectBpmMode projectBpmMode = ProjectBpmMode::Single;
		std::string videoUrl;
		float youtubeOffset = 0.0f;
		bool isMirror = false;

		std::string editorVersion;
		int beatmapID = -1;
		int ostID = -1;
		std::string ostName;
		long long createTicks = 0ll;
		std::string createTicksStr;
	};

	struct DifficultyBpmStatus {
		int diffIndex = -1;
		bool exists = false;
		bool loaded = false;
		BpmExportMode exportMode = BpmExportMode::Single;
		size_t timingCount = 0;
		float firstTimingBpm = 0.0f;
		float rawExportBpm = 0.0f;
		float effectiveExportBpm = 0.0f;
		DifficultyBpmConsistency consistency = DifficultyBpmConsistency::Missing;
		std::string message;
	};

	struct DifficultyBpmEditState {
		BpmExportMode exportMode = BpmExportMode::Single;
		float exportBpm = Beatmap::getDefaultExportBPM();
	};

	void openWorkingFolder();
	void drawFolderControls();
	void drawBeatmapInfo();
	void drawSongPathControls();
	void drawBpmConsistency();
	void drawDifficultyList(ScreenResult& result);
	void drawCloseConfirmationModal(ScreenResult& result);
	void browseSongFileInWorkingFolder();
	void importSongFileToWorkingFolder();
	bool trySetSongPathFromWorkingFolderFile(const std::filesystem::path& songFilePath);
	bool isSupportedAudioFile(const std::filesystem::path& songFilePath) const;
	void loadEditStateFromBeatmapInfo();
	void applyEditStateToBeatmapInfo();
	bool saveInfo();
	void updateBpmConsistency();
	void reExportDifficultyWithBpmSettings(int diffIndex);
	void createDifficulty(int diffIndex);
	void deleteDifficulty(int diffIndex);
	bool openDifficultyForEdit(int diffIndex, ScreenResult& result);
	bool tryGetFirstLoadedDifficultyFirstBpm(float& bpm) const;

	EditorContext& editorContext;
	Beatmap& beatmap;
	BeatmapInfo& bmInfo;
	BeatmapStack& beatmapStack;
	Editor& editor;
	void* nativeWindowHandle = nullptr;

	bool hasWorkingFolder = false;
	bool missingInfoJson = false;
	bool dirty = false;
	int pendingEditDiffIndex = -1;
	int pendingDeleteDiffIndex = -1;
	bool closeConfirmationRequested = false;
	std::string statusMessage;
	EditState editState;
	ProjectBpmConsistency projectBpmConsistency = ProjectBpmConsistency::Unknown;
	std::array<DifficultyBpmStatus, BeatmapInfo::LEVEL_NUM> difficultyBpmStatuses{};
	std::array<DifficultyBpmEditState, BeatmapInfo::LEVEL_NUM> difficultyBpmEditStates{};
};
