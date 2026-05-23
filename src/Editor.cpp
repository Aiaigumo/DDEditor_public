#include "Editor.h"
#include "EditorUtility.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <unordered_set>

using namespace EditorUtility;

namespace {
bool hasDelta(float dx, float dy, int gridDx, int gridDy, double dt) {
	return std::fabs(dx) > CurvePathDetail::TIME_EPSILON ||
		std::fabs(dy) > CurvePathDetail::TIME_EPSILON ||
		gridDx != 0 ||
		gridDy != 0 ||
		std::fabs(dt) > CurvePathDetail::TIME_EPSILON;
}

void pushNoteToDrawBuffers(EditorContext& ec, NoteType type, const glm::vec3& notePos, bool isOnHitLine) {
	switch (type) {
	case NoteType::LEFT_HAND:
		if (isOnHitLine) ec.yhNotes.push_back(notePos);
		else ec.lhNotes.push_back(notePos);
		break;
	case NoteType::RIGHT_HAND:
		if (isOnHitLine) ec.yhNotes.push_back(notePos);
		else ec.rhNotes.push_back(notePos);
		break;
	case NoteType::LEFT_FOOT:
		if (isOnHitLine) ec.yfNotes.push_back(notePos);
		else ec.lfNotes.push_back(notePos);
		break;
	case NoteType::RIGHT_FOOT:
		if (isOnHitLine) ec.yfNotes.push_back(notePos);
		else ec.rfNotes.push_back(notePos);
		break;
	case NoteType::BAR:
		ec.bars.push_back(notePos);
		break;
	case NoteType::TRAP:
		ec.traps.push_back(notePos);
		break;
	case NoteType::STREAM_NODE:
		ec.nodes.push_back(notePos);
		break;
	default:
		break;
	}
}
}

bool Editor::tick(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState, bool allowEditInput) {
	bool changed = false;

	if (allowEditInput && inputState.isActionPressed(Action::Undo)) {
		changed = beatmapStack.canUndo() || changed;
		undoBeatmapStack(beatmap, beatmapStack, ec);
	}

	ec.clearDrawingPositions();
	beatmap.setStreamSegmentDivisions(ec.streamSegmentDivisions);

	bool requiredRaycast = allowEditInput && (
		(ec.isHoveredContent3d || ec.isHoveredContent2d)
		&& (
			inputState.isActionPressed(Action::SelectPut) ||
			inputState.isActionPressed(Action::MultipleSelect) ||
			inputState.isActionPressed(Action::GroupNotesSelect) ||
			inputState.isActionPressed(Action::SelectDelete) ||
			inputState.isActionPressed(Action::MultipleSelectDelete)
			));

	Note* notep = scanFrame(ec, beatmap, inputState, requiredRaycast);

	if (requiredRaycast) {

		if (inputState.isActionPressed(Action::SelectDelete)) {
			changed = deleteNoteByPointer(notep, ec, beatmap, beatmapStack) || changed;
			notep = nullptr;
		}

		if (inputState.isActionPressed(Action::MultipleSelectDelete)) {
			selectNote(notep, ec, beatmap, true, false);
			changed = deleteSelectedNotes(ec, beatmap, beatmapStack) || changed;
			notep = nullptr;
		}

		bool pointedNoteIn3d = ec.isHoveredContent3d && notep != nullptr;
		bool pointedNoteIsBeforeEditPlaneIn3d = false;
		if (pointedNoteIn3d) {
			glm::vec3 notePos = calcNotePos(*notep, beatmap, ec);
			if (ec.cameraPos.z > ec.editingZ) {
				pointedNoteIsBeforeEditPlaneIn3d = notePos.z >= ec.editingZ;
			}
			else {
				pointedNoteIsBeforeEditPlaneIn3d = notePos.z <= ec.editingZ;
			}
		}
		bool pointedNoteIn2d = ec.isHoveredContent2d && notep != nullptr;
		bool isPuttingMode = !(ec.editMode == EditMode::MOVE || ec.editMode == EditMode::NONE);

		//select or put
		if (inputState.isActionPressed(Action::SelectPut)) {
			if (!isPuttingMode && (pointedNoteIn3d || pointedNoteIn2d) || isPuttingMode && (pointedNoteIsBeforeEditPlaneIn3d || pointedNoteIn2d)) {
				selectNote(notep, ec, beatmap, false, false);
				startDragEdit(notep, ec, beatmap, beatmapStack, inputState);
			}
			else if (isPuttingMode) {
				changed = putNote(ec, beatmap, beatmapStack, inputState) || changed;
			}
			else {
				clearSelection(ec, true);
				startRectSelect(ec, inputState);
			}
		}

		if (inputState.isActionPressed(Action::MultipleSelect)) {
			selectNote(notep, ec, beatmap, true, true);
			startDragEdit(notep, ec, beatmap, beatmapStack, inputState);
		}

		if (inputState.isActionPressed(Action::GroupNotesSelect)) {
			selectGroupNotes(notep, ec, beatmap);
		}

	}

	if (allowEditInput) {
		// drag Edit
		changed = dragEditSelectedNotes(ec, beatmap, beatmapStack, inputState) || changed;
		// rect Select
		updateRectSelect(ec, beatmap, inputState);
	}
	else {
		ec.isDraggingNotes3d = false;
		ec.isDraggingNotes2d = false;
		ec.duringRectSelect3d = false;
		ec.duringRectSelect2d = false;
		ec.dragStartWorldPos = glm::vec3(0.0f);
		ec.dragGridWorldPos = glm::vec3(0.0f);
		ec.rectSelectStartPos = ImVec2(0.0f, 0.0f);
		ec.rectSelectEndPos = ImVec2(0.0f, 0.0f);
	}

	calcDrawingPositionsOfSelectedNotes(ec, beatmap);

	return changed;
}

bool Editor::changeNoteByDelta(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt) {
	return changeNoteByDeltaInternal(noteID, ec, beatmap, beatmapStack, dx, dy, gridDx, gridDy, dt, true);
}

bool Editor::changeSelectedNotesByDelta(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt) {
	return changeSelectedNotesByDeltaInternal(ec, beatmap, beatmapStack, dx, dy, gridDx, gridDy, dt, true);
}

bool Editor::setObstacleDuration(uint64_t obstacleGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double durationSec) {
	return setObstacleDurationInternal(obstacleGroupID, ec, beatmap, beatmapStack, durationSec, true);
}

bool Editor::setStreamRepeatIntervalSec(uint64_t streamGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double intervalSec) {
	return setStreamRepeatIntervalSecInternal(streamGroupID, ec, beatmap, beatmapStack, intervalSec, true);
}

void Editor::updateTimingState(EditorContext& ec, Beatmap& beatmap) {
	beatmap.updateTimingList(ec.currentPlayerTime);

	const double firstSecPerBeat = beatmap.getFirstSecPerBeat();
	if (firstSecPerBeat > CurvePathDetail::TIME_EPSILON) {
		ec.distPerBeat = static_cast<float>(ec.DEFAULT_DIST_PER_BEAT * beatmap.getsecPerBeat() / firstSecPerBeat);
	}

	ec.subbeat = beatmap.getSubbeat();
	const double step = beatmap.getsecPerBeat() / static_cast<double>(ec.subbeat);
	double snappedRepeatIntervalSec = snapToStep(ec.streamRepeatIntervalSec, step);
	if (snappedRepeatIntervalSec <= CurvePathDetail::TIME_EPSILON) {
		snappedRepeatIntervalSec = step;
	}
	ec.streamRepeatIntervalSec = snappedRepeatIntervalSec;
}

void Editor::updateSeek(AudioPlayer& songPlayer, EditorContext& ec, Beatmap& beatmap) {
	const bool isPlaying = songPlayer.isPlaying();
	if (isPlaying) {
		songPlayer.stop();
	}
	songPlayer.setSeek(ec.currentPlayerTime);
	if (isPlaying) {
		songPlayer.play();
	}
	updateTimingState(ec, beatmap);
	beatmap.checkNextBeat(ec.currentPlayerTime);
	beatmap.resetSoundAlreadyPlayed();
	ec.editingZ = -ec.hitLineDistance;
}

