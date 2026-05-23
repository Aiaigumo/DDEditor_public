#include "Note.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cmath>

Note::Note(double timeSec_, int x_, int y_, NoteType type_)
	: noteID(lastNoteID++), timeSec(timeSec_), x(x_), y(y_), floatX(static_cast<float>(x_)), floatY(static_cast<float>(y_)), type(type_), duration(0.0f), index(-1), GroupID(0), alreadyPlayed(false)
{
}
Note::Note(double timeSec_, int x_, int y_, NoteType type_, double duration_)
	: noteID(lastNoteID++), timeSec(timeSec_), x(x_), y(y_), floatX(static_cast<float>(x_)), floatY(static_cast<float>(y_)), type(type_), duration(duration_), index(-1), GroupID(0), alreadyPlayed(false)
{
}
Note::Note(double timeSec_, int x_, int y_, NoteType type_, int index_)
	: noteID(lastNoteID++), timeSec(timeSec_), x(x_), y(y_), floatX(static_cast<float>(x_)), floatY(static_cast<float>(y_)), type(type_), duration(0.0f), index(index_), GroupID(0), alreadyPlayed(false)
{
}
Note::Note(double timeSec_, int x_, int y_, NoteType type_, double duration_, int index_)
	: noteID(lastNoteID++), timeSec(timeSec_), x(x_), y(y_), floatX(static_cast<float>(x_)), floatY(static_cast<float>(y_)), type(type_), duration(duration_), index(index_), GroupID(0), alreadyPlayed(false)
{
}

// Getters
double Note::getTime() const {
	return timeSec;
}

int Note::getX() const {
	return x;
}

int Note::getY() const {
	return y;
}

float Note::getFloatX() const {
	return floatX;
}

float Note::getFloatY() const {
	return floatY;
}

NoteType Note::getType() const {
	return type;
}

std::string Note::getTypeString() const {
	switch (type) {
	case NoteType::LEFT_HAND: return "Left Hand";
	case NoteType::RIGHT_HAND: return "Right Hand";
	case NoteType::LEFT_FOOT: return "Left Foot";
	case NoteType::RIGHT_FOOT: return "Right Foot";
	case NoteType::LONG_LEFT_FOOT: return "Long Left Foot";
	case NoteType::LONG_RIGHT_FOOT: return "Long Right Foot";
	case NoteType::BAR: return "Bar";
	case NoteType::OBS: return "Obstacle Start";
	case NoteType::OBS_END: return "Obstacle End";
	case NoteType::TRAP: return "Trap";
	case NoteType::STREAM_NODE: return "Stream Node";
	default:
		std::cerr << "[Note.cpp] Note::getTypeString: Note type is unexpected" << std::endl;
		assert(!"[Note.cpp] Note::getTypeString: Note type is unexpected");
		return "None";
	}

}

double Note::getDuration() const {
	return duration;
}

int Note::getIndex() const {
	return index;
}

uint64_t Note::getGroupID() const {
	return GroupID;
}

bool Note::isInGroupNotes() const {
	return getGroupID() != 0;
}

bool Note::getAlreadyPlayed() const {
	return alreadyPlayed;
}

uint64_t Note::getNoteID() const {
	return noteID;
}

SegmentMode Note::getSegmentMode() const {
	return streamNodeType;
}

// Setters
void Note::setTime(double timeSec_) {
	timeSec = timeSec_;
}

void Note::setX(int x_) {
	x = x_;
	floatX = static_cast<float>(x_);
}

void Note::setY(int y_) {
	y = y_;
	floatY = static_cast<float>(y_);
}

void Note::setFloatPosition(float x_, float y_) {
	floatX = x_;
	floatY = y_;
	x = static_cast<int>(std::lround(x_));
	y = static_cast<int>(std::lround(y_));
}

void Note::setType(NoteType type_) {
	type = type_;
}

void Note::setDuration(double duration_) {
	duration = duration_;
}

void Note::setIndex(int index_) {
	index = index_;
}

