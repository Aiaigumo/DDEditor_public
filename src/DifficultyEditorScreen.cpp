#include "DifficultyEditorScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <glm.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "EditorUtility.h"
#include "TimingList.h"

inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x + b.x, a.y + b.y);
}

inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
	return ImVec2(a.x - b.x, a.y - b.y);
}

DifficultyEditorScreen::DifficultyEditorScreen(
	EditorContext& editorContext,
	Beatmap& beatmap,
	BeatmapInfo& bmInfo,
	BeatmapStack& beatmapStack,
	Editor& editor,
	InputState& inputState,
	Renderer& perspectiveRenderer,
	Renderer& orthoRenderer
) :
	editorContext(editorContext),
	beatmap(beatmap),
	bmInfo(bmInfo),
	beatmapStack(beatmapStack),
	editor(editor),
	inputState(inputState),
	perspectiveRenderer(perspectiveRenderer),
	orthoRenderer(orthoRenderer) {
}

void DifficultyEditorScreen::markDifficultyDirty() {
	dirty = true;
	saveStatusMessage.clear();
}

void DifficultyEditorScreen::clearDifficultyDirty() {
	dirty = false;
}

bool DifficultyEditorScreen::hasUnsavedChanges() const {
	return dirty;
}

void DifficultyEditorScreen::requestCloseConfirmation() {
	closeConfirmationRequested = true;
}

bool DifficultyEditorScreen::tryGetCurrentDifficultyPath(std::filesystem::path& path) const {
	const int diffIndex = editorContext.currentDiffIndex;
	if (diffIndex < 0 || diffIndex >= BeatmapInfo::LEVEL_NUM) {
		return false;
	}
	if (!bmInfo.getExistDiff(diffIndex)) {
		return false;
	}
	const std::string diffPath = bmInfo.getDiffPath(diffIndex);
	if (diffPath.empty() || editorContext.beatmapFolderPathBuffer.empty()) {
		return false;
	}

	path = std::filesystem::path(editorContext.beatmapFolderPathBuffer) / diffPath;
	return true;
}

bool DifficultyEditorScreen::saveCurrentDifficulty() {
	if (!beatmap.isLoaded()) {
		saveStatusMessage = "No difficulty is loaded.";
		return false;
	}

	std::filesystem::path difficultyPath;
	if (!tryGetCurrentDifficultyPath(difficultyPath)) {
		saveStatusMessage = "Current difficulty save path is invalid.";
		return false;
	}

	if (!beatmap.exportJson(difficultyPath.string(), true)) {
		saveStatusMessage = "Failed to save difficulty.";
		return false;
	}

	clearDifficultyDirty();
	saveStatusMessage = "Saved difficulty successfully.";
	return true;
}

bool DifficultyEditorScreen::isProjectSingleBpmMode() const {
	return bmInfo.getProjectBpmMode() == ProjectBpmMode::Single;
}

const char* DifficultyEditorScreen::getProjectBpmModeText() const {
	return isProjectSingleBpmMode() ? "Single" : "Multiple";
}

const char* DifficultyEditorScreen::getDifficultyBpmModeText() const {
	return beatmap.getBpmExportMode() == BpmExportMode::Single ? "Single" : "Multiple";
}

bool DifficultyEditorScreen::isDifficultyBpmModeConsistentWithProject() const {
	if (isProjectSingleBpmMode()) {
		return beatmap.getBpmExportMode() == BpmExportMode::Single;
	}
	return beatmap.getBpmExportMode() == BpmExportMode::Multiple;
}

void DifficultyEditorScreen::refreshBpmConsistencyMessages() {
	bpmModeConsistencyMessage.clear();
	bpmValueConsistencyMessage.clear();

	if (!isDifficultyBpmModeConsistentWithProject()) {
		bpmModeConsistencyMessage =
			"Difficulty BPM mode does not match the project BPM mode. Re-export or change the BPM mode on the BeatmapInfo screen.";
		return;
	}

	if (isProjectSingleBpmMode()) {
		if (beatmap.getTimingListSize() != 1) {
			bpmValueConsistencyMessage =
				"Single BPM mode requires exactly one timing in this difficulty.";
			return;
		}
		if (std::fabs(beatmap.getFirstBPM() - bmInfo.getBPM()) > 0.001f) {
			bpmValueConsistencyMessage =
				"First timing BPM does not match the project BPM. Update project BPM on the BeatmapInfo screen before export.";
		}
		return;
	}

	if (std::fabs(beatmap.getRawExportBPM() - bmInfo.getBPM()) > 0.001f) {
		bpmValueConsistencyMessage =
			"Difficulty Export BPM does not match the project BPM. Re-export or update project BPM on the BeatmapInfo screen.";
	}
}