void Editor::updateBeat(EditorContext& ec, Beatmap& beatmap) {
	if (!ec.isSongPlaying) {
		return;
	}

	if (beatmap.checkNextBeat(ec.currentPlayerTime)) {
		ec.metronomePlayer.playFromStart();
	}
}

bool Editor::addTimingAtCurrentTime(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	beatmapStack.push(beatmap);
	beatmap.addTiming(beatmap.getBPM(), ec.currentPlayerTime);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::setTimingBPM(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float bpm) {
	if (bpm <= 0.0f) {
		return false;
	}

	TimingList& timingList = beatmap.getTimingList();
	if (std::fabs(timingList.getBPM(timingIndex) - bpm) <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}

	beatmapStack.push(beatmap);
	timingList.setBPM(bpm, timingIndex);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::setTimingOffset(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double offset) {
	TimingList& timingList = beatmap.getTimingList();
	if (std::fabs(timingList.getOffset(timingIndex) - offset) <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}

	beatmapStack.push(beatmap);
	timingList.setOffset(offset, timingIndex);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::setTimingSubbeat(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, int subbeat) {
	const int normalizedSubbeat = (std::max)(subbeat, 1);
	TimingList& timingList = beatmap.getTimingList();
	if (timingList.getSubbeat(timingIndex) == normalizedSubbeat) {
		return false;
	}

	beatmapStack.push(beatmap);
	timingList.setSubbeat(normalizedSubbeat, timingIndex);
	beatmap.checkNextBeat(ec.currentPlayerTime);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::deleteTiming(size_t timingIndex, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	if (timingIndex == 0) {
		return false;
	}

	beatmapStack.push(beatmap);
	beatmap.getTimingList().removeTiming(timingIndex);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::setExportBPM(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float bpm) {
	if (beatmap.getTimingListSize() <= 1 || bpm <= 0.0f) {
		return false;
	}
	if (std::fabs(beatmap.getRawExportBPM() - bpm) <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}

	beatmapStack.push(beatmap);
	beatmap.setRawExportBPM(bpm);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::setEditorOffset(EditorContext& ec, Beatmap& beatmap, double offsetInEditor) {
	if (std::fabs(beatmap.getOffsetInEditor() - offsetInEditor) <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}

	beatmap.setOffsetInEditor(offsetInEditor);
	updateTimingState(ec, beatmap);
	return true;
}

bool Editor::loadSong(EditorContext& ec, Beatmap& beatmap, const std::string& path) {
	const bool isSongLoaded = ec.songPlayer.load(path);
	if (!isSongLoaded) {
		return false;
	}

	ec.loadedSongFilePath = path;
	ec.songPlayer.setVolume(ec.songVolume);
	ec.songLength = ec.songPlayer.getLength();
	beatmap.setSongDuration(ec.songLength);
	return true;
}

bool Editor::loadDifficultyAndSong(Beatmap& beatmap, BeatmapStack& beatmapStack, EditorContext& ec, const BeatmapInfo& bmInfo, int diffIndex) {
	if (diffIndex < 0 || diffIndex >= static_cast<int>(BeatmapInfo::LEVEL_NUM)) {
		return false;
	}

	ec.diffPathBuffer = ec.beatmapFolderPathBuffer + "/" + bmInfo.getDiffPath(diffIndex);
	ec.outPutBeatmapFolderPathBuffer = ec.beatmapFolderPathBuffer;
	ec.outPutDiffPathBuffer = bmInfo.getDiffPath(diffIndex);
	ec.currentDiffIndex = diffIndex;

	beatmapStack.push(beatmap);
	ec.diffLoaded.fill(false);
	ec.diffLoaded.at(diffIndex) = beatmap.loadFromFile(ec.diffPathBuffer);
	ec.currentPlayerTime = ec.songPlayer.getCurrentTime();
	ec.selectedNotes.clear();
	ec.groupNotesEditing = 0;

	loadSong(ec, beatmap, ec.songPathBuffer);
	if (ec.diffLoaded.at(diffIndex)) {
		updateTimingState(ec, beatmap);
	}
	return ec.diffLoaded.at(diffIndex);
}

bool Editor::clearBeatmap(EditorContext& ec, Beatmap& beatmap, BeatmapInfo& bmInfo, BeatmapStack& beatmapStack) {
	beatmapStack.push(beatmap);
	beatmap.clear();
	bmInfo.clear();
	ec.selectedNotes.clear();
	ec.groupNotesEditing = 0;
	ec.diffLoaded.fill(false);
	ec.loadedSongFilePath.clear();
	ec.hasLoadedBeatmapFolder = false;
	updateTimingState(ec, beatmap);
	return true;
}

Editor::ExportResult Editor::exportBeatmapAndSong(Beatmap& beatmap, BeatmapInfo& bmInfo, EditorContext& ec, bool overwriteExisting) {
	ExportResult result{};
	std::error_code fsError;
	std::filesystem::create_directories(ec.outPutBeatmapFolderPathBuffer, fsError);
	if (fsError) {
		std::cerr << "Error creating output beatmap folder: " << fsError.message() << std::endl;
		result.exportSucceeded = false;
		result.songCopyStatus = SongCopyStatus::FAILED_COPY;
		result.songCopyMessage = "Song file was not copied because export folder creation failed.";
		return result;
	}

	std::filesystem::path outDir = ec.outPutBeatmapFolderPathBuffer;
	std::filesystem::path infoFile = outDir / ec.infoPath;
	std::filesystem::path levelFile = outDir / ec.outPutDiffPathBuffer;

	beatmap.organizeVecGroupNotesIndex();
	beatmap.exportJson(levelFile.string(), overwriteExisting);

	std::filesystem::path originalSongFile = ec.loadedSongFilePath;
	bool hasValidLoadedSongFile = !(originalSongFile.empty() || !std::filesystem::exists(originalSongFile, fsError));
	if (hasValidLoadedSongFile) {
		std::filesystem::path songFile = outDir / originalSongFile.filename();
		bmInfo.setSongPath(songFile.filename().string());
		fsError.clear();
		if (overwriteExisting) {
			std::filesystem::copy_file(originalSongFile, songFile, std::filesystem::copy_options::overwrite_existing, fsError);
		}
		else {
			std::filesystem::copy_file(originalSongFile, songFile, fsError);
		}
		if (fsError) {
			std::cerr << "Warning exporting beatmap: failed to copy song file: " << fsError.message() << std::endl;
			result.songCopyStatus = SongCopyStatus::FAILED_COPY;
			result.songCopyMessage = "Failed to copy song file: " + fsError.message();
		}
		else {
			result.songCopyStatus = SongCopyStatus::COPIED;
			result.songCopyMessage = "";
		}
	}
	else {
		std::cerr << "Warning exporting beatmap: loaded song file path is invalid, skip copying song file: " << originalSongFile << std::endl;
		result.songCopyStatus = SongCopyStatus::SKIPPED_INVALID_SOURCE;
		result.songCopyMessage = "Song file was not copied because loaded song path is invalid.";
	}

	bmInfo.setSongLengthAndBPM(beatmap);
	bmInfo.setDiffPath(ec.currentDiffIndex, ec.outPutDiffPathBuffer);
	bmInfo.saveToFile(infoFile.string(), overwriteExisting);
	ec.beatmapFolderPathBuffer = ec.outPutBeatmapFolderPathBuffer;
	result.exportSucceeded = true;
	return result;
}

bool Editor::changeNoteByDeltaInternal(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt, bool saveUndo) {
	if (!hasDelta(dx, dy, gridDx, gridDy, dt)) {
		return false;
	}

	Note& note = beatmap.getNoteByID(noteID);
	const uint64_t groupID = note.getGroupID();
	const NoteType type = note.getType();
	const bool rebuildGeneratedStream = note.isInGroupNotes() && beatmap.existInStreamTemplateWithGroupID(groupID);
	bool changed = false;

	switch (type) {
	case NoteType::LEFT_HAND:
	case NoteType::RIGHT_HAND:
	case NoteType::LEFT_FOOT:
	case NoteType::RIGHT_FOOT:
	case NoteType::TRAP:
	case NoteType::BAR:
	{
		const double snappedTime = calcTimeWithBeat(note.getTime() + dt, beatmap, ec);
		changed = gridDx != 0 || gridDy != 0 || std::fabs(snappedTime - note.getTime()) > CurvePathDetail::TIME_EPSILON;
		if (!changed) {
			break;
		}
		if (saveUndo) {
			beatmapStack.push(beatmap);
		}
		note.setX(note.getX() + gridDx);
		note.setY(note.getY() + gridDy);
		note.setTime(snappedTime);
		break;
	}
	case NoteType::LONG_LEFT_FOOT:
	case NoteType::LONG_RIGHT_FOOT:
	{
		const double snappedTime = calcTimeWithBeat(note.getTime() + dt, beatmap, ec);
		const double snappedDt = snappedTime - note.getTime();
		changed = gridDx != 0 || gridDy != 0 || std::fabs(snappedDt) > CurvePathDetail::TIME_EPSILON;
		if (!changed) {
			break;
		}
		if (saveUndo) {
			beatmapStack.push(beatmap);
		}
		GroupNotes& longGroup = beatmap.getGroupNotesWithGroupID(groupID);
		longGroup.changeLongNotesNodeByNoteID(noteID, gridDx, gridDy, snappedDt);
		break;
	}
	case NoteType::STREAM_NODE:
	{
		const double snappedTime = calcTimeWithBeat(note.getTime() + dt, beatmap, ec);
		const double snappedDt = snappedTime - note.getTime();
		changed = std::fabs(dx) > CurvePathDetail::TIME_EPSILON ||
			std::fabs(dy) > CurvePathDetail::TIME_EPSILON ||
			std::fabs(snappedDt) > CurvePathDetail::TIME_EPSILON;
		if (!changed) {
			break;
		}
		if (saveUndo) {
			beatmapStack.push(beatmap);
		}
		beatmap.changeStreamNodeByNoteID(noteID, dx, dy, snappedDt);
		break;
	}
	case NoteType::OBS:
	{
		GroupNotes& obsGroup = beatmap.getGroupNotesWithGroupID(groupID);
		const double snappedStartTime = calcTimeWithBeat(note.getTime() + dt, beatmap, ec);
		changed = gridDx != 0 || gridDy != 0 || std::fabs(snappedStartTime - note.getTime()) > CurvePathDetail::TIME_EPSILON;
		if (!changed) {
			break;
		}
		if (saveUndo) {
			beatmapStack.push(beatmap);
		}
		obsGroup.setObstacleX(obsGroup.getNoteAtIndex(0).getX() + gridDx);
		obsGroup.setObstacleY(obsGroup.getNoteAtIndex(0).getY() + gridDy);
		obsGroup.setObstacleStartTime(snappedStartTime);
		break;
	}
	case NoteType::OBS_END:
	{
		GroupNotes& obsGroup = beatmap.getGroupNotesWithGroupID(groupID);
		const double snappedEndTime = calcTimeWithBeat(note.getTime() + dt, beatmap, ec);
		const double newDuration = snappedEndTime - obsGroup.getStartTime();
		if (newDuration <= CurvePathDetail::TIME_EPSILON ||
			std::fabs(newDuration - note.getDuration()) <= CurvePathDetail::TIME_EPSILON) {
			break;
		}
		if (saveUndo) {
			beatmapStack.push(beatmap);
		}
		obsGroup.setObstacleDuration(newDuration);
		changed = true;
		break;
	}
	default:
		break;
	}

	if (changed && rebuildGeneratedStream) {
		beatmap.rebuildGeneratedStreamGroupNotes();
	}
	return changed;
}

bool Editor::changeSelectedNotesByDeltaInternal(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, float dx, float dy, int gridDx, int gridDy, double dt, bool saveUndo) {
	std::unordered_set<uint64_t> obstacleGroupsWithSelectedStart;
	for (uint64_t noteID : ec.selectedNotes) {
		Note& note = beatmap.getNoteByID(noteID);
		if (note.getType() == NoteType::OBS) {
			obstacleGroupsWithSelectedStart.insert(note.getGroupID());
		}
	}

	bool changed = false;
	bool shouldSaveUndo = saveUndo;
	for (uint64_t noteID : ec.selectedNotes) {
		Note& note = beatmap.getNoteByID(noteID);
		if (note.getType() == NoteType::OBS_END &&
			obstacleGroupsWithSelectedStart.find(note.getGroupID()) != obstacleGroupsWithSelectedStart.end()) {
			continue;
		}

		const bool noteChanged = changeNoteByDeltaInternal(noteID, ec, beatmap, beatmapStack, dx, dy, gridDx, gridDy, dt, shouldSaveUndo);
		if (noteChanged) {
			changed = true;
			shouldSaveUndo = false;
		}
	}
	return changed;
}

bool Editor::setObstacleDurationInternal(uint64_t obstacleGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double durationSec, bool saveUndo) {
	(void)ec;
	if (durationSec <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}

	GroupNotes& obsGroup = beatmap.getGroupNotesWithGroupID(obstacleGroupID);
	if (obsGroup.getGroupType() != GroupType::OBSTACLE || obsGroup.size() == 0) {
		return false;
	}
	const double currentDuration = obsGroup.getNoteAtIndex(0).getDuration();
	if (std::fabs(currentDuration - durationSec) <= CurvePathDetail::TIME_EPSILON) {
		return false;
	}
	if (saveUndo) {
		beatmapStack.push(beatmap);
	}
	obsGroup.setObstacleDuration(durationSec);
	if (beatmap.existInStreamTemplateWithGroupID(obstacleGroupID)) {
		beatmap.rebuildGeneratedStreamGroupNotes();
	}
	return true;
}

bool Editor::setStreamRepeatIntervalSecInternal(uint64_t streamGroupID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, double intervalSec, bool saveUndo) {
	const double step = beatmap.getsecPerBeat() / static_cast<double>(beatmap.getSubbeat());
	double snappedIntervalSec = intervalSec;
	if (step > CurvePathDetail::TIME_EPSILON) {
		snappedIntervalSec = snapToStep(intervalSec, step);
		if (snappedIntervalSec <= CurvePathDetail::TIME_EPSILON) {
			snappedIntervalSec = step;
		}
	}

	GroupNotes& streamGroup = beatmap.getGroupNotesWithGroupID(streamGroupID);
	if (streamGroup.getGroupType() != GroupType::STREAM) {
		return false;
	}
	if (std::fabs(streamGroup.getStreamRepeatIntervalSec() - snappedIntervalSec) <= CurvePathDetail::TIME_EPSILON) {
		ec.streamRepeatIntervalSec = snappedIntervalSec;
		return false;
	}
	if (saveUndo) {
		beatmapStack.push(beatmap);
	}
	beatmap.setStreamRepeatIntervalSec(streamGroupID, snappedIntervalSec);
	ec.streamRepeatIntervalSec = snappedIntervalSec;
	return true;
}

bool Editor::deleteNoteByPointer(Note* note, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	if (note == nullptr) {
		return false;
	}
	return deleteNoteByID(note->getNoteID(), ec, beatmap, beatmapStack);
}

bool Editor::deleteNoteByID(uint64_t noteID, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	Note& note = beatmap.getNoteByID(noteID);
	const uint64_t groupID = note.getGroupID();
	beatmapStack.push(beatmap);

	const bool deletedGroupNotes = beatmap.removeNoteByNoteID(noteID);
	const bool clearEditingGroupNotes = deletedGroupNotes && groupID == ec.groupNotesEditing;
	clearSelection(ec, clearEditingGroupNotes);
	return true;
}

bool Editor::deleteSelectedNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	if (ec.selectedNotes.empty()) {
		return false;
	}

	const std::vector<uint64_t> selectedNotes = ec.selectedNotes;
	bool deletedGroupNotesEditing = false;
	bool hasDeleted = false;
	bool saved = false;

	for (uint64_t noteID : selectedNotes) {
		Note& note = beatmap.getNoteByID(noteID);
		const uint64_t groupID = note.getGroupID();
		if (!saved) {
			beatmapStack.push(beatmap);
			saved = true;
		}
		const bool deletedGroupNotes = beatmap.removeNoteByNoteID(noteID);
		deletedGroupNotesEditing = deletedGroupNotesEditing || (deletedGroupNotes && groupID == ec.groupNotesEditing);
		hasDeleted = true;
	}

	if (hasDeleted) {
		clearSelection(ec, deletedGroupNotesEditing);
	}
	return hasDeleted;
}

bool Editor::clearAllNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {
	beatmapStack.push(beatmap);
	beatmap.clearNotes();
	clearSelection(ec, true);
	ec.diffLoaded.fill(false);
	return true;
}

void Editor::undoBeatmapStack(Beatmap& beatmap, BeatmapStack& beatmapStack, EditorContext& ec) {
	beatmapStack.undo(beatmap);
	clearSelection(ec, true);
	updateTimingState(ec, beatmap);
}

void Editor::clearSelection(EditorContext& ec, bool clearEditingGroupNotes) {
	ec.selectedNotes.clear();
	if (clearEditingGroupNotes) {
		ec.groupNotesEditing = 0;
	}
	ec.editingZ = -ec.hitLineDistance;
}

Note* Editor::scanFrame(EditorContext& editorContext, Beatmap& beatmap, const InputState& inputState, bool requireRaycast) {

	//calc distance moved distance with song playing
	editorContext.laneMoveDistance = static_cast<float>((editorContext.currentPlayerTime - beatmap.getActualOffset()) / beatmap.getsecPerBeat() * editorContext.distPerBeat);

	float drawMaxDistance = (std::max)(editorContext.perspectiveDrawMaxDistance, editorContext.orthoDrawMaxDistance);
	float drawMinDistance = (std::min)(editorContext.perspectiveDrawMinDistance, editorContext.orthoDrawMinDistance);

	//calc timing lines pos to draw
	{
		const TimingList& timingList = beatmap.getTimingList();
		for (size_t i = 0; i < timingList.size(); i++) {
			double nextTimingOffset = 0.0;
			if (i + 1 < beatmap.getTimingListSize()) {
				nextTimingOffset = timingList.getOffsetInNoteTime(i + 1);
			}
			else {
				nextTimingOffset = editorContext.songLength; // until end of song
			}
			double timingOffset = timingList.getOffsetInNoteTime(i);
			double secPerBeat = timingList.secPerBeat(i);
			int subbeat = timingList.getSubbeat(i);
			double step = secPerBeat / static_cast<double>(subbeat);

			for (int n = 0; timingOffset + static_cast<double>(n) * step < nextTimingOffset; n++) {
				double t = timingOffset + static_cast<double>(n) * step;
				float posZ = calcNotePosZ(t, beatmap, editorContext);
				if (posZ < -drawMaxDistance) {
					break;
				}
				else if (posZ > drawMinDistance) {
					continue;
				}
				if (n % subbeat == 0) {
					editorContext.timingLinesOnBeat.push_back(posZ);
				}
				else {
					editorContext.timingLinesOnSubbeat.push_back(posZ);
				}
				// timing start line
				if (t == timingOffset) {
					editorContext.timingStartLines.push_back(posZ);
				}
			}
		}
	}

	float lastSelectedNoteDistance = FLT_MAX;					// check the note is most close distance of raycasted note
	Note* lastSelectedNote = nullptr;

	//single notes
	{
		bool judgeEnd = false;

		for (size_t n = 0; n < beatmap.getGroupNotesSize(); n++) {	//for each notes
			if (judgeEnd) break;
			Note& note = beatmap.getNoteAtIndex(n);
			NoteType type = note.getType();		//note type
			glm::vec3 notePos = calcNotePos(note, beatmap, editorContext);


			if (notePos.z < -drawMaxDistance) {	// when it is too far
				judgeEnd = true;	// notes vector is sorted, so stop judge
				break;
			}
			else if (notePos.z > drawMinDistance) { // when it is completely back side (already passsed)
				continue;
			}

			//check raycast
			if (requireRaycast) {

				if (editorContext.isHoveredContent3d) {
					if (type != NoteType::BAR) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, true);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {

							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}
					else {	//when bar, collision is large in direction x
						for (float r = 0.0f; r < editorContext.barScale.x; r += editorContext.hitRadius) {
							glm::vec3 collisionPos = notePos;
							collisionPos.x += r - editorContext.barScale.x / 2;
							float dist = checkMouseRaySphere(collisionPos, inputState.getMousePos(), editorContext, true);
							if (dist > 0.0f && dist < lastSelectedNoteDistance) {

								lastSelectedNoteDistance = dist;
								lastSelectedNote = &note;
							}
						}
					}
				}

				if (editorContext.isHoveredContent2d) {
					if (type != NoteType::BAR) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, false);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {

							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}
					else {	//when bar, collision is large in direction x
						for (float r = 0.0f; r < editorContext.barScale.x; r += editorContext.hitRadius) {
							glm::vec3 collisionPos = notePos;
							collisionPos.x += r - editorContext.barScale.x / 2;
							float dist = checkMouseRaySphere(collisionPos, inputState.getMousePos(), editorContext, false);
							if (dist > 0.0f && dist < lastSelectedNoteDistance) {

								lastSelectedNoteDistance = dist;
								lastSelectedNote = &note;
							}
						}
					}
				}
			}
			const bool isOnHitLine =
				editorContext.isSongPlaying &&
				checkOnHitLine(notePos.z, -editorContext.hitLineDistance, editorContext.rangeHitLine);
			if (isOnHitLine && notePos.z >= -editorContext.hitLineDistance) {
				playHitSound(note, editorContext);
			}
			pushNoteToDrawBuffers(editorContext, type, notePos, isOnHitLine);
			continue;	//continue (for)
		}
	}

	//group Notes
	for (size_t n = 0; n < beatmap.getVecGroupNotesLength(); n++) {
		GroupNotes& gn = beatmap.getGroupNotesAtIndex(n);
		float startPosZ = calcNotePosZ(gn.getStartTime(), beatmap, editorContext);
		float endPosZ = calcNotePosZ(gn.getEndTime(), beatmap, editorContext);

		//check the group is in draw range
		if (startPosZ < -drawMaxDistance) {
			break;
		}
		else if (endPosZ > drawMinDistance) {
			continue;
		}

		GroupType groupType = gn.getGroupType();
		switch (groupType) {
		case GroupType::LONG_NOTE:

		{
			NoteType noteType = gn.getNoteType();
			std::vector<glm::vec3> pointList;
			switch (noteType) {

			case NoteType::LONG_LEFT_FOOT:

				for (size_t i = 0; i < gn.size(); i++) {
					Note& note = gn.getNoteAtIndex(i);
					glm::vec3 notePos = calcNotePos(note, beatmap, editorContext);

					//check raycast
					if (editorContext.isHoveredContent3d && requireRaycast) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, true);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {
							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}
					if (editorContext.isHoveredContent2d && requireRaycast) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, false);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {
							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}

					if (editorContext.isSongPlaying && checkOnHitLine(notePos.z, -editorContext.hitLineDistance, editorContext.rangeHitLine)) {

						editorContext.yfNotes.push_back(notePos);
						if (notePos.z >= -editorContext.hitLineDistance) {
							playHitSound(note, editorContext);
						}
					}
					else {
						editorContext.lfNotes.push_back(notePos);
					}
				}
				pointList = EditorUtility::buildLongNoteRenderPolyline(gn, beatmap, editorContext);
				if (pointList.empty())
				{
					break;
				}
				editorContext.llfLines.insert(editorContext.llfLines.end(), pointList.begin(), pointList.end());
				editorContext.llfLines.push_back(glm::vec3(NAN));		//dammy for partition between groups
				break;

			case NoteType::LONG_RIGHT_FOOT:

				for (size_t i = 0; i < gn.size(); i++) {
					Note& note = gn.getNoteAtIndex(i);
					glm::vec3 notePos = calcNotePos(note, beatmap, editorContext);

					//check raycast
					if (editorContext.isHoveredContent3d && requireRaycast) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, true);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {
							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}
					if (editorContext.isHoveredContent2d && requireRaycast) {
						float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, false);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {
							lastSelectedNoteDistance = dist;
							lastSelectedNote = &note;
						}
					}

					if (editorContext.isSongPlaying && checkOnHitLine(notePos.z, -editorContext.hitLineDistance, editorContext.rangeHitLine)) {
						editorContext.yfNotes.push_back(notePos);
						if (notePos.z >= -editorContext.hitLineDistance) {
							playHitSound(note, editorContext);
						}
					}
					else {
						editorContext.rfNotes.push_back(notePos);
					}
				}
				pointList = EditorUtility::buildLongNoteRenderPolyline(gn, beatmap, editorContext);
				if (pointList.empty())
				{
					break;
				}
				editorContext.lrfLines.insert(editorContext.lrfLines.end(), pointList.begin(), pointList.end());
				editorContext.lrfLines.push_back(glm::vec3(NAN));		//dammy for partition between groups
				break;
			default:
				//std::cerr << "[Editor.cpp] Note type of GroupNotes(longNotes) is unexpected" << std::endl;
				break;
			}
		}
		break;

		case GroupType::OBSTACLE:

			if (gn.size() == 2) {

				Note& startNote = gn.getNoteAtIndex(0); // get first note (start note)
				Note& endNote = gn.getNoteAtIndex(1);   // get second note (end note)
				glm::vec3 startNotePos = calcNotePos(startNote, beatmap, editorContext);
				glm::vec3 endNotePos = startNotePos;
				endNotePos.z = endPosZ;

				editorContext.obstacles.push_back(glm::vec4(startNotePos, endPosZ));

				//check raycast
				if (requireRaycast) {
					if (editorContext.isHoveredContent3d) {
						//check raycast on start edge & body
						for (float checkZ = startNotePos.z; checkZ > endNotePos.z + editorContext.hitRadius; checkZ -= editorContext.hitRadius) {
							for (float h = 0.0f; h < editorContext.obsScale.y; h += editorContext.hitRadius) {
								glm::vec3 checkPos = startNotePos;
								checkPos.y += h;
								checkPos.z = checkZ;
								float dist = checkMouseRaySphere(checkPos, inputState.getMousePos(), editorContext, true);
								if (dist > 0.0f && dist < lastSelectedNoteDistance) {

									lastSelectedNoteDistance = dist;
									lastSelectedNote = &startNote;
								}
							}
						}

						//check raycast on end edge
						for (float h = 0.0f; h < editorContext.obsScale.y; h += editorContext.hitRadius) {
							glm::vec3 checkPos = endNotePos;
							checkPos.y += h;
							float dist = checkMouseRaySphere(checkPos, inputState.getMousePos(), editorContext, true);
							if (dist > 0.0f && dist < lastSelectedNoteDistance) {

								lastSelectedNoteDistance = dist;
								lastSelectedNote = &endNote;
							}
						}

					}
					if (editorContext.isHoveredContent2d) {
						//check raycast on start edge & body
						for (float checkZ = startNotePos.z; checkZ > endNotePos.z + editorContext.hitRadius; checkZ -= editorContext.hitRadius) {

							glm::vec3 checkPos = startNotePos;
							checkPos.z = checkZ;
							float dist = checkMouseRaySphere(checkPos, inputState.getMousePos(), editorContext, false);
							if (dist > 0.0f && dist < lastSelectedNoteDistance) {

								lastSelectedNoteDistance = dist;
								lastSelectedNote = &startNote;
							}

						}

						//check raycast on end edge
						glm::vec3 checkPos = endNotePos;
						float dist = checkMouseRaySphere(checkPos, inputState.getMousePos(), editorContext, false);
						if (dist > 0.0f && dist < lastSelectedNoteDistance) {

							lastSelectedNoteDistance = dist;
							lastSelectedNote = &endNote;
						}
					}
				}


			}
			else {
				//std::cerr << "[Editor.cpp] GroupNotes(obstacle) is wrong format" << std::endl;
			}
			break;
		case GroupType::STREAM:
		{
			const bool isEditingStream = gn.getGroupID() == editorContext.groupNotesEditing;
			for (size_t i = 0; i < gn.size(); ++i) {
				Note& note = gn.getNoteAtIndex(i);
				glm::vec3 notePos = calcNotePos(note, beatmap, editorContext);

				if (editorContext.isHoveredContent3d && requireRaycast) {
					float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, true);
					if (dist > 0.0f && dist < lastSelectedNoteDistance) {
						lastSelectedNoteDistance = dist;
						lastSelectedNote = &note;
					}
				}
				if (editorContext.isHoveredContent2d && requireRaycast) {
					float dist = checkMouseRaySphere(notePos, inputState.getMousePos(), editorContext, false);
					if (dist > 0.0f && dist < lastSelectedNoteDistance) {
						lastSelectedNoteDistance = dist;
						lastSelectedNote = &note;
					}
				}

				if (!isEditingStream) {
					editorContext.nodes.push_back(notePos);
				}
			}
			std::vector<glm::vec3> streamPointList = EditorUtility::buildStreamRenderPolyline(gn, beatmap, editorContext);
			if (!isEditingStream && !streamPointList.empty()) {
				editorContext.streamLines.insert(editorContext.streamLines.end(), streamPointList.begin(), streamPointList.end());
				editorContext.streamLines.push_back(glm::vec3(NAN));
			}
			break;
		}

		default:
			//std::cerr << "[Editor.cpp] group type is none or unexpected" << std::endl;
			break;
		}

	}

	for (GroupNotes& generatedGroup : beatmap.getGeneratedStreamGroupNotes()) {
		if (generatedGroup.getGroupType() == GroupType::OBSTACLE) {
			if (generatedGroup.size() == 2) {
				const Note& startNote = generatedGroup.getNoteAtIndex(0);
				const Note& endNote = generatedGroup.getNoteAtIndex(1);
				const glm::vec3 startNotePos = calcNotePos(startNote, beatmap, editorContext);
				const glm::vec3 endNotePos = calcNotePos(endNote, beatmap, editorContext);
				if (!(endNotePos.z < -drawMaxDistance || startNotePos.z > drawMinDistance)) {
					editorContext.obstacles.push_back(glm::vec4(startNotePos.x, startNotePos.y, startNotePos.z, endNotePos.z));
				}
			}
			continue;
		}

		for (size_t i = 0; i < generatedGroup.size(); ++i) {
			Note& generatedNote = generatedGroup.getNoteAtIndex(i);
			const glm::vec3 generatedNotePos = calcNotePos(generatedNote, beatmap, editorContext);

			if (generatedNotePos.z < -drawMaxDistance || generatedNotePos.z > drawMinDistance) {
				continue;
			}

			const bool isOnHitLine =
				editorContext.isSongPlaying &&
				checkOnHitLine(generatedNotePos.z, -editorContext.hitLineDistance, editorContext.rangeHitLine);
			if (isOnHitLine && generatedNotePos.z >= -editorContext.hitLineDistance) {
				playHitSound(generatedNote, editorContext);
			}
			pushNoteToDrawBuffers(editorContext, generatedNote.getType(), generatedNotePos, isOnHitLine);
		}
	}
	return lastSelectedNote;
}

