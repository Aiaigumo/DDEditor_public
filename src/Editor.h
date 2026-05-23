#pragma once
#include "Beatmap.h"
#include "BeatmapInfo.h"
#include "BeatmapStack.h"
#include "EditorContext.h"
#include "InputState.h"
#include "AudioPlayer.h"
#include <string>

// Coordinates per-frame editor behavior and exposes operation-level editing APIs with undo handling.
class Editor {
public:
	enum class SongCopyStatus {
		COPIED,
		SKIPPED_INVALID_SOURCE,
		FAILED_COPY
	};

	// Describes the result of exporting beatmap data and copying the song file.
	struct ExportResult {
		bool exportSucceeded = false;
		SongCopyStatus songCopyStatus = SongCopyStatus::SKIPPED_INVALID_SOURCE;
		std::string songCopyMessage = "";
	};

	Editor() = default;
	~Editor() = default;
	bool tick(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState, bool allowEditInput);

	//note editing methods
	void clearSelection(EditorContext& ec, bool clearEditingGroupNotes);
	void selectNote(Note* note, EditorContext& ec, Beatmap& beatmap, bool isMultipleSelect, bool enableUnselect);
	void selectGroupNotes(Note* note, EditorContext& ec, Beatmap& beatmap);
	bool putNote(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState);
	bool deleteNoteByPointer(Note* note, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool deleteNoteByID(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool deleteSelectedNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool clearAllNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool changeNoteByDelta(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt);
	bool changeSelectedNotesByDelta(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt);
	bool setObstacleDuration(uint64_t obstacleGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double durationSec);
	bool setStreamRepeatIntervalSec(uint64_t streamGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double intervalSec);
	//timing editing methods
	void updateTimingState(EditorContext& ec, Beatmap& beatmap);
	void updateSeek(AudioPlayer& songPlayer, EditorContext& ec, Beatmap& beatmap);
	void updateBeat(EditorContext& ec, Beatmap& beatmap);
	bool addTimingAtCurrentTime(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool setTimingBPM(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float bpm);
	bool setTimingOffset(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double offset);
	bool setTimingSubbeat(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, int subbeat);
	bool deleteTiming(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
	bool setExportBPM(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float bpm);
	bool setEditorOffset(EditorContext& ec, Beatmap& beatmap, double offsetInEditor);
	//beatmap lifecycle methods
	bool loadSong(EditorContext& ec, Beatmap& beatmap, const std::string& path);
	bool loadDifficultyAndSong(Beatmap& beatmap, BeatmapStack& beatmapStack, EditorContext& ec, const BeatmapInfo& bmInfo, int diffIndex);
	bool clearBeatmap(EditorContext& ec, Beatmap& beatmap, BeatmapInfo& bmInfo, BeatmapStack& beatmapStack);
	ExportResult exportBeatmapAndSong(Beatmap& beatmap, BeatmapInfo& bmInfo, EditorContext& ec, bool overwriteExisting);

private:

	Note* scanFrame(EditorContext& ec, Beatmap& beatmap, const InputState& inputState, bool requireRaycast);

	void undoBeatmapStack(Beatmap& beatmap, BeatmapStack& beatmapStack, EditorContext& editorContext);

	void calcDrawingPositionsOfSelectedNotes(EditorContext& ec, Beatmap& beatmap);

	//internal note editing method
	bool changeNoteByDeltaInternal(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt, bool saveUndo);
	bool changeSelectedNotesByDeltaInternal(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt, bool saveUndo);
	bool setObstacleDurationInternal(uint64_t obstacleGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double durationSec, bool saveUndo);
	bool setStreamRepeatIntervalSecInternal(uint64_t streamGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double intervalSec, bool saveUndo);

	// drag edit
	void startDragEdit(Note* notep, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState);
	bool dragEditSelectedNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState);

	// rect select 
	void startRectSelect(EditorContext& ec, const InputState& inputState);
	void updateRectSelect(EditorContext& ec, Beatmap& beatmap, const InputState& inputState);
	void scanNotesInRect(EditorContext& ec, Beatmap& beatmap);
};
