#include "BeatmapInfo.h"
#include "json.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <charconv>

namespace {
std::string projectBpmModeToString(ProjectBpmMode mode) {
	switch (mode) {
	case ProjectBpmMode::Multiple:
		return "Multiple";
	case ProjectBpmMode::Single:
	default:
		return "Single";
	}
}

ProjectBpmMode projectBpmModeFromString(const std::string& modeString) {
	if (modeString == "Multiple") {
		return ProjectBpmMode::Multiple;
	}
	return ProjectBpmMode::Single;
}
}

BeatmapInfo::BeatmapInfo()
{
	clear();
}

void BeatmapInfo::clear() {

	//clear diff existence info
	hasDiff.fill(false);

	//clear path info
	diffPaths.fill(std::string());
	songPath.clear();
	coverPath.clear();
	soundPaths.fill(std::string());

	//clear other info
	editorVersion = DEFAULT_EDITOR_VERSION;
	beatmapID = DEFAULT_BEATMAP_ID;
	ostID = DEFAULT_OST_ID;
	ostName.clear();
	createTicks = DEFAULT_CREATE_TICKS;
	createTicksStr.clear();

	//clear song info
	songName.clear();
	songLength.clear();
	songAuthorName.clear();
	mapperName.clear();
	songPreviewTime = 0.0f;
	bpm = 0.0f;
	projectBpmMode = ProjectBpmMode::Single;

	//clear youtube link info
	videoUrl.clear();
	youtubeOffset = 0.0f;
	isMirror = false;
}