void Editor::calcDrawingPositionsOfSelectedNotes(EditorContext& ec, Beatmap& beatmap) {

	//draw editing GroupNotes
	if (ec.groupNotesEditing != 0) {
		GroupNotes& editingGroup = beatmap.getGroupNotesWithGroupID(ec.groupNotesEditing);
		GroupType gtype = editingGroup.getGroupType();
		std::vector<glm::vec3> pointList;
		switch (gtype) {
		case GroupType::LONG_NOTE:
			pointList = EditorUtility::buildLongNoteRenderPolyline(editingGroup, beatmap, ec);
			if (pointList.empty())
			{
				break;
			}
			ec.elfLines.insert(ec.elfLines.end(), pointList.begin(), pointList.end());
			ec.elfLines.push_back(glm::vec3(NAN));
			break;
		case GroupType::STREAM:
			for (size_t i = 0; i < editingGroup.size(); i++) {
				glm::vec3 notePos = calcNotePos(editingGroup.getNoteAtIndex(i), beatmap, ec);
				ec.enodes.push_back(notePos);
			}
			pointList = EditorUtility::buildStreamRenderPolyline(editingGroup, beatmap, ec);
			if (!pointList.empty()) {
				ec.editingStreamLines.insert(ec.editingStreamLines.end(), pointList.begin(), pointList.end());
				ec.editingStreamLines.push_back(glm::vec3(NAN));
			}
			break;
		default:
			break;
		}
	}

	//draw position(selected Notes)
	for (uint64_t noteID : ec.selectedNotes) {
		Note& note = beatmap.getNoteByID(noteID);
		glm::vec3 notePos = calcNotePos(note, beatmap, ec);
		switch (note.getType()) {
		case NoteType::LEFT_HAND:
		case NoteType::RIGHT_HAND:
		case NoteType::OBS:
		case NoteType::OBS_END:
			ec.slhNotes.push_back(notePos);
			break;
		case NoteType::RIGHT_FOOT:
		case NoteType::LEFT_FOOT:
		case NoteType::LONG_LEFT_FOOT:
		case NoteType::LONG_RIGHT_FOOT:
			ec.slfNotes.push_back(notePos);
			break;
		case NoteType::BAR:
			ec.slbars.push_back(notePos);
			break;
		case NoteType::TRAP:
			ec.sltraps.push_back(notePos);
			break;
		case NoteType::STREAM_NODE:
			ec.slnodes.push_back(notePos);
			break;
		default:
			//std::cerr << "[Editor.cpp] Note type of selected single note is unexpected" << std::endl;
			break;
		}
	}
}