void Note::setGroupID(uint64_t groupID_) {
	GroupID = groupID_;
}

void Note::setAlreadyPlayed(bool alreadyPlayed_) {
	alreadyPlayed = alreadyPlayed_;
}

void Note::setSegmentMode(SegmentMode mode) {
	streamNodeType = mode;
}

NoteType Note::intToNoteType(int typeInt) {
	switch (typeInt) {
	case 6: return	NoteType::LEFT_HAND;
	case 7: return	NoteType::RIGHT_HAND;
	case 2:
	case 8: return	NoteType::LEFT_FOOT;
	case 1:
	case 9: return	NoteType::RIGHT_FOOT;
	case 12: return NoteType::LONG_LEFT_FOOT;
	case 13: return NoteType::LONG_RIGHT_FOOT;
	case 14: return NoteType::BAR;
	case 15: return NoteType::OBS;
	case 16: return NoteType::OBS_END;
	case 17: return NoteType::TRAP;
	case 30: return NoteType::STREAM_NODE;
	default:
		std::cerr << "[Note.cpp] Note::intToNoteType unexpected int value" << std::endl;
		assert(!"[Note.cpp] Note::intToNoteType unexpected int value");
		return NoteType::NONE;	//default is none
	}
}

int Note::noteTypeToInt(NoteType type) {
	switch (type) {
	case NoteType::LEFT_HAND:		return 6;
	case NoteType::RIGHT_HAND:		return 7;
	case NoteType::LEFT_FOOT:		return 8;
	case NoteType::RIGHT_FOOT:		return 9;
	case NoteType::LONG_LEFT_FOOT:		return 12;
	case NoteType::LONG_RIGHT_FOOT:		return 13;
	case NoteType::BAR:		return 14;
	case NoteType::OBS:		return 15;
	case NoteType::OBS_END:	return 16;
	case NoteType::TRAP:		return 17;
	case NoteType::STREAM_NODE:	return 30;
	default:
		std::cerr << "[Note.cpp] Note::noteTypeToInt unexpected type value" << std::endl;
		assert(!"[Note.cpp] Note::noteTypeToInt  unexpected type value");
		return 0;
	}
}



/* GroupNotes Method */

GroupNotes::GroupNotes(GroupType _groupType) 
	:groupID(lastGroupID++), groupType(_groupType)
{
	if(!(_groupType == GroupType::LONG_NOTE || _groupType == GroupType::OBSTACLE || _groupType == GroupType::DEFAULT)) {
		std::cerr << "[Note.cpp] GroupNotes::GroupNotes unexpected group type" << std::endl;
		assert(!"[Note.cpp] GroupNotes::GroupNotes unexpected group type");
	}
	
}

GroupNotes::GroupNotes(double _startTimeSec, double _durationSec, int _x, int _y)
	:groupID(lastGroupID++), groupType(GroupType::OBSTACLE)
{
	if (_durationSec <= 0.0) {
		std::cerr << "[Note.cpp] GroupNotes::GroupNotes durationSec should be positive" << std::endl;
		assert(!"[Note.cpp] GroupNotes::GroupNotes durationSec should be positive");
	}
	addNote(Note(_startTimeSec, _x, _y, NoteType::OBS, _durationSec, 0));
	addNote(Note(_startTimeSec + _durationSec, _x, _y, NoteType::OBS_END, _durationSec, 1));
}

GroupNotes::GroupNotes(const std::vector<uint64_t>& _streamGroupNotesIDs, double _repeatIntervalSec)
	:groupID(lastGroupID++), groupType(GroupType::STREAM), streamGroupNotesIDs(_streamGroupNotesIDs), repeatIntervalSec(_repeatIntervalSec)
{
	if (_repeatIntervalSec <= 0.0) {
		repeatIntervalSec = 1.0;
		std::cerr << "[Note.cpp] GroupNotes::GroupNotes repeatIntervalSec should be positive" << std::endl;
		assert(!"[Note.cpp] GroupNotes::GroupNotes repeatIntervalSec should be positive");
	}
}