bool BeatmapInfo::loadFromFile(const std::string& path) {
	
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open beatmapInfo file: " << path << std::endl;
		return false;
	}

	// clear the Object
	clear();

	nlohmann::json j;
	file >> j;

	bool shouldReadCustomInfoFields = false;
	if (j.contains(INFO_FORMAT_VERSION_KEY_NAME) &&
		!j[INFO_FORMAT_VERSION_KEY_NAME].is_null() &&
		j[INFO_FORMAT_VERSION_KEY_NAME].is_string()) {
		const std::string infoFormatVersion =
			j.value(INFO_FORMAT_VERSION_KEY_NAME, std::string());
		shouldReadCustomInfoFields = true;
		if (infoFormatVersion != LATEST_INFO_FORMAT_VERSION_STRING) {
			std::cerr << "Unsupported DDEditor beatmap info format version: "
				<< infoFormatVersion << std::endl;
		}
	}

	//load beatmap file paths for each difficulty
	for (int i = 0; i < LEVEL_NUM; i++) {
		if (j.contains(DIFF_KEY_NAME.at(i)) && !j[DIFF_KEY_NAME.at(i)].is_null() && j[DIFF_KEY_NAME.at(i)].is_string()) {	// when null or not string, occur error
			diffPaths.at(i) = j.value(DIFF_KEY_NAME.at(i), "");
		}
		if (diffPaths.at(i) != "")
			hasDiff.at(i) = true;		//set if exist diff
	}

	//load song file path
	if (j.contains(SONG_PATH_KEY_NAME) && !j[SONG_PATH_KEY_NAME].is_null() && j[SONG_PATH_KEY_NAME].is_string()) {
		songPath = j.value(SONG_PATH_KEY_NAME, "");
	}

	//load cover image path
	if (j.contains(COVER_PATH_KEY_NAME) && !j[COVER_PATH_KEY_NAME].is_null() && j[COVER_PATH_KEY_NAME].is_string()) {
		coverPath = j.value(COVER_PATH_KEY_NAME, "");
	}

	//load sound effect paths
	for (int i = 0; i < static_cast<int>(SOUND_KEY_NAME.size()); i++) {
		if (j.contains(SOUND_KEY_NAME.at(i)) && !j[SOUND_KEY_NAME.at(i)].is_null() && j[SOUND_KEY_NAME.at(i)].is_string()) {
			soundPaths.at(i) = j.value(SOUND_KEY_NAME.at(i), "");
		}
	}

	//other info
	
	//editor version
	if (j.contains(EDITOR_VERSION_KEY_NAME) && !j[EDITOR_VERSION_KEY_NAME].is_null() && j[EDITOR_VERSION_KEY_NAME].is_string()) {
		editorVersion = j.value(EDITOR_VERSION_KEY_NAME, DEFAULT_EDITOR_VERSION);
	}

	//beatmap ID
	if (j.contains(BEATMAP_ID_KEY_NAME) && !j[BEATMAP_ID_KEY_NAME].is_null() && j[BEATMAP_ID_KEY_NAME].is_number_integer()) {
		beatmapID = j.value(BEATMAP_ID_KEY_NAME, DEFAULT_BEATMAP_ID);
	}

	//ost ID
	if (j.contains(OST_ID_KEY_NAME) && !j[OST_ID_KEY_NAME].is_null() && j[OST_ID_KEY_NAME].is_number_integer()) {
		ostID = j.value(OST_ID_KEY_NAME, DEFAULT_OST_ID);
	}

	//ost name
	if (j.contains(OST_NAME_KEY_NAME) && !j[OST_NAME_KEY_NAME].is_null() && j[OST_NAME_KEY_NAME].is_string()) {
		ostName = j.value(OST_NAME_KEY_NAME, "");
	}

	//create ticks (string)
	if (j.contains(CREATE_TICKS_STRING_KEY_NAME) && !j[CREATE_TICKS_STRING_KEY_NAME].is_null() && j[CREATE_TICKS_STRING_KEY_NAME].is_string()) {
		createTicksStr = j.value(CREATE_TICKS_STRING_KEY_NAME, "");
	}

	//create ticks (int64)
	if (j.contains(CREATE_TICKS_KEY_NAME) && !j[CREATE_TICKS_KEY_NAME].is_null() && j[CREATE_TICKS_KEY_NAME].is_number_integer()) {
		createTicks = j.value(CREATE_TICKS_KEY_NAME, DEFAULT_CREATE_TICKS);
	}

	//load song info
	//song name
	if (j.contains(SONG_NAME_KEY_NAME) && !j[SONG_NAME_KEY_NAME].is_null() && j[SONG_NAME_KEY_NAME].is_string()) {
		songName = j.value(SONG_NAME_KEY_NAME, "");
	}

	//song length
	if (j.contains(SONG_LENGTH_KEY_NAME) && !j[SONG_LENGTH_KEY_NAME].is_null() && j[SONG_LENGTH_KEY_NAME].is_string()) {
		songLength = j.value(SONG_LENGTH_KEY_NAME, "");
	}

	//song author name
	if (j.contains(SONG_AUTHOR_KEY_NAME) && !j[SONG_AUTHOR_KEY_NAME].is_null() && j[SONG_AUTHOR_KEY_NAME].is_string()) {
		songAuthorName = j.value(SONG_AUTHOR_KEY_NAME, "");
	}

	//mapper name
	if (j.contains(LEVEL_AUTHOR_KEY_NAME) && !j[LEVEL_AUTHOR_KEY_NAME].is_null() && j[LEVEL_AUTHOR_KEY_NAME].is_string()) {
		mapperName = j.value(LEVEL_AUTHOR_KEY_NAME, "");
	}

	//song preview time
	if (j.contains(SONG_PREVIEW_TIME_KEY_NAME) && !j[SONG_PREVIEW_TIME_KEY_NAME].is_null() && j[SONG_PREVIEW_TIME_KEY_NAME].is_number_float()) {
		songPreviewTime = j.value(SONG_PREVIEW_TIME_KEY_NAME, 0.0f);
	}

	//bpm
	if (j.contains(BPM_KEY_NAME) && !j[BPM_KEY_NAME].is_null() && j[BPM_KEY_NAME].is_string()) {
		try {
			bpm = std::stof(j[BPM_KEY_NAME].get<std::string>());
		}
		catch (...) {
			bpm = 0.0f;
		}
	}

	// project BPM mode
	if (shouldReadCustomInfoFields && j.contains(BPM_MODE_KEY_NAME) && !j[BPM_MODE_KEY_NAME].is_null() && j[BPM_MODE_KEY_NAME].is_string()) {
		projectBpmMode = projectBpmModeFromString(j.value(BPM_MODE_KEY_NAME, std::string("Single")));
	}

	//load youtube link info
	if (j.contains(YOUTUBE_LINK_KEY_NAME) && !j[YOUTUBE_LINK_KEY_NAME].is_null() && j[YOUTUBE_LINK_KEY_NAME].is_object()) {
		nlohmann::json jy = j[YOUTUBE_LINK_KEY_NAME];
		//video url
		if (jy.contains(YOUTUBE_VIDEO_URL_KEY_NAME) && !jy[YOUTUBE_VIDEO_URL_KEY_NAME].is_null() && jy[YOUTUBE_VIDEO_URL_KEY_NAME].is_string()) {
			videoUrl = jy.value(YOUTUBE_VIDEO_URL_KEY_NAME, "");
		}
		//youtube offset
		if (jy.contains(YOUTUBE_OFFSET_KEY_NAME) && !jy[YOUTUBE_OFFSET_KEY_NAME].is_null() && jy[YOUTUBE_OFFSET_KEY_NAME].is_number_float()) {
			youtubeOffset = jy.value(YOUTUBE_OFFSET_KEY_NAME, 0.0f);
		}
		//is mirror
		if (jy.contains(YOUTUBE_IS_MIRROR_KEY_NAME) && !jy[YOUTUBE_IS_MIRROR_KEY_NAME].is_null() && jy[YOUTUBE_IS_MIRROR_KEY_NAME].is_boolean()) {
			isMirror = jy.value(YOUTUBE_IS_MIRROR_KEY_NAME, false);
		}
	}

	return true;
}