void Editor::selectNote(Note* notep, EditorContext& ec, Beatmap& beatmap, bool isMultipleSelect, bool enableUnselect) {
	if (notep == nullptr) {
		if (!isMultipleSelect) {
			clearSelection(ec, true);
		}
		return;
	}

	//check already in selected notes
	const uint64_t noteID = notep->getNoteID();
	if (std::find(ec.selectedNotes.begin(), ec.selectedNotes.end(), noteID)
		== ec.selectedNotes.end()) {

		//when sigle select, clear selected notes
		if (!isMultipleSelect) {
			clearSelection(ec, false);

			// if select notes is not contained group editing, clear editing group 
			if (ec.groupNotesEditing != 0 && notep->getGroupID() != ec.groupNotesEditing) {
				ec.groupNotesEditing = 0;
			}
		}

		ec.selectedNotes.push_back(noteID);		//add to selected notes

		//sort selected notes by time	
		std::sort(ec.selectedNotes.begin(), ec.selectedNotes.end(), [&beatmap](uint64_t a, uint64_t b) {
			return beatmap.getNoteByID(a).getTime() < beatmap.getNoteByID(b).getTime();
			});
	}
	else if(enableUnselect){
		//when already in selected notes, unselect
		ec.selectedNotes.erase(std::remove(ec.selectedNotes.begin(), ec.selectedNotes.end(), noteID), ec.selectedNotes.end());
	}
}