void GroupNotes::clearNotes() {
	notes = std::vector<Note>();
	return;
}

void GroupNotes::addRawNote(Note _note) {
	_note.setGroupID(getGroupID()); // change to groupID of this group 
	notes.push_back(_note);
	sortNotesByIndex();
	return;
}

void GroupNotes::addNote(Note _note) {
	if (getGroupType() == GroupType::LONG_NOTE) {
		addLongNotesNode(_note);
		return;
	}

	_note.setGroupID(getGroupID()); // change to groupID of this group 
	notes.push_back(_note);
	sortNotesByTime();
	organizeNotesIndex();
	return;
}

void GroupNotes::addLongNotesNode(Note _note) {
	if (getGroupType() != GroupType::LONG_NOTE) {
		std::cerr << "[Note.cpp] GroupNotes::addLongNotesNode type is not long note" << std::endl;
		assert(!"[Note.cpp] GroupNotes::addLongNotesNode type is not long note");
	}
	_note.setGroupID(getGroupID()); // change to groupID of this group
	notes.push_back(_note);
	sortNotesByTime();
	organizeNotesIndex();
	applyConstraintsLongNoteNodes();
	return;
}

void GroupNotes::addNextLongNotesNode(Note _note) {
	if (getGroupType() != GroupType::LONG_NOTE) {
		std::cerr << "[Note.cpp] GroupNotes::addNextLongNotesNode type is not long note" << std::endl;
		assert(!"[Note.cpp] GroupNotes::addNextLongNotesNode type is not long note");
		return;
	}
	_note.setGroupID(getGroupID()); // change to groupID of this group
	// set index to next number
	if (!notes.empty()) {
		_note.setIndex(notes.back().getIndex() + 1);
	}
	else {
		_note.setIndex(0);
	}

	notes.push_back(_note);

	sortNotesByIndex();
	organizeNotesIndex();
	applyConstraintsLongNoteNodes();
	return;
}

void GroupNotes::changeLongNotesNodeByNoteID(uint64_t noteID, int dx, int dy, double dt) {
	if (getGroupType() != GroupType::LONG_NOTE) {
		throw std::logic_error("[Note.cpp] GroupNotes::changeLongNotesNodeByNoteID: group type is not LONG_NOTE");
	}

	auto it = std::find_if(notes.begin(), notes.end(), [noteID](const Note& note) {
		return note.getNoteID() == noteID;
		});
	if (it == notes.end()) {
		throw std::out_of_range("[Note.cpp] GroupNotes::changeLongNotesNodeByNoteID: noteID not found");
	}

	it->setX(it->getX() + dx);
	it->setY(it->getY() + dy);
	it->setTime(it->getTime() + dt);

	sortNotesByTime();
	organizeNotesIndex();
	applyConstraintsLongNoteNodes();
}

void GroupNotes::changeStreamNodeByNoteID(uint64_t noteID, float dx, float dy, double dt) {
	if (getGroupType() != GroupType::STREAM) {
		throw std::logic_error("[Note.cpp] GroupNotes::changeStreamNodeByNoteID: group type is not STREAM");
	}

	auto it = std::find_if(notes.begin(), notes.end(), [noteID](const Note& note) {
		return note.getNoteID() == noteID;
		});
	if (it == notes.end()) {
		throw std::out_of_range("[Note.cpp] GroupNotes::changeStreamNodeByNoteID: noteID not found");
	}

	it->setFloatPosition(it->getFloatX() + dx, it->getFloatY() + dy);
	it->setTime(it->getTime() + dt);

	sortNotesByTime();
	organizeNotesIndex();
}

bool GroupNotes::removeNote(size_t _index) {
	if (_index >= notes.size()) throw std::out_of_range("[Note.cpp] GroupNotes::removeNote: index out of range");
	notes.erase(notes.begin() + _index);

	if (size() > 0) {
		organizeNotesIndex();
		return false;
	}
	else {
		return true;
	}
}

