#pragma once
#include <vector>
#include <string>
#include "json.hpp"
#include "TimingList.h"
#include "Note.h"

// Defines how a difficulty exports its top-level BPM value.
enum class BpmExportMode {
	Single,
	Multiple
};

// Owns beatmap data, timing data, note groups, and import/export logic for a playable chart.
class Beatmap {
public:
	Beatmap();
	~Beatmap() = default;

	void clear();
	void clearNotes();
	
	// load beatmap level file (Both official format & DDEditor custom format)
	bool loadFromFile(const std::string& path);

	// export beatmap level file as json (custom format)
	bool exportJson(const std::string& path, bool isForcedExport) const;

	bool isLoaded() const;

	//note methods
	void addNote(const Note& note);
	bool removeNoteByNoteID(uint64_t noteID);
	void removeNoteAtIndex(size_t index);

	//group notes methods
	GroupNotes& addGroupNotes(GroupNotes& groupNotes);
	void addObstacleGroupNotes(double startTime, double duration, int x, int y);
	void removeGroupNotesAtIndex(size_t index);
	void removeGroupNotesWithGroupID(uint64_t groupID);
	bool removeLongNotesNode(uint64_t groupID, int index);

	//stream notes methods
	GroupNotes& addStreamGroupNotes(GroupNotes& groupNotes);
	GroupNotes& addStreamInstance(const std::vector<uint64_t>& streamGroupNotesIDs, double repeatIntervalSec, const Note& firstNode);
	void addStreamNode(uint64_t streamGroupID, const Note& node, double repeatIntervalSec);
	void removeStreamGroupNotesWithGroupID(uint64_t groupID);
	void changeStreamNodeByNoteID(uint64_t noteID, float dx, float dy, double dt);
	void setStreamNodePositionByNoteID(uint64_t noteID, float x, float y);
	void setStreamRepeatIntervalSec(uint64_t streamGroupID, double intervalSec);
	void setStreamSegmentDivisions(int divisions);
	void rebuildGeneratedStreamGroupNotes();	// regenerate stream instance groups based on current stream templates and timing data
	
	// reset sound flag had by each notes
	void resetSoundAlreadyPlayed();	

	void setSongDuration(double songDuration);
	double getSongDuration() const;

	static constexpr float getDefaultExportBPM() { return DEFAULT_EXPORT_BPM; }
	// set export BPM value used when BpmExportMode::Multiple is active
	void setRawExportBPM(float bpm);
	// get actually exported BPM value based on the current BPM export mode
	float getExportBPM() const;
	// get raw export BPM value
	float getRawExportBPM() const;
	BpmExportMode getBpmExportMode() const;
	void setBpmExportMode(BpmExportMode mode);

	double orderToSec(int order, double secPerBeat) const;
	int secToOrder(double timeSec, double secPerBeat) const;
	double calcTimeNormalized(int order, double secPerbeat) const;
	float calcPosition2DX(int x) const;
	float calcPosition2DY(int y) const;

	Note& getNoteAtIndex(size_t index);
	Note& getNoteByID(uint64_t noteID);
	GroupNotes& getGroupNotesAtIndex(size_t index);
	GroupNotes& getGroupNotesWithGroupID(uint64_t groupID);
	GroupNotes& getStreamGroupNotesWithGroupID(uint64_t groupID);
	const GroupNotes& getStreamGroupNotesWithGroupID(uint64_t groupID) const;
	std::vector<GroupNotes>& getGeneratedStreamGroupNotes();
	const std::vector<GroupNotes>& getGeneratedStreamGroupNotes() const;
	size_t getGroupNotesSize() const;
	size_t getVecGroupNotesLength() const;
	size_t getStreamGroupNotesSize() const;
	void organizeVecGroupNotesIndex();
	bool existInStreamTemplateWithGroupID(uint64_t groupID) const;

	// method for TimingList
	void addTiming(float BPM, double offset);
	TimingList& getTimingList();
	const TimingList& getTimingList() const;
	size_t getTimingListSize() const;
	size_t getCurrentTimingIndex() const;
	bool timingListisEmpty() const;
	// update which timing to use
	bool updateTimingList(double currentTime);	
	void setBPM(float bpm);
	float getBPM() const;
	float getFirstBPM() const;
	// set  the timing offset in use at that time (depends on timingIndex which updated in Beatmap::updateTimingList)
	void setOffset(double offset);
	// get the timing offset in use at that time (depends on timingIndex which updated in Beatmap::updateTimingList)
	double getOffset() const;
	double getFirstOffset() const;
	// get raw offset value use for calc in this application
	double getActualOffset() const;
	// get raw offset value use for calc in this application
	double getActualFirstOffset() const;
	double getOffsetInNoteTime() const;
	void setOffsetInEditor(double offset);
	double getOffsetInEditor() const;
	double getsecPerBeat() const;
	double getFirstSecPerBeat() const;
	int getSubbeat() const;
	int getCurrentBeat() const;
	double calcCurrentTime(int beat) const;
	bool checkNextBeat(double currentTime);
	void resetBeat();

private:
	bool loaded;
	TimingList timingList;
	std::vector<Note> notes;
	std::vector<GroupNotes> vecGroupNotes;
	std::vector<GroupNotes> vecStreamTemplates;
	std::vector<GroupNotes> vecGeneratedStreamGroupNotes;
	int streamSegmentDivisions = 32;
	double songDuration;
	float exportBPM;
	BpmExportMode bpmExportMode;
	inline static constexpr float DEFAULT_EXPORT_BPM = 220;
	inline static constexpr int ORDER_PER_BEAT = 24;