void Editor::selectGroupNotes(Note* notep, EditorContext& ec, Beatmap& beatmap) {
	if (notep == nullptr) {
		return;
	}

	//select editing GroupNotes now by double click 
	if (notep->isInGroupNotes()) {
		GroupNotes& gn = beatmap.getGroupNotesWithGroupID(notep->getGroupID());
		if (gn.getGroupType() == GroupType::LONG_NOTE || gn.getGroupType() == GroupType::STREAM) {
			ec.groupNotesEditing = gn.getGroupID();
		}
	}
}

void Editor::startDragEdit(Note* notep, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState) {
	if (notep == nullptr) {
		return;
	}

	if (!ec.isHoveredContent3d && !ec.isHoveredContent2d) {
		return;
	}

	beatmapStack.push(beatmap);

	// start dragging
	if (ec.isHoveredContent3d)		ec.isDraggingNotes3d = true;
	else if (ec.isHoveredContent2d)	ec.isDraggingNotes2d = true;

	glm::vec3 notePos = calcNotePos(*notep, beatmap, ec);
	ec.editingZ = notePos.z;
	if (ec.isDraggingNotes3d) {
		ec.dragStartWorldPos = glm::vec3(mouseToWorldOnPlaneZ(inputState.getMousePos(), ec, notePos.z), notePos.z);
		ec.dragGridWorldPos = ec.dragStartWorldPos;
	}
	else if (ec.isDraggingNotes2d) {
		glm::vec2 mousePosXZ = mouseToWorldOnPlaneY(inputState.getMousePos(), ec, notePos.y);
		ec.dragStartWorldPos = glm::vec3(mousePosXZ.x, notePos.y, mousePosXZ.y);
		ec.dragGridWorldPos = ec.dragStartWorldPos;
	}
}