bool GroupNotes::removeLongNotesNode(int index) {
	if (getGroupType() != GroupType::LONG_NOTE) {
		std::cerr << "[Note.cpp] GroupNotes::removeLongNotesNode type is not Long Note" << std::endl;
		assert("[Note.cpp] GroupNotes::removeLongNotesNode type is not Long Note");
		return false;
	}

	auto it = std::find_if(notes.begin(), notes.end(),
		[index](Note& note) { return note.getIndex() == index; });

	if (it != notes.end()) {
		notes.erase(it);
	}

	if (size() > 0) {
		organizeNotesIndex();
		return false;
	}
	else {
		return true;
	}
}

size_t GroupNotes::size() const {
	return notes.size();
}

Note& GroupNotes::getNoteAtIndex(size_t index) {
	if (index >= notes.size()) throw std::out_of_range("[Note.cpp] GroupNotes::getNoteAtIndex: index out of range");
	return notes.at(index);
}

const Note& GroupNotes::getNoteAtIndex(size_t index) const {
	if (index >= notes.size()) throw std::out_of_range("[Note.cpp] GroupNotes::getNoteAtIndex: index out of range");
	return notes.at(index);
}

/* basically use when LongNotes/stream(group constructed single type notes)*/
/* return first notes type */
NoteType GroupNotes::getNoteType() const {
	if (!notes.empty()) {
		return notes.at(0).getType();
	}
	return NoteType::NONE;
}

GroupType GroupNotes::getGroupType() const {
	return groupType;
}

std::string GroupNotes::getGroupTypeString() const {
	switch (groupType) {
	case GroupType::LONG_NOTE: return "Long Note";
	case GroupType::OBSTACLE: return "Obstacle";
	case GroupType::STREAM: return "Stream";
	case GroupType::DEFAULT: return "Default";
	default:
		std::cerr << "[Note.cpp] GroupNotes::getGroupTypeString: Group type is unexpected" << std::endl;
		assert(!"[Note.cpp] GroupNotes::getGroupTypeString: Group type is unexpected");
		return "None";
	}
}

uint64_t GroupNotes::getGroupID() const {
	return groupID;
}

/* return time of the first note in the group */
double GroupNotes::getStartTime() const {
	if (!notes.empty()) {
		return notes.at(0).getTime();
	}
	return 0.0;
}

/* return time of the final note in the group  */
double GroupNotes::getEndTime() const {
	if (!notes.empty()) {
		return notes.back().getTime();
	}
	return 0.0;
}

void GroupNotes::setObstacleX(int x) {
	if (groupType != GroupType::OBSTACLE) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleX: Group type is not obstacle" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleX: Group type is not obstacle");
		return;
	}
	if (notes.size() != 2) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleX: Obstacle group does not have 2 notes" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleX: Obstacle group does not have 2 notes");
		return;
	}
	notes.at(0).setX(x);
	notes.at(1).setX(x);
	return;
}
void GroupNotes::setObstacleY(int y) {
	if (groupType != GroupType::OBSTACLE) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleY: Group type is not obstacle" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleY: Group type is not obstacle");
		return;
	}
	if (notes.size() != 2) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleY: Obstacle group does not have 2 notes" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleY: Obstacle group does not have 2 notes");
		return;
	}
	notes.at(0).setY(y);
	notes.at(1).setY(y);
	return;
}

void GroupNotes::setObstacleStartTime(double time) {
	if (groupType != GroupType::OBSTACLE) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleStartTime: Group type is not obstacle" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleStartTime: Group type is not obstacle");
		return;
	}
	if (notes.size() != 2) {
		std::cerr << "[Note.cpp] GroupNotes::setObstacleStartTime: Obstacle group does not have 2 notes" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setObstacleStartTime: Obstacle group does not have 2 notes");
		return;
	}
	notes.at(0).setTime(time); // start note
	notes.at(1).setTime(time + notes.at(0).getDuration()); // update end note time
	return;
}

