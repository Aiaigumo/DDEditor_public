#include "BeatmapInfoScreen.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cmath>
#include <string>
#include <vector>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "NativeFileDialog.h"

namespace {
bool nearlyEqualBpm(float a, float b) {
	return std::fabs(a - b) <= 0.001f;
}

const char* projectBpmConsistencyToText(BeatmapInfoScreen::ProjectBpmConsistency consistency) {
	switch (consistency) {
	case BeatmapInfoScreen::ProjectBpmConsistency::Consistent:
		return "Consistent";
	case BeatmapInfoScreen::ProjectBpmConsistency::Mismatch:
		return "Mismatch";
	case BeatmapInfoScreen::ProjectBpmConsistency::Unknown:
	default:
		return "Unknown";
	}
}

const char* difficultyBpmConsistencyToText(BeatmapInfoScreen::DifficultyBpmConsistency consistency) {
	switch (consistency) {
	case BeatmapInfoScreen::DifficultyBpmConsistency::Consistent:
		return "Consistent";
	case BeatmapInfoScreen::DifficultyBpmConsistency::Mismatch:
		return "Mismatch";
	case BeatmapInfoScreen::DifficultyBpmConsistency::LoadFailed:
		return "Load Failed";
	case BeatmapInfoScreen::DifficultyBpmConsistency::Missing:
	default:
		return "Missing";
	}
}

const char* bpmExportModeToText(BpmExportMode mode) {
	switch (mode) {
	case BpmExportMode::Multiple:
		return "Multiple";
	case BpmExportMode::Single:
	default:
		return "Single";
	}
}

std::string toLowerAscii(std::string value) {
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

std::vector<NativeFileDialog::FileTypeFilter> buildAudioFileFilters(const EditorContext& editorContext) {
	std::string pattern;
	for (const std::string& extension : editorContext.audioExtensions) {
		if (!pattern.empty()) {
			pattern += ";";
		}
		pattern += "*" + extension;
	}

	return {
		{ "Audio Files (" + pattern + ")", pattern },
		{ "All Files (*.*)", "*.*" }
	};
}

bool tryMakeRelativeToWorkingFolder(
	const std::filesystem::path& workingFolder,
	const std::filesystem::path& filePath,
	std::filesystem::path& relativePath
) {
	namespace fs = std::filesystem;

	std::error_code fsError;
	const fs::path canonicalWorkingFolder = fs::weakly_canonical(workingFolder, fsError);
	if (fsError) {
		return false;
	}

	fsError.clear();
	const fs::path canonicalFilePath = fs::weakly_canonical(filePath, fsError);
	if (fsError) {
		return false;
	}

	fsError.clear();
	relativePath = fs::relative(canonicalFilePath, canonicalWorkingFolder, fsError);
	if (fsError || relativePath.empty() || relativePath.is_absolute()) {
		return false;
	}

	for (const fs::path& part : relativePath) {
		if (part == "..") {
			return false;
		}
	}

	return true;
}
}

BeatmapInfoScreen::BeatmapInfoScreen(
	EditorContext& editorContext,
	Beatmap& beatmap,
	BeatmapInfo& bmInfo,
	BeatmapStack& beatmapStack,
	Editor& editor,
	void* nativeWindowHandle
) :
	editorContext(editorContext),
	beatmap(beatmap),
	bmInfo(bmInfo),
	beatmapStack(beatmapStack),
	editor(editor),
	nativeWindowHandle(nativeWindowHandle) {
}

ScreenResult BeatmapInfoScreen::runFrame() {
	ScreenResult result;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("Beatmap Info", nullptr, windowFlags);

	drawFolderControls();

	if (!statusMessage.empty()) {
		ImGui::TextWrapped("%s", statusMessage.c_str());
	}

	ImGui::Separator();

	if (!hasWorkingFolder) {
		ImGui::Text("Select a beatmap folder to edit.");
		drawCloseConfirmationModal(result);
		ImGui::End();
		return result;
	}

	if (missingInfoJson) {
		ImGui::TextWrapped("info.json was not found. Empty BeatmapInfo is being used.");
	}

	drawBeatmapInfo();
	drawBpmConsistency();
	drawDifficultyList(result);
	drawCloseConfirmationModal(result);

	ImGui::End();

	return result;
}

bool BeatmapInfoScreen::hasUnsavedChanges() const {
	return dirty;
}

void BeatmapInfoScreen::requestCloseConfirmation() {
	closeConfirmationRequested = true;
}

void BeatmapInfoScreen::onEnterScreen() {
	if (!hasWorkingFolder) {
		return;
	}

	updateBpmConsistency();
}

void BeatmapInfoScreen::onExitScreen() {
}

void BeatmapInfoScreen::openWorkingFolder() {
	namespace fs = std::filesystem;

	if (editorContext.beatmapFolderPathBuffer.empty()) {
		hasWorkingFolder = false;
		statusMessage = "Beatmap folder path is empty.";
		return;
	}

	const fs::path folderPath = editorContext.beatmapFolderPathBuffer;
	if (!fs::exists(folderPath)) {
		hasWorkingFolder = false;
		statusMessage = "Beatmap folder does not exist.";
		return;
	}
	if (!fs::is_directory(folderPath)) {
		hasWorkingFolder = false;
		statusMessage = "Beatmap folder path is not a directory.";
		return;
	}

	bmInfo.clear();
	editorContext.hasLoadedBeatmapFolder = true;
	hasWorkingFolder = true;

	const fs::path infoPath = folderPath / editorContext.infoPath;
	if (fs::exists(infoPath)) {
		if (bmInfo.loadFromFile(infoPath.string())) {
			missingInfoJson = false;
			statusMessage = "Loaded info.json.";
		}
		else {
			missingInfoJson = true;
			statusMessage = "Failed to load info.json. Empty BeatmapInfo is being used.";
		}
	}
	else {
		missingInfoJson = true;
		statusMessage = "info.json was not found. Empty BeatmapInfo is being used.";
	}

	if (!bmInfo.getSongPath().empty()) {
		editorContext.songPathBuffer = (folderPath / bmInfo.getSongPath()).string();
	}
	else {
		editorContext.songPathBuffer.clear();
	}

	loadEditStateFromBeatmapInfo();
	updateBpmConsistency();
	dirty = false;
}

void BeatmapInfoScreen::drawFolderControls() {
	ImGui::Text("Working Folder");
	ImGui::InputText("##WorkingFolder", &editorContext.beatmapFolderPathBuffer);
	ImGui::SameLine();
	if (ImGui::Button("Browse...")) {
		const NativeFileDialog::FolderDialogResult folderResult =
			NativeFileDialog::selectFolder(
				nativeWindowHandle,
				"Select Beatmap Folder",
				editorContext.beatmapFolderPathBuffer
			);
		if (folderResult.selected) {
			editorContext.beatmapFolderPathBuffer = folderResult.path;
			openWorkingFolder();
		}
		else if (!folderResult.errorMessage.empty()) {
			statusMessage = folderResult.errorMessage;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Open")) {
		openWorkingFolder();
	}
}

void BeatmapInfoScreen::drawBeatmapInfo() {
	ImGui::Text("BeatmapInfo");

	if (dirty) {
		ImGui::SameLine();
		ImGui::TextUnformatted("(Unsaved)");
	}

	if (ImGui::InputText("Song Name", &editState.songName)) {
		dirty = true;
	}
	if (ImGui::InputText("Artist Name", &editState.artistName)) {
		dirty = true;
	}
	if (ImGui::InputText("Mapper Name", &editState.mapperName)) {
		dirty = true;
	}
	drawSongPathControls();

	const char* bpmModeItems[] = { "Single", "Multiple" };
	int bpmModeIndex = editState.projectBpmMode == ProjectBpmMode::Multiple ? 1 : 0;
	if (ImGui::Combo("BPM Mode", &bpmModeIndex, bpmModeItems, IM_ARRAYSIZE(bpmModeItems))) {
		editState.projectBpmMode = bpmModeIndex == 1 ? ProjectBpmMode::Multiple : ProjectBpmMode::Single;
		if (editState.projectBpmMode == ProjectBpmMode::Multiple) {
			editState.bpm = Beatmap::getDefaultExportBPM();
		}
		else {
			float firstDifficultyBpm = 0.0f;
			if (tryGetFirstLoadedDifficultyFirstBpm(firstDifficultyBpm)) {
				editState.bpm = firstDifficultyBpm;
			}
		}
		dirty = true;
	}
	if (ImGui::InputFloat("BPM", &editState.bpm, 0.0f, 0.0f, "%.2f")) {
		if (editState.bpm < 0.0f) {
			editState.bpm = 0.0f;
		}
		dirty = true;
	}

	ImGui::BeginDisabled(true);
	ImGui::InputText("Song Length", &editState.songLength, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputFloat("Song Preview Time", &editState.songPreviewSeconds);
	ImGui::Separator();
	ImGui::InputText("Video URL", &editState.videoUrl);
	ImGui::InputFloat("Youtube Offset", &editState.youtubeOffset);
	ImGui::Checkbox("Is Mirror", &editState.isMirror);
	ImGui::Separator();
	ImGui::InputText("Editor Version", &editState.editorVersion, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputInt("Beatmap ID", &editState.beatmapID, 0, 0, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputInt("OST ID", &editState.ostID, 0, 0, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("OST Name", &editState.ostName, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputText("Create Time", &editState.createTicksStr, ImGuiInputTextFlags_ReadOnly);
	ImGui::InputScalar("Create Ticks", ImGuiDataType_U64, &editState.createTicks, nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly);
	ImGui::EndDisabled();

	if (ImGui::Button("Save Info")) {
		saveInfo();
	}
}

void BeatmapInfoScreen::drawSongPathControls() {
	if (ImGui::InputText("Song Path", &editState.songPath)) {
		dirty = true;
		if (!editState.songPath.empty() && hasWorkingFolder) {
			editorContext.songPathBuffer =
				(std::filesystem::path(editorContext.beatmapFolderPathBuffer) / editState.songPath).string();
		}
		else {
			editorContext.songPathBuffer.clear();
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!hasWorkingFolder);
	if (ImGui::Button("Browse...##SongPath")) {
		browseSongFileInWorkingFolder();
	}
	ImGui::SameLine();
	if (ImGui::Button("Import...##SongPath")) {
		importSongFileToWorkingFolder();
	}
	ImGui::EndDisabled();

	if (!editState.songPath.empty()) {
		const std::filesystem::path fullSongPath =
			std::filesystem::path(editorContext.beatmapFolderPathBuffer) / editState.songPath;
		ImGui::TextWrapped(
			"%s: %s",
			std::filesystem::exists(fullSongPath) ? "Song file found" : "Song file missing",
			fullSongPath.string().c_str()
		);
	}
	else {
		ImGui::TextWrapped("Song path is empty.");
	}
}

void BeatmapInfoScreen::browseSongFileInWorkingFolder() {
	const std::filesystem::path workingFolder = editorContext.beatmapFolderPathBuffer;
	const NativeFileDialog::FileDialogResult fileResult =
		NativeFileDialog::selectFile(
			nativeWindowHandle,
			"Select Song File in Working Folder",
			workingFolder.string(),
			buildAudioFileFilters(editorContext)
		);

	if (!fileResult.selected) {
		if (!fileResult.errorMessage.empty()) {
			statusMessage = fileResult.errorMessage;
		}
		return;
	}

	if (!trySetSongPathFromWorkingFolderFile(fileResult.path)) {
		return;
	}

	statusMessage = "Selected song file: " + editState.songPath;
}

void BeatmapInfoScreen::importSongFileToWorkingFolder() {
	namespace fs = std::filesystem;

	const fs::path workingFolder = editorContext.beatmapFolderPathBuffer;
	const NativeFileDialog::FileDialogResult fileResult =
		NativeFileDialog::selectFile(
			nativeWindowHandle,
			"Import Song File",
			workingFolder.string(),
			buildAudioFileFilters(editorContext)
		);

	if (!fileResult.selected) {
		if (!fileResult.errorMessage.empty()) {
			statusMessage = fileResult.errorMessage;
		}
		return;
	}

	const fs::path sourcePath = fileResult.path;
	if (!isSupportedAudioFile(sourcePath)) {
		statusMessage = "Selected file is not a supported audio file.";
		return;
	}

	std::error_code fsError;
	if (!fs::is_regular_file(sourcePath, fsError)) {
		statusMessage = "Selected song path is not a file.";
		return;
	}

	const fs::path destinationPath = workingFolder / sourcePath.filename();
	fsError.clear();
	if (fs::exists(destinationPath, fsError)) {
		statusMessage = "Song file already exists in the working folder: " + destinationPath.filename().string();
		return;
	}
	if (fsError) {
		statusMessage = "Failed to check destination song file: " + destinationPath.string();
		return;
	}

	fsError.clear();
	fs::copy_file(sourcePath, destinationPath, fs::copy_options::none, fsError);
	if (fsError) {
		statusMessage = "Failed to import song file: " + fsError.message();
		return;
	}

	if (!trySetSongPathFromWorkingFolderFile(destinationPath)) {
		return;
	}

	statusMessage = "Imported song file: " + editState.songPath;
}

bool BeatmapInfoScreen::trySetSongPathFromWorkingFolderFile(const std::filesystem::path& songFilePath) {
	namespace fs = std::filesystem;

	if (!hasWorkingFolder || editorContext.beatmapFolderPathBuffer.empty()) {
		statusMessage = "Select a beatmap folder before selecting a song file.";
		return false;
	}

	std::error_code fsError;
	if (!fs::is_regular_file(songFilePath, fsError)) {
		statusMessage = "Selected song path is not a file.";
		return false;
	}

	if (!isSupportedAudioFile(songFilePath)) {
		statusMessage = "Selected file is not a supported audio file.";
		return false;
	}

	fs::path relativeSongPath;
	if (!tryMakeRelativeToWorkingFolder(editorContext.beatmapFolderPathBuffer, songFilePath, relativeSongPath)) {
		statusMessage = "Selected song file must be inside the working folder. Use Import... to copy an external file.";
		return false;
	}

	editState.songPath = relativeSongPath.generic_string();
	editorContext.songPathBuffer =
		(fs::path(editorContext.beatmapFolderPathBuffer) / relativeSongPath).string();
	dirty = true;
	return true;
}

bool BeatmapInfoScreen::isSupportedAudioFile(const std::filesystem::path& songFilePath) const {
	const std::string extension = toLowerAscii(songFilePath.extension().string());
	for (const std::string& supportedExtension : editorContext.audioExtensions) {
		if (extension == toLowerAscii(supportedExtension)) {
			return true;
		}
	}

	return false;
}

void BeatmapInfoScreen::drawBpmConsistency() {
	ImGui::Separator();
	ImGui::Text("BPM Consistency: %s", projectBpmConsistencyToText(projectBpmConsistency));
	ImGui::Text("Project BPM: %.2f", bmInfo.getBPM());

	if (projectBpmConsistency == ProjectBpmConsistency::Unknown) {
		ImGui::Text("No registered difficulty has been checked.");
		return;
	}

	if (ImGui::BeginTable("BpmConsistencyTable", 10, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
		ImGui::TableSetupColumn("Difficulty");
		ImGui::TableSetupColumn("State");
		ImGui::TableSetupColumn("Mode");
		ImGui::TableSetupColumn("Timings");
		ImGui::TableSetupColumn("First BPM");
		ImGui::TableSetupColumn("Export BPM");
		ImGui::TableSetupColumn("Message");
		ImGui::TableSetupColumn("Re-export Mode");
		ImGui::TableSetupColumn("Re-export BPM");
		ImGui::TableSetupColumn("Action");
		ImGui::TableHeadersRow();

		for (const DifficultyBpmStatus& status : difficultyBpmStatuses) {
			if (!status.exists && status.consistency == DifficultyBpmConsistency::Missing) {
				continue;
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%s", editorContext.beatmapDiff.at(status.diffIndex).c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::Text("%s", difficultyBpmConsistencyToText(status.consistency));
			ImGui::TableSetColumnIndex(2);
			ImGui::Text("%s", bpmExportModeToText(status.exportMode));
			const bool isModeConsistent =
				(bmInfo.getProjectBpmMode() == ProjectBpmMode::Single && status.exportMode == BpmExportMode::Single) ||
				(bmInfo.getProjectBpmMode() == ProjectBpmMode::Multiple && status.exportMode == BpmExportMode::Multiple);
			if (!isModeConsistent) {
				ImGui::TextWrapped("Mode mismatch with project BPM mode.");
			}
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%zu", status.timingCount);
			ImGui::TableSetColumnIndex(4);
			ImGui::Text("%.2f", status.firstTimingBpm);
			ImGui::TableSetColumnIndex(5);
			ImGui::Text("%.2f", status.rawExportBpm);
			ImGui::TableSetColumnIndex(6);
			ImGui::TextWrapped("%s", status.message.c_str());
			ImGui::TableSetColumnIndex(7);
			if (status.loaded) {
				ImGui::PushID(status.diffIndex);
				DifficultyBpmEditState& bpmEditState = difficultyBpmEditStates.at(status.diffIndex);
				const char* modeItems[] = { "Single", "Multiple" };
				int modeIndex = bpmEditState.exportMode == BpmExportMode::Multiple ? 1 : 0;
				if (ImGui::Combo("##ReExportMode", &modeIndex, modeItems, IM_ARRAYSIZE(modeItems))) {
					bpmEditState.exportMode = modeIndex == 1 ? BpmExportMode::Multiple : BpmExportMode::Single;
					if (bpmEditState.exportMode == BpmExportMode::Multiple) {
						bpmEditState.exportBpm = Beatmap::getDefaultExportBPM();
					}
					else {
						bpmEditState.exportBpm = status.firstTimingBpm;
					}
				}
				ImGui::PopID();
			}
			ImGui::TableSetColumnIndex(8);
			if (status.loaded) {
				DifficultyBpmEditState& bpmEditState = difficultyBpmEditStates.at(status.diffIndex);
				const bool isSingleMode = bpmEditState.exportMode == BpmExportMode::Single;
				ImGui::BeginDisabled(isSingleMode);
				ImGui::PushID(status.diffIndex);
				ImGui::SetNextItemWidth(90.0f);
				if (ImGui::InputFloat("##ReExportBPM", &bpmEditState.exportBpm, 0.0f, 0.0f, "%.2f")) {
					if (bpmEditState.exportBpm < 0.0f) {
						bpmEditState.exportBpm = 0.0f;
					}
				}
				ImGui::PopID();
				ImGui::EndDisabled();
			}
			ImGui::TableSetColumnIndex(9);
			if (status.loaded) {
				const DifficultyBpmEditState& bpmEditState = difficultyBpmEditStates.at(status.diffIndex);
				const bool canReExport =
					(bpmEditState.exportMode == BpmExportMode::Single && status.timingCount == 1) ||
					(bpmEditState.exportMode == BpmExportMode::Multiple && bpmEditState.exportBpm > 0.0f);
				ImGui::BeginDisabled(!canReExport);
				ImGui::PushID(status.diffIndex);
				if (ImGui::Button("Re-export")) {
					reExportDifficultyWithBpmSettings(status.diffIndex);
				}
				ImGui::PopID();
				ImGui::EndDisabled();
			}
		}

		ImGui::EndTable();
	}
}

void BeatmapInfoScreen::reExportDifficultyWithBpmSettings(int diffIndex) {
	if (diffIndex < 0 || diffIndex >= BeatmapInfo::LEVEL_NUM || !bmInfo.getExistDiff(diffIndex)) {
		statusMessage = "Invalid difficulty selected for BPM re-export.";
		return;
	}

	const std::filesystem::path diffPath =
		std::filesystem::path(editorContext.beatmapFolderPathBuffer) / bmInfo.getDiffPath(diffIndex);

	Beatmap targetBeatmap;
	if (!targetBeatmap.loadFromFile(diffPath.string())) {
		statusMessage = "Failed to load difficulty file for BPM re-export.";
		updateBpmConsistency();
		return;
	}

	const DifficultyBpmEditState& bpmEditState = difficultyBpmEditStates.at(diffIndex);
	if (bpmEditState.exportMode == BpmExportMode::Single) {
		if (targetBeatmap.getTimingListSize() != 1) {
			statusMessage = "Single re-export requires exactly one timing.";
			updateBpmConsistency();
			return;
		}
		targetBeatmap.setBpmExportMode(BpmExportMode::Single);
	}
	else {
		if (bpmEditState.exportBpm <= 0.0f) {
			statusMessage = "Multiple re-export requires a positive Export BPM.";
			updateBpmConsistency();
			return;
		}
		targetBeatmap.setBpmExportMode(BpmExportMode::Multiple);
		targetBeatmap.setRawExportBPM(bpmEditState.exportBpm);
	}

	if (!targetBeatmap.exportJson(diffPath.string(), true)) {
		statusMessage = "Failed to re-export difficulty.";
		updateBpmConsistency();
		return;
	}

	statusMessage = "Re-exported difficulty.";
	updateBpmConsistency();
}

void BeatmapInfoScreen::drawDifficultyList(ScreenResult& result) {
	ImGui::Separator();
	ImGui::Text("Difficulties");

	bool hasAnyDifficulty = false;
	bool shouldOpenUnsavedModal = false;
	bool shouldOpenDeleteModal = false;
	for (int i = 0; i < BeatmapInfo::LEVEL_NUM; i++) {
		ImGui::PushID(i);
		ImGui::Text("%s", editorContext.beatmapDiff.at(i).c_str());
		ImGui::SameLine();
		if (bmInfo.getExistDiff(i)) {
			hasAnyDifficulty = true;
			ImGui::Text("%s", bmInfo.getDiffPath(i).c_str());
			ImGui::SameLine();
			if (ImGui::Button("Edit")) {
				if (dirty) {
					pendingEditDiffIndex = i;
					shouldOpenUnsavedModal = true;
				}
				else {
					openDifficultyForEdit(i, result);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete")) {
				pendingDeleteDiffIndex = i;
				shouldOpenDeleteModal = true;
			}
		}
		else {
			const std::string defaultDiffPath =
				editorContext.beatmapDiff.at(i) + editorContext.jsonExtension;
			ImGui::Text("%s", defaultDiffPath.c_str());
			ImGui::SameLine();
			if (ImGui::Button("Create")) {
				createDifficulty(i);
			}
		}
		ImGui::PopID();
	}
	if (shouldOpenUnsavedModal) {
		ImGui::OpenPopup("Unsaved BeatmapInfo Changes");
	}
	if (shouldOpenDeleteModal) {
		ImGui::OpenPopup("Delete Difficulty");
	}

	if (ImGui::BeginPopupModal("Unsaved BeatmapInfo Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("BeatmapInfo has unsaved changes.");
		ImGui::Separator();

		if (ImGui::Button("Save", ImVec2(120, 0))) {
			if (saveInfo() && openDifficultyForEdit(pendingEditDiffIndex, result)) {
				pendingEditDiffIndex = -1;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Discard", ImVec2(120, 0))) {
			loadEditStateFromBeatmapInfo();
			dirty = false;
			if (openDifficultyForEdit(pendingEditDiffIndex, result)) {
				pendingEditDiffIndex = -1;
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			pendingEditDiffIndex = -1;
			ImGui::CloseCurrentPopup();
		}

		if (!statusMessage.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", statusMessage.c_str());
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Delete Difficulty", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Delete this difficulty file?");
		if (pendingDeleteDiffIndex >= 0 && pendingDeleteDiffIndex < BeatmapInfo::LEVEL_NUM) {
			const std::filesystem::path diffPath =
				std::filesystem::path(editorContext.beatmapFolderPathBuffer) / bmInfo.getDiffPath(pendingDeleteDiffIndex);
			ImGui::TextWrapped("%s", diffPath.string().c_str());
		}
		ImGui::Separator();

		if (ImGui::Button("Delete", ImVec2(120, 0))) {
			deleteDifficulty(pendingDeleteDiffIndex);
			pendingDeleteDiffIndex = -1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			pendingDeleteDiffIndex = -1;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (!hasAnyDifficulty) {
		ImGui::Text("No difficulty is registered in BeatmapInfo.");
	}
}

void BeatmapInfoScreen::drawCloseConfirmationModal(ScreenResult& result) {
	if (closeConfirmationRequested) {
		ImGui::OpenPopup("Unsaved BeatmapInfo Changes Before Exit");
		closeConfirmationRequested = false;
	}

	if (ImGui::BeginPopupModal("Unsaved BeatmapInfo Changes Before Exit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("BeatmapInfo has unsaved changes.");
		ImGui::Separator();

		if (ImGui::Button("Save and Exit", ImVec2(140, 0))) {
			if (saveInfo()) {
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

		if (!statusMessage.empty()) {
			ImGui::Separator();
			ImGui::TextWrapped("%s", statusMessage.c_str());
		}

		ImGui::EndPopup();
	}
}

void BeatmapInfoScreen::loadEditStateFromBeatmapInfo() {
	editState.songName = bmInfo.getSongName();
	editState.artistName = bmInfo.getSongAuthorName();
	editState.mapperName = bmInfo.getMapperName();
	editState.songPath = bmInfo.getSongPath();
	editState.songLength = bmInfo.getSongLength();
	editState.songPreviewSeconds = bmInfo.getSongPreviewTime();
	editState.bpm = bmInfo.getBPM();
	editState.projectBpmMode = bmInfo.getProjectBpmMode();
	editState.videoUrl = bmInfo.getVideoUrl();
	editState.youtubeOffset = bmInfo.getYoutubeOffset();
	editState.isMirror = bmInfo.getIsMirror();
	editState.editorVersion = bmInfo.getEditorVersion();
	editState.beatmapID = bmInfo.getBeatmapID();
	editState.ostID = bmInfo.getOstID();
	editState.ostName = bmInfo.getOstName();
	editState.createTicks = bmInfo.getCreateTicks();
	editState.createTicksStr = bmInfo.getCreateTicksStr();
}

void BeatmapInfoScreen::applyEditStateToBeatmapInfo() {
	bmInfo.setSongName(editState.songName);
	bmInfo.setSongAuthorName(editState.artistName);
	bmInfo.setMapperName(editState.mapperName);
	bmInfo.setSongPath(editState.songPath);
	bmInfo.setBPM(editState.bpm);
	bmInfo.setProjectBpmMode(editState.projectBpmMode);
}

bool BeatmapInfoScreen::saveInfo() {
	if (!hasWorkingFolder) {
		statusMessage = "Select a beatmap folder before saving info.json.";
		return false;
	}

	applyEditStateToBeatmapInfo();

	const std::filesystem::path infoPath =
		std::filesystem::path(editorContext.beatmapFolderPathBuffer) / editorContext.infoPath;
	if (bmInfo.saveToFile(infoPath.string(), true)) {
		missingInfoJson = false;
		dirty = false;
		loadEditStateFromBeatmapInfo();
		if (!bmInfo.getSongPath().empty()) {
			editorContext.songPathBuffer =
				(std::filesystem::path(editorContext.beatmapFolderPathBuffer) / bmInfo.getSongPath()).string();
		}
		else {
			editorContext.songPathBuffer.clear();
		}
		updateBpmConsistency();
		statusMessage = "Saved info.json.";
		return true;
	}
	else {
		dirty = true;
		statusMessage = "Failed to save info.json.";
		return false;
	}
}

void BeatmapInfoScreen::createDifficulty(int diffIndex) {
	if (!hasWorkingFolder) {
		statusMessage = "Select a beatmap folder before creating a difficulty.";
		return;
	}
	if (diffIndex < 0 || diffIndex >= BeatmapInfo::LEVEL_NUM) {
		statusMessage = "Invalid difficulty selected.";
		return;
	}
	if (bmInfo.getExistDiff(diffIndex)) {
		statusMessage = "Difficulty already exists.";
		return;
	}

	const std::string diffPath =
		editorContext.beatmapDiff.at(diffIndex) + editorContext.jsonExtension;
	const std::filesystem::path fullDiffPath =
		std::filesystem::path(editorContext.beatmapFolderPathBuffer) / diffPath;
	if (std::filesystem::exists(fullDiffPath)) {
		statusMessage = "Difficulty file already exists: " + fullDiffPath.string();
		return;
	}

	Beatmap newBeatmap;
	newBeatmap.setBpmExportMode(
		bmInfo.getProjectBpmMode() == ProjectBpmMode::Multiple
		? BpmExportMode::Multiple
		: BpmExportMode::Single
	);

	if (!newBeatmap.exportJson(fullDiffPath.string(), false)) {
		statusMessage = "Failed to create difficulty file.";
		return;
	}

	bmInfo.setDiffPath(diffIndex, diffPath);
	dirty = true;
	updateBpmConsistency();
	statusMessage = "Created difficulty: " + diffPath;
}

void BeatmapInfoScreen::deleteDifficulty(int diffIndex) {
	if (!hasWorkingFolder) {
		statusMessage = "Select a beatmap folder before deleting a difficulty.";
		return;
	}
	if (diffIndex < 0 || diffIndex >= BeatmapInfo::LEVEL_NUM || !bmInfo.getExistDiff(diffIndex)) {
		statusMessage = "Invalid difficulty selected.";
		return;
	}

	const std::string diffPath = bmInfo.getDiffPath(diffIndex);
	const std::filesystem::path fullDiffPath =
		std::filesystem::path(editorContext.beatmapFolderPathBuffer) / diffPath;

	std::error_code fsError;
	if (std::filesystem::exists(fullDiffPath, fsError)) {
		if (!std::filesystem::remove(fullDiffPath, fsError) || fsError) {
			statusMessage = "Failed to delete difficulty file: " + fullDiffPath.string();
			return;
		}
	}
	else if (fsError) {
		statusMessage = "Failed to check difficulty file: " + fullDiffPath.string();
		return;
	}

	bmInfo.setDiffPath(diffIndex, "");
	dirty = true;
	updateBpmConsistency();
	statusMessage = "Deleted difficulty file: " + diffPath;
}

bool BeatmapInfoScreen::openDifficultyForEdit(int diffIndex, ScreenResult& result) {
	if (editor.loadDifficultyAndSong(beatmap, beatmapStack, editorContext, bmInfo, diffIndex)) {
		result.hasScreenChangeRequest = true;
		result.nextScreen = AppScreen::DifficultyEditor;
		return true;
	}

	statusMessage = "Failed to load difficulty file.";
	return false;
}

void BeatmapInfoScreen::updateBpmConsistency() {
	namespace fs = std::filesystem;

	projectBpmConsistency = ProjectBpmConsistency::Unknown;
	bool hasCheckedDifficulty = false;
	bool hasMismatch = false;

	for (int i = 0; i < BeatmapInfo::LEVEL_NUM; i++) {
		DifficultyBpmStatus status;
		status.diffIndex = i;

		if (!bmInfo.getExistDiff(i)) {
			difficultyBpmStatuses.at(i) = status;
			continue;
		}

		status.exists = true;
		hasCheckedDifficulty = true;

		const fs::path diffPath =
			fs::path(editorContext.beatmapFolderPathBuffer) / bmInfo.getDiffPath(i);
		Beatmap checkedBeatmap;
		if (!checkedBeatmap.loadFromFile(diffPath.string())) {
			status.consistency = DifficultyBpmConsistency::LoadFailed;
			status.message = "Failed to load difficulty file.";
			hasMismatch = true;
			difficultyBpmStatuses.at(i) = status;
			continue;
		}

		status.loaded = true;
		status.exportMode = checkedBeatmap.getBpmExportMode();
		status.timingCount = checkedBeatmap.getTimingListSize();
		status.firstTimingBpm = checkedBeatmap.getFirstBPM();
		status.rawExportBpm = checkedBeatmap.getRawExportBPM();
		status.effectiveExportBpm = checkedBeatmap.getExportBPM();
		difficultyBpmEditStates.at(i).exportMode = status.exportMode;
		difficultyBpmEditStates.at(i).exportBpm =
			status.exportMode == BpmExportMode::Multiple
			? Beatmap::getDefaultExportBPM()
			: status.firstTimingBpm;

		const float projectBpm = bmInfo.getBPM();
		if (bmInfo.getProjectBpmMode() == ProjectBpmMode::Single) {
			const bool isConsistent =
				status.timingCount == 1 &&
				status.exportMode == BpmExportMode::Single &&
				nearlyEqualBpm(status.firstTimingBpm, projectBpm);
			status.consistency = isConsistent ? DifficultyBpmConsistency::Consistent : DifficultyBpmConsistency::Mismatch;
			status.message = isConsistent
				? "Single mode BPM is consistent."
				: "Single mode requires one timing, Single export mode, and first timing BPM equal to project BPM.";
		}
		else {
			const bool isConsistent =
				status.exportMode == BpmExportMode::Multiple &&
				nearlyEqualBpm(status.rawExportBpm, projectBpm);
			status.consistency = isConsistent ? DifficultyBpmConsistency::Consistent : DifficultyBpmConsistency::Mismatch;
			status.message = isConsistent
				? "Export BPM is consistent."
				: "Multiple mode requires Multiple export mode and export BPM equal to project BPM.";
		}

		if (status.consistency != DifficultyBpmConsistency::Consistent) {
			hasMismatch = true;
		}
		difficultyBpmStatuses.at(i) = status;
	}

	if (!hasCheckedDifficulty) {
		projectBpmConsistency = ProjectBpmConsistency::Unknown;
	}
	else {
		projectBpmConsistency = hasMismatch ? ProjectBpmConsistency::Mismatch : ProjectBpmConsistency::Consistent;
	}
}

bool BeatmapInfoScreen::tryGetFirstLoadedDifficultyFirstBpm(float& bpm) const {
	for (const DifficultyBpmStatus& status : difficultyBpmStatuses) {
		if (!status.loaded) {
			continue;
		}

		bpm = status.firstTimingBpm;
		return true;
	}

	return false;
}