bool Editor::dragEditSelectedNotes(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState) {
	// drag edit
	bool changed = false;
	bool duringDragInput = (inputState.isActionDown(Action::SelectPut) || inputState.isActionDown(Action::MultipleSelect));
	bool hasSelectedStreamNode = false;
	for (uint64_t noteID : ec.selectedNotes) {
		if (beatmap.getNoteByID(noteID).getType() == NoteType::STREAM_NODE) {
			hasSelectedStreamNode = true;
			break;
		}
	}

	// when dragging note in 3d view
	if (ec.isDraggingNotes3d && duringDragInput) {
		// calc world pos delta
		glm::vec2 currentWorldPosPlaneZ = mouseToWorldOnPlaneZ(inputState.getMousePos(), ec, ec.dragStartWorldPos.z);
		glm::vec2 streamStartWorldPosPlaneZ = glm::vec2(ec.dragStartWorldPos.x, ec.dragStartWorldPos.y);
		glm::vec2 gridStartWorldPosPlaneZ = glm::vec2(ec.dragGridWorldPos.x, ec.dragGridWorldPos.y);
		glm::vec2 streamDeltaWorldPosPlaneZ = currentWorldPosPlaneZ - streamStartWorldPosPlaneZ;
		glm::vec2 gridDeltaWorldPosPlaneZ = currentWorldPosPlaneZ - gridStartWorldPosPlaneZ;
		int dx = static_cast<int>(std::round(gridDeltaWorldPosPlaneZ.x));
		int dy = static_cast<int>(std::round(gridDeltaWorldPosPlaneZ.y));
		const bool hasFloatMove = hasSelectedStreamNode &&
			(std::fabs(streamDeltaWorldPosPlaneZ.x) > CurvePathDetail::TIME_EPSILON ||
			 std::fabs(streamDeltaWorldPosPlaneZ.y) > CurvePathDetail::TIME_EPSILON);
		if (hasFloatMove || dx != 0 || dy != 0) {
			// update note position
			changed = changeSelectedNotesByDeltaInternal(ec, beatmap, beatmapStack, streamDeltaWorldPosPlaneZ.x, streamDeltaWorldPosPlaneZ.y, dx, dy, 0.0, false) || changed;
			if (hasFloatMove) {
				ec.dragStartWorldPos = glm::vec3(currentWorldPosPlaneZ, ec.dragStartWorldPos.z);
			}
			if (dx != 0 || dy != 0) {
				ec.dragGridWorldPos = glm::vec3(ec.dragGridWorldPos.x + dx, ec.dragGridWorldPos.y + dy, ec.dragGridWorldPos.z);
			}
		}
	}
	else if (ec.isDraggingNotes2d && duringDragInput) {
		glm::vec2 currentWorldPosPlaneY = mouseToWorldOnPlaneY(inputState.getMousePos(), ec, ec.dragStartWorldPos.y);
		glm::vec2 streamStartWorldPosPlaneY = glm::vec2(ec.dragStartWorldPos.x, ec.dragStartWorldPos.z);
		glm::vec2 gridStartWorldPosPlaneY = glm::vec2(ec.dragGridWorldPos.x, ec.dragGridWorldPos.z);
		glm::vec2 streamDeltaWorldPosPlaneY = currentWorldPosPlaneY - streamStartWorldPosPlaneY;
		glm::vec2 gridDeltaWorldPosPlaneY = currentWorldPosPlaneY - gridStartWorldPosPlaneY;
		int dx = static_cast<int>(std::round(gridDeltaWorldPosPlaneY.x));
		int dz = static_cast<int>(std::round((ec.subbeat * gridDeltaWorldPosPlaneY.y) / ec.distPerBeat));	//  how many subbeat moved
		const bool hasFloatMove = hasSelectedStreamNode &&
			std::fabs(streamDeltaWorldPosPlaneY.x) > CurvePathDetail::TIME_EPSILON;
		if (hasFloatMove || dx != 0 || dz != 0) {
			double dt = -dz * beatmap.getsecPerBeat() / ec.subbeat;
			// update note position
			changed = changeSelectedNotesByDeltaInternal(ec, beatmap, beatmapStack, streamDeltaWorldPosPlaneY.x, 0.0f, dx, 0, dt, false) || changed;
			if (hasFloatMove) {
				ec.dragStartWorldPos = glm::vec3(currentWorldPosPlaneY.x, ec.dragStartWorldPos.y, currentWorldPosPlaneY.y);
			}
			if (dx != 0 || dz != 0) {
				ec.dragGridWorldPos = glm::vec3(
					ec.dragGridWorldPos.x + dx,
					ec.dragGridWorldPos.y,
					ec.dragGridWorldPos.z + ((dz * ec.distPerBeat) / ec.subbeat));
			}
		}
	}
	else {
		ec.isDraggingNotes3d = false;
		ec.isDraggingNotes2d = false;
		ec.dragStartWorldPos = glm::vec3(0.0f);
		ec.dragGridWorldPos = glm::vec3(0.0f);
	}
	return changed;
}