void GroupNotes::setObstacleDuration(double newDuration) {
	if (groupType != GroupType::OBSTACLE) {
		std::cerr << "[Note.cpp] GroupNotes::changeObstacleDuration: Group type is not obstacle" << std::endl;
		assert(!"[Note.cpp] GroupNotes::changeObstacleDuration: Group type is not obstacle");
		return;
	}
	if (notes.size() != 2) {
		std::cerr << "[Note.cpp] GroupNotes::changeObstacleDuration: Obstacle group does not have 2 notes" << std::endl;
		assert(!"[Note.cpp] GroupNotes::changeObstacleDuration: Obstacle group does not have 2 notes");
		return;
	}
	notes.at(0).setDuration(newDuration); // start note
	notes.at(1).setDuration(newDuration); // end note
	notes.at(1).setTime(notes.at(0).getTime() + newDuration); // update end note time
	return;
}

double GroupNotes::getStreamRepeatIntervalSec() const {
	if (groupType != GroupType::STREAM) {
		std::cerr << "[Note.cpp] GroupNotes::getStreamRepeatIntervalSec: Group type is not stream" << std::endl;
		assert(!"[Note.cpp] GroupNotes::getStreamRepeatIntervalSec: Group type is not stream");
	}
	return repeatIntervalSec;
}

void GroupNotes::setStreamRepeatIntervalSec(double intervalSec) {
	if (groupType != GroupType::STREAM) {
		std::cerr << "[Note.cpp] GroupNotes::setStreamRepeatIntervalSec: Group type is not stream" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setStreamRepeatIntervalSec: Group type is not stream");
		return;
	}
	if (intervalSec <= 0.0) {
		std::cerr << "[Note.cpp] GroupNotes::setStreamRepeatIntervalSec: Interval should be positive" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setStreamRepeatIntervalSec: Interval should be positive");
		intervalSec = 1.0;
	}
	repeatIntervalSec = intervalSec;
	return;
}

const std::vector<uint64_t>& GroupNotes::getStreamGroupNotesIDs() const {
	if (groupType != GroupType::STREAM) {
		std::cerr << "[Note.cpp] GroupNotes::getStreamGroupNotesIDs: Group type is not stream" << std::endl;
		assert(!"[Note.cpp] GroupNotes::getStreamGroupNotesIDs: Group type is not stream");
	}
	return streamGroupNotesIDs;
}

void GroupNotes::setStreamGroupNotesIDs(const std::vector<uint64_t>& groupIDs) {
	if (groupType != GroupType::STREAM) {
		std::cerr << "[Note.cpp] GroupNotes::setStreamGroupNotesIDs: Group type is not stream" << std::endl;
		assert(!"[Note.cpp] GroupNotes::setStreamGroupNotesIDs: Group type is not stream");
		return;
	}
	streamGroupNotesIDs = groupIDs;
}

void GroupNotes::addStreamGroupNotesID(uint64_t groupID) {
	if (groupType != GroupType::STREAM) {
		std::cerr << "[Note.cpp] GroupNotes::addStreamGroupNotesID: Group type is not stream" << std::endl;
		assert(!"[Note.cpp] GroupNotes::addStreamGroupNotesID: Group type is not stream");
		return;
	}
	streamGroupNotesIDs.push_back(groupID);
}


void GroupNotes::applyConstraintsLongNoteNodes() {
	if (groupType != GroupType::LONG_NOTE || notes.size() <= 1) {
		return;
	}

	const int baseY = notes.at(0).getY();
	for (size_t i = 1; i < notes.size(); ++i) {
		notes.at(i).setY(baseY);
		if (notes.at(i).getTime() < notes.at(i - 1).getTime()) {
			notes.at(i).setTime(notes.at(i - 1).getTime());
		}
	}
}

void GroupNotes::sortNotesByTime() {
	std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
		return a.getTime() < b.getTime();
		});
	return;
}

void GroupNotes::sortNotesByIndex() {
	std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
		return a.getIndex() < b.getIndex();
		});
	return;
}

void GroupNotes::organizeNotesIndex() {
	int i = 0;
	for (Note& n : notes) {
		n.setIndex(i++);
	}
	return;
}