	void sortNotesByTime();
	void sortGroupNotesByTime();
	bool loadOfficialFormat(const nlohmann::json& j);
	bool loadCustomFormat(const nlohmann::json& j);

	/* official json format key */
	inline static const std::string BEATMAP_DATA_KEY = "data";
	inline static const std::string DATA_NAME_KEY = "name";
	inline static const std::string DATA_INTERVAL_PER_SECOND_KEY = "intervalPerSecond";
	inline static const std::string DATA_GRID_SIZE_KEY = "gridSize";
	inline static const std::string DATA_PLANE_SIZE_KEY = "planeSize";
	inline static const std::string DATA_ORDER_COUNT_PER_BEAT_KEY = "orderCountPerBeat";
	inline static const std::string DATA_SPHERE_NODES_KEY = "sphereNodes";
	inline static const std::string DATA_LINE_NODES_KEY = "lineNodes";
	inline static const std::string DATA_EFFECT_NODES_KEY = "effectNodes";
	inline static const std::string DATA_ROADBLOCK_NODES_KEY = "roadBlockNodes";
	inline static const std::string DATA_TRAP_NODES_KEY = "trapNodes";
	inline static const std::string BEATMAP_BEATSUBS_KEY = "beatsubs";
	inline static const std::string BEATMAP_BPM_KEY = "BPM";
	inline static const std::string BEATMAP_OFFSET_KEY = "songStartOffset";
	inline static const std::string BEATMAP_NPS_KEY = "NPS";
	inline static const std::string BEATMAP_DEVELOPER_MODE_KEY = "developerMode";
	inline static const std::string BEATMAP_NOTE_SPEED_KEY = "noteSpeed";
	inline static const std::string BEATMAP_NOTE_JUMP_OFFSET_KEY = "noteJumpOffset";
	inline static const std::string BEATMAP_INTERVAL_KEY = "interval";
	inline static const std::string BEATMAP_INFO_KEY = "info";
	inline static const std::string NOTE_ORDER_KEY = "noteOrder";
	inline static const std::string NOTE_TIME_KEY = "time";
	inline static const std::string NOTE_POSITION_KEY = "position";
	inline static const std::string NOTE_POSITION_2D_KEY = "position2D";
	inline static const std::string NOTE_SIZE_KEY = "size";
	inline static const std::string NOTE_POSITION_OFFSET_KEY = "positionOffset";
	inline static const std::string NOTE_X_KEY = "x";
	inline static const std::string NOTE_Y_KEY = "y";
	inline static const std::string NOTE_Z_KEY = "z";
	inline static const std::string NOTE_TYPE_KEY = "noteType";
	inline static const std::string NOTE_IS_PLAY_AUDIO_KEY = "isPlayAudio";
	inline static const std::string NOTE_LINE_GROUP_ID_KEY = "lineGroupId";
	inline static const std::string NOTE_INDEX_IN_LINE_KEY = "indexInLine";
	inline static const std::string NOTE_LENGTH_KEY = "length";
	inline static const std::string NOTE_DURATION_KEY = "duration";
	inline static const std::string NOTE_START_POSITION_SELECT_ORDER_KEY = "startPosSelctOrder";
	inline static const std::string NOTE_END_POSITION_SELECT_ORDER_KEY = "endPosSelctOrder";

	/* DDEditor custom json format key */
	inline static const std::string BEATMAP_FORMAT_VERSION_KEY = "DDEditorExportFormatVersion";
	inline static const std::string BPM_EXPORT_MODE_KEY = "BpmExportMode";
	inline static const std::string NOTE_TIME_SEC_KEY = "timeSec";
	inline static const std::string NOTE_START_TIME_SEC_KEY = "startTimeSec";
	inline static const std::string NOTE_END_TIME_SEC_KEY = "endTimeSec";
	inline static const std::string NOTE_IS_GENERATED_FROM_STREAM_GROUP_KEY = "isGeneratedFromStreamGroup";
	inline static const std::string NOTE_FLOAT_X_KEY = "floatX";
	inline static const std::string NOTE_FLOAT_Y_KEY = "floatY";
	inline static const std::string NOTE_SEGMENT_MODE_KEY = "segmentMode";
	inline static const std::string BEATMAP_DURATION_KEY = "songDuration";
	inline static const std::string BEATMAP_TIMINGLIST_KEY = "timingList";
	inline static const std::string TIMING_BPM_KEY = "BPM";
	inline static const std::string TIMING_OFFSET_KEY = "offset";
	inline static const std::string DATA_STREAM_GROUPS_KEY = "streamGroups";
	inline static const std::string DATA_STREAM_TEMPLATES_KEY = "streamTemplates";
	inline static const std::string STREAM_TEMPLATE_GROUP_IDS_KEY = "streamTemplateGroupIds";
	inline static const std::string STREAM_REPEAT_INTERVAL_SEC_KEY = "repeatIntervalSec";
	inline static const std::string STREAM_NODES_KEY = "nodes";
	inline static const std::string STREAM_TEMPLATE_GROUP_ID_FILE_KEY = "groupId";
	inline static const std::string STREAM_TEMPLATE_NOTES_KEY = "notes";
	inline static const std::string STREAM_TEMPLATE_GROUP_TYPE_KEY = "groupType";

	/* data value */
	inline static const std::string OFFICIAL_FORMAT_VERSION_STRING = "official";
	inline static const std::string LATEST_CUSTOM_FORMAT_VERSION_STRING = "1.0.0";
};