bool Editor::putNote(EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack, const InputState& inputState) {
	bool isAlreadyNote = false;
	double noteTime = 0.0f;
	int putx = 0, puty = 0;

	if (ec.isHoveredContent3d) {
		//calc put position in world from mouse position
		glm::vec2 putPosXY = mouseToWorldOnPlaneZ(inputState.getMousePos(), ec, ec.editingZ);
		putx = static_cast<int>(std::round(putPosXY.x)), puty = static_cast<int>(std::round(putPosXY.y)); // snap to int grid

		if (ec.isNotesPlaceConstraint) {
			glm::ivec2 constrainedPos = constrainNotePosXY(glm::ivec2(putx, puty), ec);
			putx = constrainedPos.x;
			puty = constrainedPos.y;
		}

		//check is there already note in drawn area
		if (isThereAlreadyNote(glm::vec3(putx, puty, ec.editingZ), ec)) {
			isAlreadyNote = true;
		}
		else {
			noteTime = calcNoteTimefromPosZ(ec.editingZ, beatmap, ec);
		}
	}
	else if (ec.isHoveredContent2d) {
		//calc put position in world from mouse position
		glm::vec2 putPosXZ = mouseToWorldOnPlaneY(inputState.getMousePos(), ec, 0.0f);
		putx = static_cast<int>(std::round(putPosXZ.x)); // snap to int grid
		noteTime = calcNoteTimefromPosZ(putPosXZ.y, beatmap, ec);
		noteTime = calcTimeWithBeat(noteTime, beatmap, ec); // snap to subbeat
		float posZ = calcNotePosZ(noteTime, beatmap, ec);

		if (ec.isNotesPlaceConstraint) {
			glm::ivec2 constrainedPos = constrainNotePosXY(glm::ivec2(putx, puty), ec);
			putx = constrainedPos.x;
			puty = constrainedPos.y;
		}

		if (isThereAlreadyNote(glm::vec3(putx, puty, posZ), ec)) {
			isAlreadyNote = true;
		}
	}

	//putting note
	if (!isAlreadyNote) {
		//save stack for undo
		beatmapStack.push(beatmap);
		ec.hitHandPlayer.playFromStart();
		clearSelection(ec, false);

		switch (ec.editMode) {
		case EditMode::LEFT_HAND:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::LEFT_HAND));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::RIGHT_HAND:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::RIGHT_HAND));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::LEFT_FOOT:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::LEFT_FOOT));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::RIGHT_FOOT:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::RIGHT_FOOT));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::BAR:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::BAR));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::TRAP:
			beatmap.addNote(Note(noteTime, putx, puty, NoteType::TRAP));
			ec.groupNotesEditing = 0;
			break;
		case EditMode::STREAM:
			if (ec.groupNotesEditing == 0) {
				GroupNotes streamTemplate(GroupType::DEFAULT);
				if (ec.streamNoteType == NoteType::OBS) {
					streamTemplate = GroupNotes(0.0, ec.obstaclePutLength * beatmap.getsecPerBeat() / ec.subbeat, 0, 0);
				} 
				else {
					streamTemplate.addNote(Note(0.0, 0, 0, ec.streamNoteType));
				}
				GroupNotes& addedTemplate = beatmap.addStreamGroupNotes(streamTemplate);

				const std::vector<uint64_t> streamTemplateGroupIDs = { addedTemplate.getGroupID() };
				ec.groupNotesEditing = beatmap.addStreamInstance(
					streamTemplateGroupIDs,
					ec.streamRepeatIntervalSec,
					Note(noteTime, putx, puty, NoteType::STREAM_NODE)).getGroupID();
			}
			else {
				GroupNotes& editingGroup = beatmap.getGroupNotesWithGroupID(ec.groupNotesEditing);
				if (editingGroup.getGroupType() == GroupType::STREAM) {
					beatmap.addStreamNode(
						ec.groupNotesEditing,
						Note(noteTime, putx, puty, NoteType::STREAM_NODE),
						ec.streamRepeatIntervalSec);
				}
			}
			break;
		case EditMode::LONG_LEFT_FOOT:

			// put first notes of longnotes
			if (ec.groupNotesEditing == 0) {
				GroupNotes llf(GroupType::LONG_NOTE);
				llf.addLongNotesNode(Note(noteTime, putx, puty, NoteType::LONG_LEFT_FOOT));
				ec.groupNotesEditing = beatmap.addGroupNotes(llf).getGroupID();
			}
			else {	// put second or later note
				GroupNotes& editingGroup = beatmap.getGroupNotesWithGroupID(ec.groupNotesEditing);
				if (editingGroup.getGroupType() == GroupType::LONG_NOTE && editingGroup.getNoteType() == NoteType::LONG_LEFT_FOOT) {
					editingGroup.addLongNotesNode(Note(noteTime, putx, puty, NoteType::LONG_LEFT_FOOT));
				}
				else {
					//std::cerr << "[Editor.cpp] doesn't match editing group to long left foot" << std::endl;
				}
			}
			break;
		case EditMode::LONG_RIGHT_FOOT:

			// put first notes of longnotes
			if (ec.groupNotesEditing == 0) {
				GroupNotes lrf(GroupType::LONG_NOTE);
				lrf.addLongNotesNode(Note(noteTime, putx, puty, NoteType::LONG_RIGHT_FOOT));
				ec.groupNotesEditing = beatmap.addGroupNotes(lrf).getGroupID();
			}
			else {	// put second or later note
				GroupNotes& editingGroup = beatmap.getGroupNotesWithGroupID(ec.groupNotesEditing);
				if (editingGroup.getGroupType() == GroupType::LONG_NOTE && editingGroup.getNoteType() == NoteType::LONG_RIGHT_FOOT) {
					editingGroup.addLongNotesNode(Note(noteTime, putx, puty, NoteType::LONG_RIGHT_FOOT));
				}
				else {
					//std::cerr << "[Editor.cpp] doesn't match editing group to long right foot" << std::endl;
				}
			}
			break;
		case EditMode::OBSTACLE:
			beatmap.addObstacleGroupNotes(noteTime, (ec.obstaclePutLength * beatmap.getsecPerBeat() / ec.subbeat), putx, puty);
			ec.groupNotesEditing = 0;
			break;

		default:
			//std::cerr << "[Editor.cpp] Edit mode is unexpected in put single note" << std::endl;
			break;
		}
		return true;
	}
	return false;
}

