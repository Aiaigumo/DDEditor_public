#pragma once
#include <string>
#include <array>
#include "Beatmap.h"

// Defines the project-level BPM policy stored in beatmap info.
enum class ProjectBpmMode {
	Single,
	Multiple
};

// Owns beatmap metadata and difficulty file paths stored in the beatmap info file.
class BeatmapInfo {
public:
	BeatmapInfo();
	~BeatmapInfo() = default;

	void clear();

	//import/export
	bool loadFromFile(const std::string& path);
	bool saveToFile(const std::string& path, bool isForceExport);

	//getters
	std::string getEasy() const;
	std::string getNormal() const;
	std::string getHard() const;
	std::string getExpert() const;
	std::string getMaster() const;
	std::string getLegend() const;
	std::string getDiffPath(int n) const;
	std::string getSongPath() const;
	std::string getCoverPath() const;
	std::string getSoundPath(int n) const;
	std::array<bool, 6> gethasDiff() const;
	bool getExistDiff(int n) const;
	std::string getEditorVersion() const;
	int getBeatmapID() const;
	int getOstID() const;
	std::string getOstName() const;
	long long getCreateTicks() const;
	std::string getCreateTicksStr() const;
	std::string getSongName() const;
	std::string getSongLength() const;
	std::string getSongAuthorName() const;
	std::string getMapperName() const;
	float getSongPreviewTime() const;
	float getBPM() const;
	ProjectBpmMode getProjectBpmMode() const;
	std::string getVideoUrl() const;
	float getYoutubeOffset() const;
	bool getIsMirror() const;

	//setters
	bool setDiffPath(int n, const std::string& path);
	void setSongPath(const std::string& path);
	void setSongName(const std::string& name);
	void setSongAuthorName(const std::string& name);
	void setMapperName(const std::string& name);
	void setSongLength(float length);	
	void setSongPreviewTime(float time);
	void setBPM(float bpm);
	void setProjectBpmMode(ProjectBpmMode mode);
	void setVideoUrl(const std::string& url);
	void setYoutubeOffset(float offset);
	void setIsMirror(bool mirror);
	void setSongLengthAndBPM(const Beatmap&);

	inline static const int LEVEL_NUM = 6;

private:

	bool canConvertToLongLong(const std::string& str) const;
	void updateCreateTicks();
	void updateBeatmapIDFromCreateTicks();
	
	std::array<bool, 7> hasDiff;	//	Easy, Normal, Hard, Expert, Master, Legend

	//path info
	std::array<std::string, 7> diffPaths;
	std::string songPath;
	std::string coverPath;
	std::array<std::string, 10> soundPaths;

	//other info
	std::string editorVersion;
	int beatmapID;
	int ostID;
	std::string ostName;
	std::string createTicksStr;
	long long createTicks;	// 64bit int

	//song info
	std::string songName;
	std::string songLength;
	std::string songAuthorName;
	std::string mapperName;
	float songPreviewTime;
	float bpm;
	ProjectBpmMode projectBpmMode;

	//youtube link info
	std::string videoUrl;
	float youtubeOffset;
	bool isMirror;

	inline static const std::array<std::string, 7> DIFF_KEY_NAME = {
		"DD_Easy",
		"DD_Normal",
		"DD_Hard",
		"DD_Expert",
		"DD_Master",
		"DD_Legend",
		"DD_Env"
	};
	inline static const std::array<std::string, 4> DIFF_VR_KEY_NAME = {
		"DDVR_Easy",
		"DDVR_Normal",
		"DDVR_Hard",
		"DDVR_Env"
	};
	inline static const std::array<std::string, 10> SOUND_KEY_NAME = {
		"HandNoteHitSound",
		"FootNoteHitSound",
		"FootNoteMissSound",
		"LongNoteSlideSound",
		"ObstacleTrapHitSound",
		"GameFailedSound",
		"GameEndSound",
		"HandNoteMissSound",
		"GamePauseSound",
		"NewHighScoreSound"
	};
	inline static const std::string EDITOR_VERSION_KEY_NAME = "EditorVersion";
	inline static const std::string BEATMAP_ID_KEY_NAME = "BeatMapId";
	inline static const std::string OST_ID_KEY_NAME = "OstId";
	inline static const std::string OST_NAME_KEY_NAME = "OstName";
	inline static const std::string CREATE_TICKS_KEY_NAME = "CreateTicks";
	inline static const std::string CREATE_TICKS_STRING_KEY_NAME = "CreateTime";
	inline static const std::string SONG_NAME_KEY_NAME = "SongName";
	inline static const std::string SONG_PATH_KEY_NAME = "SongPath";
	inline static const std::string COVER_PATH_KEY_NAME = "CoverPath";
	inline static const std::string BPM_KEY_NAME = "Bpm";
	inline static const std::string BPM_MODE_KEY_NAME = "BpmMode";
	inline static const std::string SONG_LENGTH_KEY_NAME = "SongLength";
	inline static const std::string SONG_AUTHOR_KEY_NAME = "SongAuthorName";
	inline static const std::string SONG_PREVIEW_TIME_KEY_NAME = "SongPreviewSection";
	inline static const std::string LEVEL_AUTHOR_KEY_NAME = "LevelAuthorName";

	inline static const std::string YOUTUBE_LINK_KEY_NAME = "YoutubeVideos";
	inline static const std::string YOUTUBE_VIDEO_URL_KEY_NAME = "videoUrl";
	inline static const std::string YOUTUBE_OFFSET_KEY_NAME = "offset";
	inline static const std::string YOUTUBE_IS_MIRROR_KEY_NAME = "isMirror";

	inline static const std::string DEFAULT_EDITOR_VERSION = "1.6.0";
	inline static const std::string INFO_FORMAT_VERSION_KEY_NAME = "DDEditorBeatmapInfoFormatVersion";
	inline static const std::string LATEST_INFO_FORMAT_VERSION_STRING = "1.0.0";
	inline static const int DEFAULT_OST_ID = -1;
	inline static const int DEFAULT_BEATMAP_ID = -1;
	inline static const long long DEFAULT_CREATE_TICKS = 0LL;
};