bool BeatmapInfo::saveToFile(const std::string& path, bool isForceExport) {

	//check file existence
	if (!isForceExport && std::filesystem::exists(path)) {
		std::cerr << "file already exists!" << std::endl;
		return false;
	}

	//createticks and beatmapID invalidity check 
	if (createTicks == DEFAULT_CREATE_TICKS || canConvertToLongLong(createTicksStr)) {
		updateCreateTicks();
	}

	if (beatmapID == DEFAULT_BEATMAP_ID) {
		updateBeatmapIDFromCreateTicks();
	}

	// helper lambda to set string or null(if data is empty string)
	auto setStringOrNull = [&](nlohmann::json& obj, const std::string& key, const std::string& value)
		{
			if (value.empty())
				obj[key] = nullptr;
			else
				obj[key] = value;
		};

	nlohmann::json j;

	j[INFO_FORMAT_VERSION_KEY_NAME] = LATEST_INFO_FORMAT_VERSION_STRING;

	// difficulty paths
	for (int i = 0; i < DIFF_KEY_NAME.size(); i++) {
		setStringOrNull(j, DIFF_KEY_NAME.at(i), diffPaths.at(i));
	}

	// VR difficulty paths(not used, set to null)
	for (int i = 0; i < DIFF_VR_KEY_NAME.size(); i++) {
		setStringOrNull(j, DIFF_VR_KEY_NAME.at(i), std::string());
	}

	// paths
	setStringOrNull(j, SONG_PATH_KEY_NAME, songPath);
	setStringOrNull(j, COVER_PATH_KEY_NAME, coverPath);

	// sound paths
	for (int i = 0; i < static_cast<int>(SOUND_KEY_NAME.size()); i++) {
		setStringOrNull(j, SOUND_KEY_NAME.at(i), soundPaths.at(i));
	}

	// editor info
	setStringOrNull(j, EDITOR_VERSION_KEY_NAME, editorVersion);
	j[BEATMAP_ID_KEY_NAME] = beatmapID;
	j[OST_ID_KEY_NAME] = ostID;
	setStringOrNull(j, OST_NAME_KEY_NAME, ostName);

	// create ticks
	setStringOrNull(j, CREATE_TICKS_STRING_KEY_NAME, createTicksStr);
	j[CREATE_TICKS_KEY_NAME] = createTicks;

	// song info
	setStringOrNull(j, SONG_NAME_KEY_NAME, songName);
	setStringOrNull(j, SONG_LENGTH_KEY_NAME, songLength);
	setStringOrNull(j, SONG_AUTHOR_KEY_NAME, songAuthorName);
	setStringOrNull(j, LEVEL_AUTHOR_KEY_NAME, mapperName);
	j[SONG_PREVIEW_TIME_KEY_NAME] = songPreviewTime;

	// bpm(write as string)
	if (bpm == 0.0f)
		j[BPM_KEY_NAME] = nullptr;
	else
		j[BPM_KEY_NAME] = std::to_string(bpm);
	j[BPM_MODE_KEY_NAME] = projectBpmModeToString(projectBpmMode);

	// youtube info
	nlohmann::json jy;
	setStringOrNull(jy, YOUTUBE_VIDEO_URL_KEY_NAME, videoUrl);
	jy[YOUTUBE_OFFSET_KEY_NAME] = youtubeOffset;
	jy[YOUTUBE_IS_MIRROR_KEY_NAME] = isMirror;

	if (!jy.empty())
		j[YOUTUBE_LINK_KEY_NAME] = jy;
	else
		j[YOUTUBE_LINK_KEY_NAME] = nullptr;

	// write file
	std::ofstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open beatmapInfo file for writing: " << path << std::endl;
		return false;
	}

	file << j.dump(2);
	return true;
}