void Editor::startRectSelect(EditorContext& ec, const InputState& inputState) {
	// start multiple select
	if (ec.isHoveredContent3d)		ec.duringRectSelect3d = true;
	else if (ec.isHoveredContent2d)	ec.duringRectSelect2d = true;
	else 						return;

	ec.rectSelectStartPos = inputState.getMousePos();
}

void Editor::updateRectSelect(EditorContext& ec, Beatmap& beatmap, const InputState& inputState) {
	bool rectSelectInput = (inputState.isActionDown(Action::SelectPut) || inputState.isActionDown(Action::MultipleSelect));

	if (ec.duringRectSelect2d && rectSelectInput && ec.isHoveredContent2d) {
		ec.rectSelectEndPos = inputState.getMousePos();
	}
	else if (ec.duringRectSelect3d && rectSelectInput && ec.isHoveredContent3d) {
		ec.rectSelectEndPos = inputState.getMousePos();
	}
	else if (ec.duringRectSelect2d || ec.duringRectSelect3d) {
		ec.rectSelectEndPos = inputState.getMousePos();
		scanNotesInRect(ec, beatmap);
		ec.duringRectSelect3d = false;
		ec.duringRectSelect2d = false;
		ec.rectSelectStartPos = ImVec2(0.0f, 0.0f);
		ec.rectSelectEndPos = ImVec2(0.0f, 0.0f);
	}
}

void Editor::scanNotesInRect(EditorContext& ec, Beatmap& beatmap) {
	if (!ec.duringRectSelect2d && !ec.duringRectSelect3d) {
		return;
	}


	//calc distance moved distance with song playing
	ec.laneMoveDistance = static_cast<float>((ec.currentPlayerTime - beatmap.getActualOffset()) / beatmap.getsecPerBeat() * ec.distPerBeat);
	
	float drawMaxDistance = (std::max)(ec.perspectiveDrawMaxDistance, ec.orthoDrawMaxDistance);
	float drawMinDistance = (std::min)(ec.perspectiveDrawMinDistance, ec.orthoDrawMinDistance);

	glm::mat4 matPV;
	ImVec2 contentOrigin;
	int contentWidth, contentHeight;
	if (ec.duringRectSelect3d) {
		matPV = ec.projectionPerspective * ec.viewPerspective;
		contentOrigin = ec.contentOrigin3d;
		contentWidth = ec.psFBOWidth;
		contentHeight = ec.psFBOHeight;
	} else {
		matPV = ec.projectionOrtho * ec.viewOrtho;
		contentOrigin = ec.contentOrigin2d;
		contentWidth = ec.orFBOWidth;
		contentHeight = ec.orFBOHeight;
	}

	ImVec2 rectLeftTop = ImVec2((std::min)(ec.rectSelectStartPos.x, ec.rectSelectEndPos.x), (std::min)(ec.rectSelectStartPos.y, ec.rectSelectEndPos.y));
	ImVec2 rectRightBottom = ImVec2((std::max)(ec.rectSelectStartPos.x, ec.rectSelectEndPos.x), (std::max)(ec.rectSelectStartPos.y, ec.rectSelectEndPos.y));

	//single notes
	for (size_t n = 0; n < beatmap.getGroupNotesSize(); n++) {	//for each notes
		Note& note = beatmap.getNoteAtIndex(n);
		NoteType type = note.getType();		//note type
		glm::vec3 notePos = calcNotePos(note, beatmap, ec);


		if (notePos.z < -drawMaxDistance) {	// when it is too far
			break;
		}
		else if (notePos.z > drawMinDistance) { // when it is completely back side (already passsed)
			continue;
		}

		if (type == NoteType::BAR) {
			for (float r = 0.0f; r < ec.barScale.x; r += ec.hitRadius) {
				glm::vec3 collisionPos = notePos;
				collisionPos.x += r - ec.barScale.x / 2;
				ImVec2 screenPos = worldToScreen(collisionPos, matPV, contentOrigin, contentWidth, contentHeight);
				if (isInRect(screenPos, rectLeftTop, rectRightBottom)) {
					selectNote(&note, ec, beatmap, true, false);
					break;
				}
			}
		}
		else {
			ImVec2 screenPos = worldToScreen(notePos, matPV, contentOrigin, contentWidth, contentHeight);
			if (isInRect(screenPos, rectLeftTop, rectRightBottom)) {
				selectNote(&note, ec, beatmap, true, false);
			}
		}
	}


	//group Notes
	for (size_t n = 0; n < beatmap.getVecGroupNotesLength(); n++) {
		GroupNotes& gn = beatmap.getGroupNotesAtIndex(n);
		float startPosZ = calcNotePosZ(gn.getStartTime(), beatmap, ec);
		float endPosZ = calcNotePosZ(gn.getEndTime(), beatmap, ec);

		//check the group is in draw range
		if (startPosZ < -drawMaxDistance) {
			break;
		}
		else if (endPosZ > drawMinDistance) {
			continue;
		}

		GroupType groupType = gn.getGroupType();
		switch (groupType) {
		case GroupType::LONG_NOTE:

		{
			NoteType noteType = gn.getNoteType();
			std::vector<glm::vec3> pointList;
			switch (noteType) {

			case NoteType::LONG_LEFT_FOOT:
			case NoteType::LONG_RIGHT_FOOT:
				for (size_t i = 0; i < gn.size(); i++) {
					Note& note = gn.getNoteAtIndex(i);
					glm::vec3 notePos = calcNotePos(note, beatmap, ec);

					ImVec2 screenPos = worldToScreen(notePos, matPV, contentOrigin, contentWidth, contentHeight);
					if (isInRect(screenPos, rectLeftTop, rectRightBottom)) {
						selectNote(&note, ec, beatmap, true, false);
					}
				}
				break;
			default:
				break;
			}
		}
		break;

		case GroupType::OBSTACLE:

			if (gn.size() == 2) {

				Note& startNote = gn.getNoteAtIndex(0); // get first note (start note)
				glm::vec3 startNotePos = calcNotePos(startNote, beatmap, ec);
				
				bool isSelected = false;
				for (float checkZ = startNotePos.z; checkZ >= endPosZ; checkZ -= ec.hitRadius) {
					if (isSelected) {
						break;
					}
					for (float h = 0.0f; h < ec.obsScale.y; h += ec.hitRadius) {
						glm::vec3 checkPos = startNotePos;
						checkPos.y += h;
						checkPos.z = checkZ;

						ImVec2 screenPos = worldToScreen(checkPos, matPV, contentOrigin, contentWidth, contentHeight);
						if (isInRect(screenPos, rectLeftTop, rectRightBottom)) {
							selectNote(&startNote, ec, beatmap, true, false);
							isSelected = true;
							break;
						}
					}
				}

			}
			break;

		default:
			break;
		}

	}
}