bool DifficultyEditorScreen::canAcceptViewerEditInput() const {
	const bool hasBlockingPopup = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
	return !hasBlockingPopup && (canEditInViewer3d || canEditInViewer2d);
}

void DifficultyEditorScreen::onEnterScreen() {
	clearDifficultyDirty();
	saveStatusMessage.clear();
	addTimingPolicyMessage.clear();
	refreshBpmConsistencyMessages();
}

void DifficultyEditorScreen::onExitScreen() {
	editorContext.songPlayer.stop();
	editorContext.isSongPlaying = false;
}

void DifficultyEditorScreen::syncPlaybackAndTiming() {
	editorContext.currentPlayerTime = editorContext.songPlayer.getCurrentTime();
	editor.updateTimingState(editorContext, beatmap);
	editorContext.isSongPlaying = editorContext.songPlayer.isPlaying();
}

ScreenResult DifficultyEditorScreen::runFrame() {
	syncPlaybackAndTiming();

	ScreenResult result;

	// ==== Navigation UI ====
	{
		ImGui::Begin("Navigation");
		if (ImGui::Button("Save Difficulty")) {
			saveCurrentDifficulty();
		}
		if (dirty) {
			ImGui::SameLine();
			ImGui::TextUnformatted("(Unsaved)");
		}
		if (!saveStatusMessage.empty()) {
			ImGui::TextWrapped("%s", saveStatusMessage.c_str());
		}
		if (ImGui::Button("Back to Beatmap Info")) {
			if (dirty) {
				ImGui::OpenPopup("Unsaved Difficulty Changes");
			}
			else {
				result.hasScreenChangeRequest = true;
				result.nextScreen = AppScreen::BeatmapInfo;
			}
		}
		ImGui::Separator();
		if (ImGui::Button("Clear Notes##Clear_Beatmap")) {
			ImGui::OpenPopup("Confirm Clear Notes");
		}

		if (ImGui::BeginPopupModal("Unsaved Difficulty Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("This difficulty has unsaved changes.");
			ImGui::Separator();

			if (ImGui::Button("Save", ImVec2(120, 0))) {
				if (saveCurrentDifficulty()) {
					result.hasScreenChangeRequest = true;
					result.nextScreen = AppScreen::BeatmapInfo;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard", ImVec2(120, 0))) {
				clearDifficultyDirty();
				result.hasScreenChangeRequest = true;
				result.nextScreen = AppScreen::BeatmapInfo;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			if (!saveStatusMessage.empty()) {
				ImGui::Separator();
				ImGui::TextWrapped("%s", saveStatusMessage.c_str());
			}

			ImGui::EndPopup();
		}

		drawCloseConfirmationModal(result);

		// pop up modal for confirm clear beatmap
		if (ImGui::BeginPopupModal("Confirm Clear Notes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("All notes exists will be deleted.\nAre you sure?");
			ImGui::Separator();

			if (ImGui::Button("OK", ImVec2(120, 0))) {
				if (editor.clearAllNotes(editorContext, beatmap, beatmapStack)) {
					markDifficultyDirty();
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();
		if (result.hasScreenChangeRequest || result.hasApplicationCloseRequest) {
			return result;
		}
	}

	// =============================

	// ==== Audio SongPlayer UI ====
	{
		ImGui::Begin("Song Player");

		if (!editorContext.loadedSongFilePath.empty()) {
			const std::filesystem::path loadedSongPath = editorContext.loadedSongFilePath;
			ImGui::Text("Loaded Song: %s", loadedSongPath.filename().string().c_str());
		}
		else {
			ImGui::TextUnformatted("No song loaded.");
		}

		//seek slider
		editorContext.songLength = editorContext.songPlayer.getLength();

		//seek control by wheel
		if (inputState.isActionDown(Action::SeekForward) || inputState.isActionDown(Action::SeekRewind)) {
			int wheel = 0;
			if (inputState.isActionDown(Action::SeekForward)) {
				wheel += 1;
			}
			else if (inputState.isActionDown(Action::SeekRewind)) {
				wheel -= 1;
			}
			int beat = beatmap.getCurrentBeat() + wheel;
			editorContext.currentPlayerTime = beatmap.calcCurrentTime(beat);
			editorContext.currentPlayerTime = std::clamp(editorContext.currentPlayerTime, 0.0, editorContext.songLength);

			editor.updateSeek(editorContext.songPlayer, editorContext, beatmap);
		}

		//seek slider UI
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 20));
		ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 5.0f);
		float sliderWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PushItemWidth(sliderWidth);
		double minSeek = 0.0;
		double maxSeek = editorContext.songLength;
		if (ImGui::SliderScalar("  Seek", ImGuiDataType_Double, &editorContext.currentPlayerTime, &minSeek, &maxSeek, "%.3f"))
		{
			editor.updateSeek(editorContext.songPlayer, editorContext, beatmap);
		}
		ImGui::PopItemWidth();
		ImGui::PopStyleVar(2);

		//play/stop button
		if (editorContext.isSongPlaying) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 1.0f, 1.0f));
		}

		if (ImGui::Button(editorContext.isSongPlaying ? "Stop" : "Play") || inputState.isActionPressed(Action::PlayToggle))
		{
			if (editorContext.isSongPlaying)
				editorContext.songPlayer.stop();
			else
				editorContext.songPlayer.play();
		}

		if (editorContext.isSongPlaying) {
			ImGui::PopStyleColor();
		}

		// Volume slider
		ImGui::SameLine();
		if (ImGui::SliderFloat("Volume", &editorContext.songVolume, 0.0f, 1.0f))
		{
			editorContext.songPlayer.setVolume(editorContext.songVolume);
		}

		if (ImGui::SliderFloat("Hitsound Volume", &editorContext.hitSoundVolume, 0.0f, 1.0f))
		{
			editorContext.hitHandPlayer.setVolume(editorContext.hitSoundVolume);
			editorContext.hitFootPlayer.setVolume(editorContext.hitSoundVolume);
		}

		ImGui::End();
	}
	// ==========================

	// ==== Timing && BPM UI ====
	{
		ImGui::Begin("Timing");

		ImGui::Text("Project BPM Mode: %s", getProjectBpmModeText());
		ImGui::Text("Difficulty BPM Mode: %s", getDifficultyBpmModeText());
		if (!bpmModeConsistencyMessage.empty()) {
			ImGui::TextWrapped("%s", bpmModeConsistencyMessage.c_str());
		}
		if (!bpmValueConsistencyMessage.empty()) {
			ImGui::TextWrapped("%s", bpmValueConsistencyMessage.c_str());
		}
		if (!addTimingPolicyMessage.empty()) {
			ImGui::TextWrapped("%s", addTimingPolicyMessage.c_str());
		}

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 1.0f, 0.7f));
		if (ImGui::Button("Add Timing in the current position")) {
			if (isProjectSingleBpmMode()) {
				addTimingPolicyMessage = "Single BPM mode allows only one timing. Switch to Multiple on the BeatmapInfo screen to add timings.";
			}
			else {
				if (editor.addTimingAtCurrentTime(editorContext, beatmap, beatmapStack)) {
					markDifficultyDirty();
					refreshBpmConsistencyMessages();
				}
				addTimingPolicyMessage.clear();
			}
		}
		ImGui::PopStyleColor();

		if (beatmap.timingListisEmpty()) {
			ImGui::Text("Please load difficulty and song");
		}
		else {
			// draw timing list
			{
				const TimingList& timingList = beatmap.getTimingList();

				ImGui::PushItemWidth(editorContext.itemWidth7char);

				//draw each timing
				for (size_t i = 0; i < beatmap.getTimingListSize(); i++) {

					ImGui::PushID((int)i);


					float bpm = timingList.getBPM(i);
					double offset = timingList.getOffset(i);

					std::string timingLabel = "Timing " + std::to_string(i + 1);

					bool isCurrentTiming = (i == beatmap.getCurrentTimingIndex());

					if (isCurrentTiming) {
						timingLabel += " (Current)";
					}

					if (ImGui::CollapsingHeader(timingLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

						bool bpmChanged = false, offsetChanged = false;

						ImGui::Text("BPM");
						if (ImGui::Button("-1##BPM_-sChange")) {
							bpm -= 1.0f;
							bpmChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("-10##BPM_-lChange")) {
							bpm -= 10.0f;
							bpmChanged = true;
						}
						ImGui::SameLine();	ImGui::SetNextItemWidth(editorContext.itemWidthSlider);
						if (ImGui::SliderFloat("##BPM_Slider", &bpm, 30.0f, 300.0f, "%.2f BPM")) {
							if (ImGui::IsItemActive())
							{
								bpm = floorf(bpm);
							}
							bpmChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+10##BPM_+lChange")) {
							bpm += 10.0f;
							bpmChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+1##BPM_+sChange")) {
							bpm += 1.0f;
							bpmChanged = true;
						}

						if (bpmChanged) {
							if (editor.setTimingBPM(i, editorContext, beatmap, beatmapStack, bpm)) {
								markDifficultyDirty();
								refreshBpmConsistencyMessages();
							}
						}

						ImGui::Text("Offset");
						if (ImGui::Button("-0.01##Offset_-sChange")) {
							offset -= 0.01;
							offsetChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("-0.1##Offset_-lChange")) {
							offset -= 0.1;
							offsetChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::InputDouble("##Offset", &offset, 0, 0, "%.3f")) {
							offsetChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+0.1##Offset_+lChange")) {
							offset += 0.1;
							offsetChanged = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+0.01##Offset_+sChange")) {
							offset += 0.01;
							offsetChanged = true;
						}

						if (offsetChanged) {
							if (editor.setTimingOffset(i, editorContext, beatmap, beatmapStack, offset)) {
								markDifficultyDirty();
								ImGui::PopID();
								break;
							}
						}

						int subbeat = timingList.getSubbeat(i);
						ImGui::SameLine();
						ImGui::Text("subbeat: ");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(editorContext.itemWidth3char + editorContext.itemWidthStep);
						if (ImGui::InputInt("##subbeat", &subbeat, 1)) {
							if (editor.setTimingSubbeat(i, editorContext, beatmap, beatmapStack, subbeat)) {
								markDifficultyDirty();
							}
						}

						//delete timing button
						if (i != 0) // prevent deleting the first timing
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f));
							if (ImGui::Button("Delete This Timing")) {
								if (editor.deleteTiming(i, editorContext, beatmap, beatmapStack)) {
									markDifficultyDirty();
									refreshBpmConsistencyMessages();
								}
								//when deleting current timing, reference of timingList is invalid, so break here
								ImGui::PopStyleColor();
								ImGui::PopID();
								break;
							}
							ImGui::PopStyleColor();
						}

					}

					ImGui::PopID();

				}
			}

			ImGui::Separator();

			bool hasMultipleTiming = beatmap.getTimingListSize() > 1;
			if (hasMultipleTiming) {

				float exportBPM = beatmap.getRawExportBPM();
				bool exportBPMChanged = false;
				ImGui::Text("Export BPM");
				const bool disableExportBpmEdit = isProjectSingleBpmMode();
				if (disableExportBpmEdit) {
					ImGui::TextWrapped("Export BPM is controlled by the first timing in Single BPM mode.");
				}
				ImGui::BeginDisabled(disableExportBpmEdit);
				if (ImGui::Button("-1##ExportBPM_-sChange")) {
					exportBPM -= 1.0f;
					exportBPMChanged = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("-10##ExportBPM_-lChange")) {
					exportBPM -= 10.0f;
					exportBPMChanged = true;
				}
				ImGui::SameLine();	ImGui::SetNextItemWidth(editorContext.itemWidthSlider);
				if (ImGui::SliderFloat("##ExportBPM_Slider", &exportBPM, 30.0f, 300.0f, "%.2f BPM")) {
					if (ImGui::IsItemActive())
					{
						exportBPM = floorf(exportBPM);
					}
					exportBPMChanged = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("+10##ExportBPM_+lChange")) {
					exportBPM += 10.0f;
					exportBPMChanged = true;
				}
				ImGui::SameLine();
				if (ImGui::Button("+1##ExportBPM_+sChange")) {
					exportBPM += 1.0f;
					exportBPMChanged = true;
				}
				ImGui::EndDisabled();

				if (!disableExportBpmEdit && exportBPMChanged) {
					if (editor.setExportBPM(editorContext, beatmap, beatmapStack, exportBPM)) {
						markDifficultyDirty();
						refreshBpmConsistencyMessages();
					}
				}
			}

			double offsetInEditor = beatmap.getOffsetInEditor();
			bool offsetInEditorChanged = false;
			ImGui::Text("Offset of DDEditor");
			if (ImGui::Button("-0.01##OffsetIE_-sChange")) {
				offsetInEditor -= 0.01;
				offsetInEditorChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("-0.1##OffsetIE_-lChange")) {
				offsetInEditor -= 0.1;
				offsetInEditorChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::InputDouble("##OffsetIE_Slider", &offsetInEditor, 0, 0, "%.3f")) {
				offsetInEditorChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("+0.1##OffsetIE_+lChange")) {
				offsetInEditor += 0.1;
				offsetInEditorChanged = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("+0.01##OffsetIE_+sChange")) {
				offsetInEditor += 0.01;
				offsetInEditorChanged = true;
			}

			if (offsetInEditorChanged) {
				editor.setEditorOffset(editorContext, beatmap, offsetInEditor);
			}

			ImGui::PopItemWidth();

			ImGui::Separator();
			ImGui::SetNextItemWidth(editorContext.itemWidthSlider);
			if (ImGui::SliderFloat("Metronome Volume", &editorContext.metronomeVolume, 0.0f, 1.0f))
			{
				editorContext.metronomePlayer.setVolume(editorContext.metronomeVolume);
			}
			editor.updateBeat(editorContext, beatmap);

			int currentBeat = beatmap.getCurrentBeat();
			ImGui::Text("Beat: %d", currentBeat);

		}
		ImGui::End();
	}
	// ==========================

	// ===== Editor UI =====
	{
		ImGui::Begin("Editor");
		// Edit mode selection
		const char* editModeItems[] = {
			"Left Hand",
			"Right Hand",
			"Left Foot",
			"Right Foot",
			"Long Left Foot",
			"Long Right Foot",
			"Bar",
			"Obstacle",
			"Trap",
			"Stream",
			"Move",
		};
		if (ImGui::BeginTable("RadioTable", 2,
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
		{
			for (int i = 0; i < IM_ARRAYSIZE(editModeItems); i++) {
				if (i % 2 == 0) {
					ImGui::TableNextRow();
				}
				ImGui::TableSetColumnIndex((i % 2));
				if (ImGui::RadioButton(editModeItems[i], editorContext.editMode == static_cast<EditMode>(i))) {
					editorContext.editMode = static_cast<EditMode>(i);
					editorContext.groupNotesEditing = 0;		// reset editing group note
				}

			}
			ImGui::EndTable();
		}

		ImGui::PushItemWidth(80);
		// Obstacle length input
		if (editorContext.editMode == EditMode::OBSTACLE || 
			(editorContext.editMode == EditMode::STREAM && editorContext.streamNoteType == NoteType::OBS)) {

			ImGui::InputInt("Obstacle Length", &editorContext.obstaclePutLength);
			if (editorContext.obstaclePutLength < 1) editorContext.obstaclePutLength = 1;
		}
		ImGui::PopItemWidth();

		if (editorContext.editMode == EditMode::STREAM) {
			const char* streamNoteTypeItems[] = {
				"Left Hand",
				"Right Hand",
				"Left Foot",
				"Right Foot",
				"Bar",
				"Trap",
				"Obstacle"
			};
			const NoteType streamNoteTypes[] = {
				NoteType::LEFT_HAND,
				NoteType::RIGHT_HAND,
				NoteType::LEFT_FOOT,
				NoteType::RIGHT_FOOT,
				NoteType::BAR,
				NoteType::TRAP,
				NoteType::OBS
			};
			int currentStreamTypeIndex = 0;
			for (int i = 0; i < IM_ARRAYSIZE(streamNoteTypes); ++i) {
				if (editorContext.streamNoteType == streamNoteTypes[i]) {
					currentStreamTypeIndex = i;
					break;
				}
			}
			if (ImGui::Combo("Stream Note Type", &currentStreamTypeIndex, streamNoteTypeItems, IM_ARRAYSIZE(streamNoteTypeItems))) {
				editorContext.streamNoteType = streamNoteTypes[currentStreamTypeIndex];
			}
			double streamIntervalStep = beatmap.getsecPerBeat() / static_cast<double>(beatmap.getSubbeat());
			if (ImGui::InputDouble("Stream Interval", &editorContext.streamRepeatIntervalSec, streamIntervalStep, 0.0, "%.3f s")) {
				editor.updateTimingState(editorContext, beatmap);
			}
		}

		// Notes placement constraint toggle
		ImGui::Checkbox("Notes Placement Constraint", &editorContext.isNotesPlaceConstraint);

		ImGui::End();
	}
	// ==========================

	// ======== Viewer Properties ========
	{
		ImGui::Begin("Viewer Properties");

#ifdef DEBUG

		ImGui::Text("isHovered3d: %s", editorContext.isHovered3d ? "true" : "false");
		ImGui::Text("isHovered2d: %s", editorContext.isHovered2d ? "true" : "false");
		ImGui::Text("isHoveredContent3d: %s", editorContext.isHoveredContent3d ? "true" : "false");
		ImGui::Text("isHoveredContent2d: %s", editorContext.isHoveredContent2d ? "true" : "false");
		ImGui::Text("duringRectSelect3d: %s", editorContext.duringRectSelect3d ? "true" : "false");
		ImGui::Text("duringRectSelect2d: %s", editorContext.duringRectSelect2d ? "true" : "false");
		ImGui::Text("rectSelectStartPos: (%.1f, %.1f)", editorContext.rectSelectStartPos.x, editorContext.rectSelectStartPos.y);
		ImGui::Text("rectSelectEndPos: (%.1f, %.1f)", editorContext.rectSelectEndPos.x, editorContext.rectSelectEndPos.y);
		ImGui::Checkbox("Axis", &editorContext.drawAxis); // Placeholder for wireframe mode

#endif // DEBUG

		//property of GroupNotes
		if (editorContext.groupNotesEditing != 0) {
			GroupNotes& editingGroup = beatmap.getGroupNotesWithGroupID(editorContext.groupNotesEditing);
			ImGui::PushID("GroupNotes");
			if (ImGui::TreeNodeEx("Editing GroupNotes: ", ImGuiTreeNodeFlags_DefaultOpen)) {
				ImGui::SameLine();
				ImGui::Text(editingGroup.getGroupTypeString().c_str());

				if (editingGroup.getGroupType() == GroupType::STREAM) {
					double repeatIntervalSec = editingGroup.getStreamRepeatIntervalSec();
					ImGui::SetNextItemWidth(editorContext.itemWidth7char + editorContext.itemWidthStep);
					double step = beatmap.getsecPerBeat() / static_cast<double>(beatmap.getSubbeat());
					if (ImGui::InputDouble("Repeat Interval", &repeatIntervalSec, step, 0.0, "%.3f s")) {

						if (editor.setStreamRepeatIntervalSec(editingGroup.getGroupID(), editorContext, beatmap, beatmapStack, repeatIntervalSec)) {
							markDifficultyDirty();
						}
					}

					// stream template
					std::vector<uint64_t> tamplateGroupIDs = editingGroup.getStreamGroupNotesIDs();
					for (uint64_t groupID : tamplateGroupIDs) {
						GroupNotes& templateGroup = beatmap.getStreamGroupNotesWithGroupID(groupID);
						std::string groupIDString = std::to_string(groupID);
						ImGui::PushID(groupIDString.c_str());
						if (ImGui::TreeNodeEx(templateGroup.getGroupTypeString().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
							for (size_t i = 0; i < templateGroup.size(); i++) {
								uint64_t noteID = templateGroup.getNoteAtIndex(i).getNoteID();
								std::string noteIDString = std::to_string(noteID);
								ImGui::PushID(noteIDString.c_str()); // Push unique ID for each note
								if (EditorUtility::drawNoteProperty(noteID, editor, editorContext, beatmap, beatmapStack)) {
									markDifficultyDirty();
								}
								ImGui::PopID();
							}
							ImGui::TreePop();
						}
						ImGui::PopID();
					}
				}

				for (size_t i = 0; i < editingGroup.size(); i++) {
					uint64_t noteID = editingGroup.getNoteAtIndex(i).getNoteID();
					std::string noteIDString = std::to_string(noteID);
					ImGui::PushID(noteIDString.c_str()); // Push unique ID for each note
					if (EditorUtility::drawNoteProperty(noteID, editor, editorContext, beatmap, beatmapStack)) {
						markDifficultyDirty();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		ImGui::Separator();

		//property of Selected notes
		if (editorContext.selectedNotes.size() > 0) {
				if (ImGui::TreeNodeEx("Selected Notes: ", ImGuiTreeNodeFlags_DefaultOpen)) {

					for (size_t i = 0; i < editorContext.selectedNotes.size(); i++) {
						uint64_t noteID = editorContext.selectedNotes[i];
						std::string noteIDString = std::to_string(noteID);
						ImGui::PushID(noteIDString.c_str()); // Push unique ID for each note
						if (EditorUtility::drawNoteProperty(noteID, editor, editorContext, beatmap, beatmapStack)) {
							markDifficultyDirty();
						}
						ImGui::PopID();
					}

				ImGui::TreePop();
			}
		}

		ImGui::Separator();

		ImGui::Text("3D Viewer");

		if (ImGui::Button("Camera Reset")) {
			editorContext.yaw = editorContext.DEFAULT_CAMERA_YAW;
			editorContext.pitch = editorContext.DEFAULT_CAMERA_PITCH;
			editorContext.cameraPos = editorContext.cameraPosInitial;
			editorContext.cameraUp = editorContext.cameraUpInitial;
			//calc of view/projection is in 3d viewer
		}
		ImGui::SameLine(); ImGui::Text("FOV:"); ImGui::SameLine();  ImGui::SetNextItemWidth(editorContext.itemWidth7char);		//FOV
		ImGui::InputFloat("##FOV", &editorContext.FOV3d);
		/*
		ImGui::Separator();
		ImGui::Text("2D Viewer");
		*/

		ImGui::End();
	}
	// ==========================



	// ======== Perspective Viewer ========

	ImGui::SetNextWindowSizeConstraints(
		ImVec2(50, 50),   // min size
		ImVec2(FLT_MAX, FLT_MAX) // max size
	);

	ImGui::Begin("3D Viewer", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);

	if (!ImGui::IsWindowCollapsed())
	{
		const bool viewerWindowHovered = ImGui::IsWindowHovered(
			ImGuiHoveredFlags_RootAndChildWindows |
			ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
		);
		editorContext.isHovered3d = viewerWindowHovered;

		ImVec2 viewerSize = ImGui::GetContentRegionAvail();
		editorContext.psFBOWidth = static_cast<int>(viewerSize.x);
		editorContext.psFBOHeight = static_cast<int>(viewerSize.y);
		//resize fbo if window size changed
		if (editorContext.psFBOWidth != editorContext.prevPsFBOWidth || editorContext.psFBOHeight != editorContext.prevPsFBOHeight) {
			perspectiveRenderer.initFBO(editorContext);
			editorContext.prevPsFBOWidth = editorContext.psFBOWidth;
			editorContext.prevPsFBOHeight = editorContext.psFBOHeight;
		}

		//check hovered content region
		editorContext.isHoveredContent3d = ImGui::IsMouseHoveringRect(
			ImGui::GetWindowContentRegionMin() + ImGui::GetWindowPos(),
			ImGui::GetWindowContentRegionMax() + ImGui::GetWindowPos()
		);
		canEditInViewer3d = editorContext.isHoveredContent3d && viewerWindowHovered;

		bool isRotating = inputState.isActionDown(Action::CameraMove) && canEditInViewer3d;

		//update content origin
		editorContext.contentOrigin3d = ImGui::GetCursorScreenPos(); // start point of drawing (top-left point of window)

		// Camera control
		// Update yaw and pitch based on mouse movement
		if (isRotating) {
			editorContext.yaw += static_cast<float>(inputState.getMouseDelta().x) * editorContext.cameraSensitivity;
			editorContext.pitch -= static_cast<float>(inputState.getMouseDelta().y) * editorContext.cameraSensitivity;
			while (editorContext.yaw > 360.0f) editorContext.yaw -= 360.0f;
			while (editorContext.yaw < 0.0f)   editorContext.yaw += 360.0f;
			// Limit pitch to avoid flipping
			editorContext.pitch = glm::clamp(editorContext.pitch, -89.0f, 89.0f);
		}

		glm::vec3 front;
		front.x = cos(glm::radians(editorContext.yaw)) * cos(glm::radians(editorContext.pitch));
		front.y = sin(glm::radians(editorContext.pitch));
		front.z = sin(glm::radians(editorContext.yaw)) * cos(glm::radians(editorContext.pitch));
		front = glm::normalize(front);
		editorContext.cameraFront = front;						// Look at direction
		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
		glm::vec3 up = glm::normalize(glm::cross(right, front));

		//update camera position with keyboard input
		if (isRotating) {
			float v = editorContext.cameraMoveSpeed * editorContext.deltaTime;
			if (inputState.isActionDown(Action::CameraForward)) editorContext.cameraPos += front * v;
			if (inputState.isActionDown(Action::CameraBackward)) editorContext.cameraPos -= front * v;
			if (inputState.isActionDown(Action::CameraLeft)) editorContext.cameraPos -= right * v;
			if (inputState.isActionDown(Action::CameraRight)) editorContext.cameraPos += right * v;
			if (inputState.isActionDown(Action::CameraDown)) editorContext.cameraPos -= up * v;
			if (inputState.isActionDown(Action::CameraUp)) editorContext.cameraPos += up * v;
		}
		editorContext.cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);		// Direction of "up"

		// View matrix
		editorContext.updateViewPerspective();

		// Projection matrix
		editorContext.updateProjectionPerspective();

		// disable drag on image (put invisible button on image)
		ImGui::InvisibleButton("BlockDrag", ImVec2(static_cast<float>(editorContext.psFBOWidth), static_cast<float>(editorContext.psFBOHeight)));

		ImGui::SetCursorScreenPos(editorContext.contentOrigin3d);		//reset cursor pos to top-left of content area

		ImGui::Image(
			(void*)(intptr_t)perspectiveRenderer.renderFrame(editorContext),
			ImVec2(static_cast<float>(editorContext.psFBOWidth), static_cast<float>(editorContext.psFBOHeight)),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);

		// Draw rectangle for rect selection 
		if (editorContext.duringRectSelect3d) {
			ImVec2 p0 = editorContext.rectSelectStartPos;
			ImVec2 p1 = editorContext.rectSelectEndPos;
			ImVec2 rectMin(
				p0.x < p1.x ? p0.x : p1.x,
				p0.y < p1.y ? p0.y : p1.y
			);
			ImVec2 rectMax(
				p0.x > p1.x ? p0.x : p1.x,
				p0.y > p1.y ? p0.y : p1.y
			);
			ImVec2 clipMin = editorContext.contentOrigin3d;
			ImVec2 clipMax = clipMin + ImVec2(static_cast<float>(editorContext.psFBOWidth), static_cast<float>(editorContext.psFBOHeight));

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->PushClipRect(clipMin, clipMax, true);
			dl->AddRectFilled(rectMin, rectMax, IM_COL32(120, 220, 255, 48));
			dl->AddRect(rectMin, rectMax, IM_COL32(120, 220, 255, 180), 0.0f, 0, 1.5f);
			dl->PopClipRect();
		}


	}
	else {
		editorContext.isHovered3d = false;
		editorContext.isHoveredContent3d = false;
		canEditInViewer3d = false;
	}

	ImGui::End();

	// ===================

	// ==== 2d Viewer ====

	ImGui::SetNextWindowSizeConstraints(
		ImVec2(50, 50),   // min size
		ImVec2(FLT_MAX, FLT_MAX) // max size
	);

	ImGui::Begin("2D Viewer", nullptr, ImGuiWindowFlags_NoBringToFrontOnFocus);

	if (!ImGui::IsWindowCollapsed())
	{
		const bool viewerWindowHovered = ImGui::IsWindowHovered(
			ImGuiHoveredFlags_RootAndChildWindows |
			ImGuiHoveredFlags_AllowWhenBlockedByActiveItem
		);
		editorContext.isHovered2d = viewerWindowHovered;

		ImVec2 viewerSize = ImGui::GetContentRegionAvail();
		editorContext.orFBOWidth = static_cast<int>(viewerSize.x);
		editorContext.orFBOHeight = static_cast<int>(viewerSize.y);
		//resize fbo if window size changed
		if (editorContext.orFBOWidth != editorContext.prevOrFBOWidth || editorContext.orFBOHeight != editorContext.prevOrFBOHeight) {
			orthoRenderer.initFBO(editorContext);
			editorContext.prevOrFBOWidth = editorContext.orFBOWidth;
			editorContext.prevOrFBOHeight = editorContext.orFBOHeight;
		}

		//check hovering content
		editorContext.isHoveredContent2d = ImGui::IsMouseHoveringRect(
			ImGui::GetWindowContentRegionMin() + ImGui::GetWindowPos(),
			ImGui::GetWindowContentRegionMax() + ImGui::GetWindowPos()
		);
		canEditInViewer2d = editorContext.isHoveredContent2d && viewerWindowHovered;

		//update content origin
		editorContext.contentOrigin2d = ImGui::GetCursorScreenPos(); // start point of drawing (top-left point of window)


		// disable drag on image (put invisible button on image)
		ImGui::InvisibleButton("BlockDrag", ImVec2(static_cast<float>(editorContext.orFBOWidth), static_cast<float>(editorContext.orFBOHeight)));

		ImGui::SetCursorScreenPos(editorContext.contentOrigin2d);		//reset cursor pos to top-left of content area

		ImGui::Image(
			(void*)(intptr_t)orthoRenderer.renderFrame(editorContext),
			ImVec2(static_cast<float>(editorContext.orFBOWidth), static_cast<float>(editorContext.orFBOHeight)),
			ImVec2(0, 1),
			ImVec2(1, 0)
		);

		// Draw rectangle for rect selection
		if (editorContext.duringRectSelect2d) {
			ImVec2 p0 = editorContext.rectSelectStartPos;
			ImVec2 p1 = editorContext.rectSelectEndPos;
			ImVec2 rectMin(
				p0.x < p1.x ? p0.x : p1.x,
				p0.y < p1.y ? p0.y : p1.y
			);
			ImVec2 rectMax(
				p0.x > p1.x ? p0.x : p1.x,
				p0.y > p1.y ? p0.y : p1.y
			);
			ImVec2 clipMin = editorContext.contentOrigin2d;
			ImVec2 clipMax = clipMin + ImVec2(static_cast<float>(editorContext.orFBOWidth), static_cast<float>(editorContext.orFBOHeight));

			ImDrawList* dl = ImGui::GetWindowDrawList();
			dl->PushClipRect(clipMin, clipMax, true);
			dl->AddRectFilled(rectMin, rectMax, IM_COL32(120, 220, 255, 48));
			dl->AddRect(rectMin, rectMax, IM_COL32(120, 220, 255, 180), 0.0f, 0, 1.5f);
			dl->PopClipRect();
		}
	}
	else {
		editorContext.isHovered2d = false;
		editorContext.isHoveredContent2d = false;
		canEditInViewer2d = false;
	}

	ImGui::End();
	// ==========================
	
	// Editor tick (build frame data, edit in viewer)
	if (editor.tick(editorContext, beatmap, beatmapStack, inputState, canAcceptViewerEditInput())) {
		markDifficultyDirty();
		refreshBpmConsistencyMessages();
	}

	return result;
}

void DifficultyEditorScreen::drawCloseConfirmationModal(ScreenResult& result) {
	if (closeConfirmationRequested) {
		ImGui::OpenPopup("Unsaved Difficulty Changes Before Exit");
		closeConfirmationRequested = false;
	}

	if (ImGui::BeginPopupModal("Unsaved Difficulty Changes Before Exit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("This difficulty has unsaved changes.");
		ImGui::Separator();

		if (ImGui::Button("Save and Exit", ImVec2(140, 0))) {
			if (saveCurrentDifficulty()) {
				result.hasApplicationCloseRequest = true;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Exit Without Saving", ImVec2(160, 0))) {
			result.hasApplicationCloseRequest = true;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		if (!saveStatusMessage.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", saveStatusMessage.c_str());
		}

		ImGui::EndPopup();
	}
}