std::string BeatmapInfo::getEasy() const {
	return diffPaths.at(0);
}
std::string BeatmapInfo::getNormal() const {
	return diffPaths.at(1);
}
std::string BeatmapInfo::getHard() const {
	return diffPaths.at(2);
}
std::string BeatmapInfo::getExpert() const {
	return diffPaths.at(3);
}
std::string BeatmapInfo::getMaster() const {
	return diffPaths.at(4);
}
std::string BeatmapInfo::getLegend() const {
	return diffPaths.at(5);
}
std::string BeatmapInfo::getSongPath() const {
	return songPath;
}
std::string BeatmapInfo::getDiffPath(int n) const {
	switch (n) {
	case 0:
		return getEasy();
	case 1:
		return getNormal();
	case 2:
		return getHard();
	case 3:
		return getExpert();
	case 4:
		return getMaster();
	case 5:
		return getLegend();
	default:
		return std::string();
	}
}
std::array<bool, 6> BeatmapInfo::gethasDiff() const {
	std::array<bool, 6> existDiff;
	for (int i = 0; i < 6; i++) {
		existDiff.at(i) = hasDiff.at(i);
	}
	return existDiff;
}
// return has the difficulty
// 0-Easy, 1-Normal, 2-Hard, 3-Expert, 4-Master, 5-Legend
bool BeatmapInfo::getExistDiff(int n) const {
	if (n < 0 || n > 5) {
		return false;
	}
	return hasDiff.at(n);
}

std::string BeatmapInfo::getCoverPath() const {
	return coverPath;
}

std::string BeatmapInfo::getSoundPath(int n) const {
	if (n < 0 || n >= static_cast<int>(soundPaths.size())) {
		return std::string();
	}
	return soundPaths.at(n);
}

std::string BeatmapInfo::getEditorVersion() const {
	return editorVersion;
}

int BeatmapInfo::getBeatmapID() const {
	return beatmapID;
}

