#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include "CurvePath.h"

enum class NoteType {
	LEFT_HAND,		// Left Hand 
	RIGHT_HAND,		// Right Hand
	LEFT_FOOT,		// Left Foot
	RIGHT_FOOT,		// Right Foot
	LONG_LEFT_FOOT, // Line Left Foot
	LONG_RIGHT_FOOT,// Line Right Foot
	BAR,			// bar
	OBS,			// Obstacle
	OBS_END,		// Obstacle(endPosition) 
	TRAP,			// Trap 
	STREAM_NODE, 
	NONE //error
};

enum class GroupType {
	LONG_NOTE,
	OBSTACLE,
	STREAM,
	DEFAULT,
	NONE //error
};

// Represents one beatmap note or editable node with timing, position, type, and group identity.
class Note {
public:

	Note(double timeSec, int x, int y, NoteType type);
	Note(double timeSec, int x, int y, NoteType type, double duration);
	// mainly used for long notes node
	Note(double timeSec, int x, int y, NoteType type, int index);
	// mainly used for Obstacle node (start & end)
	Note(double timeSec, int x, int y, NoteType type, double duration, int index);
	~Note() = default;

	// Getters
	double getTime() const;
	int getX() const;
	int getY() const;
	float getFloatX() const;
	float getFloatY() const;
	NoteType getType() const;
	std::string getTypeString() const;
	double getDuration() const;
	int getIndex() const;
	uint64_t getGroupID() const;
	bool isInGroupNotes() const;
	bool getAlreadyPlayed() const;
	uint64_t getNoteID() const;
	SegmentMode getSegmentMode() const;

	// Setters
	void setTime(double timeSec_);
	void setX(int x_);
	void setY(int y_);
	void setFloatPosition(float x_, float y_);
	void setType(NoteType type_);
	void setDuration(double durationSec_);
	void setIndex(int index_);
	void setGroupID(uint64_t groupID_);
	void setAlreadyPlayed(bool);
	void setSegmentMode(SegmentMode);
	static NoteType intToNoteType(int typeInt);
	static int noteTypeToInt(NoteType type);

private:
	inline static uint64_t lastNoteID = 0;
	uint64_t noteID;
	double timeSec;        // Seconds
	int x;
	int y;
	float floatX;
	float floatY;
	NoteType type;
	double duration;    // only obstacle
	int index;          // long note node index
	uint64_t GroupID;     // group ID(only GroupNotes)
	bool alreadyPlayed = false; // whether sound already played
	SegmentMode streamNodeType = SegmentMode::CatmullRom;
};

// Owns notes that behave as one logical group, such as long notes, obstacles, and streams.
class GroupNotes {
public:
	GroupNotes(GroupType);
	// constructor for obstacle
	GroupNotes(double startTimeSec, double durationSec, int x, int y);
	// constructor for stream
	GroupNotes(const std::vector<uint64_t>& streamGroupNotesIDs, double repeatIntervalSec = 1.0);
	~GroupNotes() = default;
	void clearNotes();

	// note manipulation
	void addRawNote(Note);
	void addNote(Note);
	void addLongNotesNode(Note);
	void addNextLongNotesNode(Note);
	void changeLongNotesNodeByNoteID(uint64_t noteID, int dx, int dy, double dt);
	void changeStreamNodeByNoteID(uint64_t noteID, float dx, float dy, double dt);
	bool removeNote(size_t);
	bool removeLongNotesNode(int);

	// Getters
	size_t size() const;
	Note& getNoteAtIndex(size_t);
	const Note& getNoteAtIndex(size_t) const;
	NoteType getNoteType() const;
	GroupType getGroupType() const;
	std::string getGroupTypeString() const;
	uint64_t getGroupID() const;
	double getStartTime() const;
	double getEndTime() const;
	
	// Setters
	void setObstacleX(int);
	void setObstacleY(int);
	void setObstacleStartTime(double);
	void setObstacleDuration(double);

	//stream related getters and setters
	double getStreamRepeatIntervalSec() const;
	void setStreamRepeatIntervalSec(double);
	const std::vector<uint64_t>& getStreamGroupNotesIDs() const;
	void setStreamGroupNotesIDs(const std::vector<uint64_t>& groupIDs);
	void addStreamGroupNotesID(uint64_t groupID);

	// Utility
	void organizeNotesIndex();

private:
	inline static uint64_t lastGroupID = 1;
	uint64_t groupID;
	GroupType groupType = GroupType::NONE;
	std::vector<Note> notes{};

	//for stream
	std::vector<uint64_t> streamGroupNotesIDs{};
	double repeatIntervalSec = 1.0;

	void applyConstraintsLongNoteNodes();
	void sortNotesByTime();
	void sortNotesByIndex();
};