int BeatmapInfo::getOstID() const {
	return ostID;
}

std::string BeatmapInfo::getOstName() const {
	return ostName;
}

long long  BeatmapInfo::getCreateTicks() const {
	return createTicks;
}

std::string BeatmapInfo::getCreateTicksStr() const {
	return createTicksStr;
}

std::string BeatmapInfo::getSongName() const {
	return songName;
}

std::string BeatmapInfo::getSongLength() const {
	return songLength;
}

std::string BeatmapInfo::getSongAuthorName() const {
	return songAuthorName;
}

std::string BeatmapInfo::getMapperName() const {
	return mapperName;
}

float BeatmapInfo::getSongPreviewTime() const {
	return songPreviewTime;
}

float BeatmapInfo::getBPM() const {
	return bpm;
}

ProjectBpmMode BeatmapInfo::getProjectBpmMode() const {
	return projectBpmMode;
}

std::string BeatmapInfo::getVideoUrl() const {
	return videoUrl;
}

float BeatmapInfo::getYoutubeOffset() const {
	return youtubeOffset;
}

bool BeatmapInfo::getIsMirror() const {
	return isMirror;
}

bool BeatmapInfo::setDiffPath(int n, const std::string& path) {
	if (n < 0 || n >= LEVEL_NUM) {
		assert(false && "[BeatmapInfo::setDiffPath] Invalid difficulty index");
		return false;
	}
	diffPaths.at(n) = path;
	hasDiff.at(n) = path != "";
	return hasDiff.at(n);
}

void BeatmapInfo::setSongPath(const std::string& path) {
	songPath = path;
}

void BeatmapInfo::setSongName(const std::string& name) {
	songName = name;
}

void BeatmapInfo::setSongAuthorName(const std::string& name) {
	songAuthorName = name;
}

void BeatmapInfo::setMapperName(const std::string& name) {
	mapperName = name;
}

void BeatmapInfo::setSongLength(float length) {
	songLength = std::to_string(length);
}

void BeatmapInfo::setSongPreviewTime(float time) {
	songPreviewTime = time;
}

void BeatmapInfo::setBPM(float bpmValue) {
	bpm = bpmValue;
}

void BeatmapInfo::setProjectBpmMode(ProjectBpmMode mode) {
	projectBpmMode = mode;
}

void BeatmapInfo::setVideoUrl(const std::string& url) {
	videoUrl = url;
}

void BeatmapInfo::setYoutubeOffset(float offset) {
	youtubeOffset = offset;
}

void BeatmapInfo::setIsMirror(bool mirror) {
	isMirror = mirror;
}

void BeatmapInfo::setSongLengthAndBPM(const Beatmap& _beatmap) {
	setSongLength(_beatmap.getSongDuration());
	setBPM(_beatmap.getExportBPM());
	return;
}

bool BeatmapInfo::canConvertToLongLong(const std::string& str) const {
	long long v{};
	auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), v, 10);
	return ec == std::errc{} && ptr == str.data() + str.size();
}

void BeatmapInfo::updateCreateTicks() {
	// Calculate CreateTicks based on current system time in .NET ticks format(since 0001/01/01 00:00:00)
	long long constexpr DOTNET_EPOCH_DIFF_SECONDS = 62135596800LL;	// Difference in seconds between Unix epoch and .NET epoch
	long long constexpr TICKS_PER_SECOND = 10000000LL;

	auto now = std::chrono::system_clock::now();
	auto unix_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

	//update createTime and creatTicks info
	createTicks = (unix_ns / 100) + (DOTNET_EPOCH_DIFF_SECONDS * TICKS_PER_SECOND);
	createTicksStr = std::to_string(createTicks);

	return;
}

void BeatmapInfo::updateBeatmapIDFromCreateTicks() {
	// Update beatmapID based on the createTime
	beatmapID = static_cast<int>(createTicks ^ (createTicks >> 32));
}
