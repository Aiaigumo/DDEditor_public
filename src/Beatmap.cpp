#include "Beatmap.h"
#include "EditorUtility.h"
#include "json.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

namespace {
std::string bpmExportModeToString(BpmExportMode mode) {
	switch (mode) {
	case BpmExportMode::Multiple:
		return "Multiple";
	case BpmExportMode::Single:
	default:
		return "Single";
	}
}

BpmExportMode bpmExportModeFromString(const std::string& modeString) {
	// Legacy value used before BpmExportMode values were renamed to Single/Multiple.
	if (modeString == "Multiple" || modeString == "OutputBpm") {
		return BpmExportMode::Multiple;
	}
	return BpmExportMode::Single;
}
}

Beatmap::Beatmap() {
	clear();
}

void Beatmap::clear() {
	songDuration = 0.0;
	loaded = false;
	timingList = TimingList();
	notes = std::vector<Note>();
	vecGroupNotes = std::vector<GroupNotes>();
	vecStreamTemplates = std::vector<GroupNotes>();
	vecGeneratedStreamGroupNotes = std::vector<GroupNotes>();
	exportBPM = DEFAULT_EXPORT_BPM;
	bpmExportMode = BpmExportMode::Single;
}

void Beatmap::clearNotes() {
	notes.clear();
	vecGroupNotes.clear();
	vecStreamTemplates.clear();
	vecGeneratedStreamGroupNotes.clear();
}

bool Beatmap::loadFromFile(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Failed to open beatmap file: " << path << std::endl;
		return false;
	}

	//initialize 
	clear();

	nlohmann::json j;
	file >> j;

	// Check for required fields
	// std::cout << "Loading beatmap from: " << path << std::endl;
	bool hasCustomFormatVersion =
		j.contains(BEATMAP_FORMAT_VERSION_KEY) &&
		!j[BEATMAP_FORMAT_VERSION_KEY].is_null() &&
		j[BEATMAP_FORMAT_VERSION_KEY].is_string();
	if (hasCustomFormatVersion) {
		const std::string formatVersion =
			j.value(BEATMAP_FORMAT_VERSION_KEY, std::string());
		if (formatVersion != LATEST_CUSTOM_FORMAT_VERSION_STRING) {
			std::cerr << "Unsupported DDEditor beatmap format version: "
				<< formatVersion << std::endl;
		}
		return loadCustomFormat(j);
	}

	return loadOfficialFormat(j);
}

bool Beatmap::loadOfficialFormat(const nlohmann::json& j) {

	if (!j.contains(BEATMAP_DATA_KEY))
	{
		std::cerr << "Invalid beatmap format: missing 'data' field." << std::endl;
		return false;
	}
	nlohmann::json data = j[BEATMAP_DATA_KEY];

	if (!data.contains(DATA_SPHERE_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'sphereNodes' field." << std::endl;
		return false;
	}
	nlohmann::json sphereNodes = data[DATA_SPHERE_NODES_KEY];

	if (!data.contains(DATA_LINE_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'lineNodes' field." << std::endl;
		return false;
	}
	nlohmann::json lineNodes = data[DATA_LINE_NODES_KEY];

	if (!data.contains(DATA_EFFECT_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'effectNodes' field." << std::endl;
		return false;
	}
	nlohmann::json effectNodes = data[DATA_EFFECT_NODES_KEY];

	if (!data.contains(DATA_ROADBLOCK_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'roadBlockNodes' field." << std::endl;
		return false;
	}
	nlohmann::json roadBlockNodes = data[DATA_ROADBLOCK_NODES_KEY];

	if (!data.contains(DATA_TRAP_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'trapNodes' field." << std::endl;
		return false;
	}
	nlohmann::json trapNodes = data[DATA_TRAP_NODES_KEY];

	float bpm = j.value(BEATMAP_BPM_KEY, 120.0f);
	setRawExportBPM(bpm);
	setBpmExportMode(BpmExportMode::Single);
	double offset = j.value(BEATMAP_OFFSET_KEY, 0.0);
	offset = -offset; //inverting offset to match internal representation

	//Loading Timing
	setBPM(bpm);
	setOffset(offset);

	/*
	std::cout << "sphereNodes count: " << sphereNodes.size() << std::endl;
	std::cout << "lineNodes count: " << lineNodes.size() << std::endl;
	std::cout << "roadBlockNodes count: " << roadBlockNodes.size() << std::endl;
	std::cout << "trapNodes count: " << trapNodes.size() << std::endl;
	*/

	//load sphereNodes(hand & single foot notes)
	for (auto& sn : sphereNodes) {
		int order = sn.value(NOTE_ORDER_KEY, 0);
		int x = sn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
		int y = sn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
		int type = sn.value(NOTE_TYPE_KEY, 0);
		addNote(Note(orderToSec(order, getFirstSecPerBeat()), x, y, Note::intToNoteType(type)));
		//std::cout << "Loaded sphere note - timeSec: " << static_cast<double>(order * (timingList.getTiming(static_cast<size_t>(0)).secPerBeat()) / 24) << ", x: " << x << ", y: " << y << ", type: " << type << std::endl;
	}

	//load lineNodes (Long foot notes)
	if (!lineNodes.empty())
	{
		size_t size = lineNodes.size();
		std::vector<bool> isLoaded(size, false);	//vector for record loaded nodes
		int i = 0;
		for (auto& ln : lineNodes) {
			if (isLoaded.at(i)) {
				i++;
				continue;
			}
			GroupNotes groupNotes(GroupType::LONG_NOTE);
			int groupID = ln.value(NOTE_LINE_GROUP_ID_KEY, 0);
			int index = ln.value(NOTE_INDEX_IN_LINE_KEY, 0);
			int order = ln.value(NOTE_ORDER_KEY, 0);
			int x = ln[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = ln[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			int type = ln.value(NOTE_TYPE_KEY, 0);
			groupNotes.addRawNote(Note(orderToSec(order, getFirstSecPerBeat()), x, y, Note::intToNoteType(type), index));
			isLoaded.at(i++) = true;

			int j = 0;
			//load nodes which has same groupID
			for (auto& ln : lineNodes) {
				if (isLoaded.at(j)) {
					j++;
					continue;
				}
				int id = ln.value(NOTE_LINE_GROUP_ID_KEY, 0);
				if (id == groupID) {
					int index = ln.value(NOTE_INDEX_IN_LINE_KEY, 0);
					int order = ln.value(NOTE_ORDER_KEY, 0);
					int x = ln[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
					int y = ln[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
					int type = ln.value(NOTE_TYPE_KEY, 0);
					groupNotes.addRawNote(Note(orderToSec(order, getFirstSecPerBeat()), x, y, Note::intToNoteType(type), index));
					isLoaded.at(j++) = true;
				}
				else {
					j++;
				}

			}
			groupNotes.organizeNotesIndex();
			addGroupNotes(groupNotes);
			//std::cout << "Loaded Long Note Group, id: " << groupID << " to " << groupNotes.getGroupID() << std::endl;
		}
	}

	//load roadBlockNodes (obstacles) 
	for (auto& bn : roadBlockNodes) {
		int type = bn.value(NOTE_TYPE_KEY, 0);
		if (Note::intToNoteType(type) == NoteType::BAR) {	// if it's bar 
			int order = bn.value(NOTE_ORDER_KEY, 0);
			int x = bn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = bn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			addNote(Note(orderToSec(order, getFirstSecPerBeat()), x, y, Note::intToNoteType(type)));
			//std::cout << "Loaded bar - timeSec: " << static_cast<double>(order * (timingList.getTiming(static_cast<size_t>(0)).secPerBeat()) / 24) << ", x: " << x << ", y: " << y << ", type: " << type << std::endl;
			continue;
		}
		else if (Note::intToNoteType(type) == NoteType::OBS) { // if it's obstacles
			GroupNotes groupNotes(GroupType::OBSTACLE);
			int startOrder = bn.value(NOTE_ORDER_KEY, 0);
			int endOrder = bn.value(NOTE_END_POSITION_SELECT_ORDER_KEY, 0);
			int x = bn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = bn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			double timeSec = orderToSec(startOrder, getFirstSecPerBeat());
			double endTimeSec = orderToSec(endOrder, getFirstSecPerBeat());
			double duration = endTimeSec - timeSec;
			groupNotes.addNote(Note(timeSec, x, y, Note::intToNoteType(type), duration, 0));
			groupNotes.addNote(Note(endTimeSec, x, y, NoteType::OBS_END, duration, 1));
			addGroupNotes(groupNotes);
			//std::cout << "Loaded obstacle Group - startTimeSec: " << timeSec << ", endTimeSec: " << endTimeSec << ", x: " << x << ", y: " << y << ", type: " << type << std::endl;
		}
	}

	//load trapNodes(trap)
	for (auto& tn : trapNodes) {
		int order = tn.value(NOTE_ORDER_KEY, 0);
		int x = tn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
		int y = tn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
		int type = tn.value(NOTE_TYPE_KEY, 0);
		addNote(Note(orderToSec(order, getFirstSecPerBeat()), x, y, Note::intToNoteType(type)));
		//std::cout << "Loaded trap - timeSec: " << static_cast<double>(order * (timingList.getTiming(static_cast<size_t>(0)).secPerBeat()) / 24) << ", x: " << x << ", y: " << y << ", type: " << type << std::endl;
	}

	loaded = true;

	return true;
}

bool Beatmap::loadCustomFormat(const nlohmann::json& j) {
	if (!j.contains(BEATMAP_DATA_KEY))
	{
		std::cerr << "Invalid beatmap format: missing 'data' field." << std::endl;
		return false;
	}
	nlohmann::json data = j[BEATMAP_DATA_KEY];

	if (!data.contains(DATA_SPHERE_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'sphereNodes' field." << std::endl;
		return false;
	}
	nlohmann::json sphereNodes = data[DATA_SPHERE_NODES_KEY];

	if (!data.contains(DATA_LINE_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'lineNodes' field." << std::endl;
		return false;
	}
	nlohmann::json lineNodes = data[DATA_LINE_NODES_KEY];

	if (!data.contains(DATA_EFFECT_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'effectNodes' field." << std::endl;
		return false;
	}
	nlohmann::json effectNodes = data[DATA_EFFECT_NODES_KEY];

	if (!data.contains(DATA_ROADBLOCK_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'roadBlockNodes' field." << std::endl;
		return false;
	}
	nlohmann::json roadBlockNodes = data[DATA_ROADBLOCK_NODES_KEY];

	if (!data.contains(DATA_TRAP_NODES_KEY)) {
		std::cerr << "Invalid beatmap format: missing 'trapNodes' field." << std::endl;
		return false;
	}
	nlohmann::json trapNodes = data[DATA_TRAP_NODES_KEY];

	nlohmann::json streamGroups = data.contains(DATA_STREAM_GROUPS_KEY)
		? data[DATA_STREAM_GROUPS_KEY]
		: nlohmann::json::array();
	nlohmann::json streamTemplates = data.contains(DATA_STREAM_TEMPLATES_KEY)
		? data[DATA_STREAM_TEMPLATES_KEY]
		: nlohmann::json::array();

	if (!j.contains(BEATMAP_TIMINGLIST_KEY))
	{
		std::cerr << "Invalid beatmap format: missing 'timingList' field." << std::endl;
		return false;
	}
	nlohmann::json timingList = j[BEATMAP_TIMINGLIST_KEY];

	double songDuration = j.value(BEATMAP_DURATION_KEY, 0.0);
	setSongDuration(songDuration);

	auto isGeneratedStreamNoteJson = [this](const nlohmann::json& noteJson) {
		return noteJson.value(NOTE_IS_GENERATED_FROM_STREAM_GROUP_KEY, false);
	};

	auto segmentModeFromString = [](const std::string& modeString) {
		if (modeString == "Linear") {
			return SegmentMode::Linear;
		}
		return SegmentMode::CatmullRom;
	};

	auto streamTemplateGroupTypeFromString = [](const std::string& groupTypeString) {
		if (groupTypeString == "Obstacle") {
			return GroupType::OBSTACLE;
		}
		return GroupType::DEFAULT;
	};

	auto hasStreamFloatPosition = [](const nlohmann::json& noteJson) {
		return noteJson.contains(NOTE_FLOAT_X_KEY) && noteJson.contains(NOTE_FLOAT_Y_KEY);
	};

	//Loading Timing
	float bpm = j.value(BEATMAP_BPM_KEY, DEFAULT_EXPORT_BPM);
	setRawExportBPM(bpm);
	bool hasExplicitBpmExportMode =
		j.contains(BPM_EXPORT_MODE_KEY) &&
		!j[BPM_EXPORT_MODE_KEY].is_null() &&
		j[BPM_EXPORT_MODE_KEY].is_string();
	if (hasExplicitBpmExportMode) {
		setBpmExportMode(bpmExportModeFromString(j.value(BPM_EXPORT_MODE_KEY, std::string("Single"))));
	}

	{
		bool firstTiming = true;
		for (auto& t : timingList) {
			float t_bpm = t.value(TIMING_BPM_KEY, 120.0f);
			double t_offset = t.value(TIMING_OFFSET_KEY, 0.0);
			if (firstTiming) {			//when load first timing, set BPM and offset
				firstTiming = false;
				setBPM(t_bpm);
				setOffset(t_offset);
				continue;
			}
			addTiming(t_bpm, t_offset);	//when load sencond and later timing, add new timing
		}
	}
	if (!hasExplicitBpmExportMode) {
		const bool topLevelBpmDiffersFromFirstTiming =
			getTimingListSize() > 0 &&
			std::fabs(bpm - getFirstBPM()) > 0.0001f;
		setBpmExportMode(
			getTimingListSize() > 1 || topLevelBpmDiffersFromFirstTiming
			? BpmExportMode::Multiple
			: BpmExportMode::Single
		);
	}

	//load sphereNodes(hand & single foot notes)
	for (auto& sn : sphereNodes) {
		if (isGeneratedStreamNoteJson(sn)) {
			continue;
		}
		double timeSec = sn.value(NOTE_TIME_SEC_KEY, 0.0);
		int x = sn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
		int y = sn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
		int type = sn.value(NOTE_TYPE_KEY, 0);
		addNote(Note(timeSec, x, y, Note::intToNoteType(type)));
	}

	//load lineNodes (Long foot notes)
	if (!lineNodes.empty())
	{
		size_t size = lineNodes.size();
		std::vector<bool> isLoaded(size, false);	//vector for record loaded nodes
		int i = 0;
		for (auto& ln : lineNodes) {
			if (isLoaded.at(i) || isGeneratedStreamNoteJson(ln)) {
				isLoaded.at(i++) = true;
				continue;
			}
			GroupNotes groupNotes(GroupType::LONG_NOTE);
			int groupID = ln.value(NOTE_LINE_GROUP_ID_KEY, 0);
			int index = ln.value(NOTE_INDEX_IN_LINE_KEY, 0);
			double timeSec = ln.value(NOTE_TIME_SEC_KEY, 0.0);
			int x = ln[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = ln[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			int type = ln.value(NOTE_TYPE_KEY, 0);
			groupNotes.addRawNote(Note(timeSec, x, y, Note::intToNoteType(type), index));
			isLoaded.at(i++) = true;

			int j = 0;
			//load nodes which has same groupID
			for (auto& groupedLn : lineNodes) {
				if (isLoaded.at(j) || isGeneratedStreamNoteJson(groupedLn)) {
					isLoaded.at(j++) = true;
					continue;
				}
				int id = groupedLn.value(NOTE_LINE_GROUP_ID_KEY, 0);
				if (id == groupID) {
					int groupedIndex = groupedLn.value(NOTE_INDEX_IN_LINE_KEY, 0);
					double groupedTimeSec = groupedLn.value(NOTE_TIME_SEC_KEY, 0.0);
					int groupedX = groupedLn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
					int groupedY = groupedLn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
					int groupedType = groupedLn.value(NOTE_TYPE_KEY, 0);
					groupNotes.addRawNote(Note(groupedTimeSec, groupedX, groupedY, Note::intToNoteType(groupedType), groupedIndex));
					isLoaded.at(j++) = true;
				}
				else {
					j++;
				}
			}
			groupNotes.organizeNotesIndex();
			addGroupNotes(groupNotes);
		}
	}

	//load roadBlockNodes (obstacles) 
	for (auto& bn : roadBlockNodes) {
		if (isGeneratedStreamNoteJson(bn)) {
			continue;
		}
		int type = bn.value(NOTE_TYPE_KEY, 0);
		if (Note::intToNoteType(type) == NoteType::BAR) {	// if it's bar 
			double timeSec = bn.value(NOTE_TIME_SEC_KEY, 0.0);
			int x = bn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = bn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			addNote(Note(timeSec, x, y, Note::intToNoteType(type)));
			continue;
		}
		else if (Note::intToNoteType(type) == NoteType::OBS) { // if it's obstacles
			GroupNotes groupNotes(GroupType::OBSTACLE);
			double startTimeSec = bn.value(NOTE_START_TIME_SEC_KEY, 0.0);
			double endTimeSec = bn.value(NOTE_END_TIME_SEC_KEY, 0.0);
			int x = bn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
			int y = bn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
			double duration = endTimeSec - startTimeSec;
			groupNotes.addNote(Note(startTimeSec, x, y, NoteType::OBS, duration, 0));
			groupNotes.addNote(Note(endTimeSec, x, y, NoteType::OBS_END, duration, 1));
			addGroupNotes(groupNotes);
		}
	}

	//load trapNodes(trap)
	for (auto& tn : trapNodes) {
		if (isGeneratedStreamNoteJson(tn)) {
			continue;
		}
		double timeSec = tn.value(NOTE_TIME_SEC_KEY, 0.0);
		int x = tn[NOTE_POSITION_KEY].value(NOTE_X_KEY, 0);
		int y = tn[NOTE_POSITION_KEY].value(NOTE_Y_KEY, 0);
		int type = tn.value(NOTE_TYPE_KEY, 0);
		addNote(Note(timeSec, x, y, Note::intToNoteType(type)));
	}

	std::unordered_map<uint64_t, uint64_t> templateGroupIDMap{};
	for (auto& templateJson : streamTemplates) {
		if (!templateJson.contains(STREAM_TEMPLATE_GROUP_TYPE_KEY)) {
			std::cerr << "Invalid stream template: missing groupType field." << std::endl;
			return false;
		}
		uint64_t fileTemplateGroupID = templateJson.value(STREAM_TEMPLATE_GROUP_ID_FILE_KEY, static_cast<uint64_t>(0));
		const nlohmann::json templateNotes = templateJson.contains(STREAM_TEMPLATE_NOTES_KEY)
			? templateJson[STREAM_TEMPLATE_NOTES_KEY]
			: nlohmann::json::array();
		GroupType templateGroupType = streamTemplateGroupTypeFromString(
			templateJson.value(STREAM_TEMPLATE_GROUP_TYPE_KEY, std::string("Default")));
		GroupNotes templateGroup(templateGroupType);
		for (auto& templateNoteJson : templateNotes) {
			if (!hasStreamFloatPosition(templateNoteJson)) {
				std::cerr << "Invalid stream template note: missing floatX/floatY field." << std::endl;
				return false;
			}
			double timeSec = templateNoteJson.value(NOTE_TIME_SEC_KEY, 0.0);
			float floatX = templateNoteJson.value(NOTE_FLOAT_X_KEY, 0.0f);
			float floatY = templateNoteJson.value(NOTE_FLOAT_Y_KEY, 0.0f);
			int type = templateNoteJson.value(NOTE_TYPE_KEY, 0);
			double duration = templateNoteJson.value(NOTE_DURATION_KEY, 0.0);
			const NoteType noteType = Note::intToNoteType(type);
			Note templateNote = (noteType == NoteType::OBS || noteType == NoteType::OBS_END)
				? Note(timeSec, static_cast<int>(std::lround(floatX)), static_cast<int>(std::lround(floatY)), noteType, duration)
				: Note(timeSec, static_cast<int>(std::lround(floatX)), static_cast<int>(std::lround(floatY)), noteType);
			templateNote.setFloatPosition(floatX, floatY);
			templateGroup.addNote(templateNote);
		}
		GroupNotes& addedTemplate = addStreamGroupNotes(templateGroup);
		templateGroupIDMap[fileTemplateGroupID] = addedTemplate.getGroupID();
	}

	for (auto& streamGroupJson : streamGroups) {
		if (!streamGroupJson.contains(STREAM_TEMPLATE_GROUP_IDS_KEY)) {
			std::cerr << "Skipping stream group: missing streamTemplateGroupIds field." << std::endl;
			continue;
		}
		std::vector<uint64_t> streamTemplateGroupIDs{};
		bool hasInvalidTemplateGroupID = false;
		for (const auto& fileTemplateGroupIDJson : streamGroupJson[STREAM_TEMPLATE_GROUP_IDS_KEY]) {
			const uint64_t fileTemplateGroupID = fileTemplateGroupIDJson.get<uint64_t>();
			auto templateGroupIDIt = templateGroupIDMap.find(fileTemplateGroupID);
			if (templateGroupIDIt == templateGroupIDMap.end()) {
				std::cerr << "Skipping stream group: missing matching streamTemplate groupId " << fileTemplateGroupID << std::endl;
				hasInvalidTemplateGroupID = true;
				break;
			}
			streamTemplateGroupIDs.push_back(templateGroupIDIt->second);
		}
		if (hasInvalidTemplateGroupID || streamTemplateGroupIDs.empty()) {
			std::cerr << "Skipping stream group: no valid streamTemplateGroupIds." << std::endl;
			continue;
		}

		GroupNotes streamGroup(streamTemplateGroupIDs, streamGroupJson.value(STREAM_REPEAT_INTERVAL_SEC_KEY, 1.0));
		const nlohmann::json nodesJson = streamGroupJson.contains(STREAM_NODES_KEY)
			? streamGroupJson[STREAM_NODES_KEY]
			: nlohmann::json::array();
		bool hasInvalidNode = false;
		for (auto& nodeJson : nodesJson) {
			if (!hasStreamFloatPosition(nodeJson)) {
				std::cerr << "Skipping stream group: invalid stream node missing floatX/floatY field." << std::endl;
				hasInvalidNode = true;
				break;
			}
			double timeSec = nodeJson.value(NOTE_TIME_SEC_KEY, 0.0);
			float floatX = nodeJson.value(NOTE_FLOAT_X_KEY, 0.0f);
			float floatY = nodeJson.value(NOTE_FLOAT_Y_KEY, 0.0f);
			std::string segmentModeString = nodeJson.value(NOTE_SEGMENT_MODE_KEY, "CatmullRom");
			Note node(timeSec, static_cast<int>(std::lround(floatX)), static_cast<int>(std::lround(floatY)), NoteType::STREAM_NODE);
			node.setFloatPosition(floatX, floatY);
			node.setSegmentMode(segmentModeFromString(segmentModeString));
			streamGroup.addNote(node);
		}
		if (hasInvalidNode) {
			continue;
		}
		addGroupNotes(streamGroup);
	}

	rebuildGeneratedStreamGroupNotes();
	loaded = true;

	return true;
}

bool Beatmap::exportJson(const std::string& path, bool isForcedExport) const {
	if (std::filesystem::exists(path) && !isForcedExport) {
		std::cerr << "file already exists!" << std::endl;
		return false;
	}

	float BPM, offset, secPerBeat;
	if (bpmExportMode == BpmExportMode::Multiple) {
		BPM = exportBPM;
		secPerBeat = 60.0 / static_cast<double>(BPM);
	}
	else {
		BPM = getFirstBPM();
		secPerBeat = getFirstSecPerBeat();
	}
	offset = -getFirstOffset();			//invert offset for export

	auto makeBaseNoteJson = [this, secPerBeat](const Note& note, bool isGenerated) {
		nlohmann::json noteJson;
		int noteOrder = secToOrder(note.getTime(), secPerBeat);
		noteJson[NOTE_ORDER_KEY] = noteOrder;
		noteJson[NOTE_TIME_KEY] = calcTimeNormalized(noteOrder, secPerBeat);
		noteJson[NOTE_TIME_SEC_KEY] = note.getTime();
		noteJson[NOTE_POSITION_KEY][NOTE_X_KEY] = note.getX();
		noteJson[NOTE_POSITION_KEY][NOTE_Y_KEY] = note.getY();
		noteJson[NOTE_POSITION_2D_KEY][NOTE_X_KEY] = calcPosition2DX(note.getX());
		noteJson[NOTE_POSITION_2D_KEY][NOTE_Y_KEY] = calcPosition2DY(note.getY());
		noteJson[NOTE_SIZE_KEY][NOTE_X_KEY] = 1.0;
		noteJson[NOTE_SIZE_KEY][NOTE_Y_KEY] = 1.0;
		noteJson[NOTE_SIZE_KEY][NOTE_Z_KEY] = 1.0;
		noteJson[NOTE_TYPE_KEY] = Note::noteTypeToInt(note.getType());
		noteJson[NOTE_POSITION_OFFSET_KEY];
		noteJson[NOTE_IS_PLAY_AUDIO_KEY] = false;
		noteJson[NOTE_IS_GENERATED_FROM_STREAM_GROUP_KEY] = isGenerated;
		return noteJson;
	};

	auto appendFlatNoteJson = [this, &makeBaseNoteJson, secPerBeat](
		nlohmann::json& sphereNodes,
		nlohmann::json& roadBlockNodes,
		nlohmann::json& trapNodes,
		const Note& note,
		bool isGenerated) {
		nlohmann::json noteJson = makeBaseNoteJson(note, isGenerated);
		switch (note.getType()) {
		case NoteType::LEFT_HAND:
		case NoteType::RIGHT_HAND:
		case NoteType::LEFT_FOOT:
		case NoteType::RIGHT_FOOT:
			sphereNodes.push_back(noteJson);
			break;
		case NoteType::BAR:
			noteJson[NOTE_LENGTH_KEY] = 1;
			noteJson[NOTE_DURATION_KEY] = calcTimeNormalized(1, secPerBeat);
			noteJson[NOTE_START_POSITION_SELECT_ORDER_KEY] = -1;
			noteJson[NOTE_END_POSITION_SELECT_ORDER_KEY] = -1;
			noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_X_KEY] = 0.0;
			noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Y_KEY] = 0.0;
			noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Z_KEY] = 0.0;
			roadBlockNodes.push_back(noteJson);
			break;
		case NoteType::TRAP:
			trapNodes.push_back(noteJson);
			break;
		default:
			assert(false && "[Beatmap::exportJson] unexpected note type value");
			break;
		}
	};

	auto segmentModeToString = [](SegmentMode mode) {
		switch (mode) {
		case SegmentMode::Linear:
			return std::string("Linear");
		case SegmentMode::CatmullRom:
		default:
			return std::string("CatmullRom");
		}
	};

	auto streamTemplateGroupTypeToString = [](GroupType groupType) {
		switch (groupType) {
		case GroupType::OBSTACLE:
			return std::string("Obstacle");
		case GroupType::DEFAULT:
		default:
			return std::string("Default");
		}
	};

	nlohmann::json j, data, sphereNodes, lineNodes, effectNodes, roadBlockNodes, trapNodes, timingList, streamGroups, streamTemplates;

	//notes data
	sphereNodes = nlohmann::json::array();
	lineNodes = nlohmann::json::array();
	effectNodes = nlohmann::json::array();
	roadBlockNodes = nlohmann::json::array();
	trapNodes = nlohmann::json::array();
	timingList = nlohmann::json::array();
	streamGroups = nlohmann::json::array();
	streamTemplates = nlohmann::json::array();

	for (const Note& n : notes) {
		appendFlatNoteJson(sphereNodes, roadBlockNodes, trapNodes, n, false);
	}

	for (const GroupNotes& group : vecGroupNotes) {
		GroupType gtype = group.getGroupType();
		switch (gtype) {
		case GroupType::LONG_NOTE:
			for (size_t i = 0; i < group.size(); i++) {
				const Note& n = group.getNoteAtIndex(i);
				nlohmann::json noteJson = makeBaseNoteJson(n, false);
				noteJson[NOTE_LINE_GROUP_ID_KEY] = n.getGroupID();
				noteJson[NOTE_INDEX_IN_LINE_KEY] = n.getIndex();
				lineNodes.push_back(noteJson);
			}
			break;
		case GroupType::OBSTACLE:
			if (group.size() != 2) {
				assert(false && "[Beatmap::exportJson] invalid size of GroupNotes Obstacles");
				break;
			}
			{
				const Note& startNote = group.getNoteAtIndex(0);
				const Note& endNote = group.getNoteAtIndex(1);
				int startNoteOrder = secToOrder(startNote.getTime(), secPerBeat);
				int endNoteOrder = secToOrder(endNote.getTime(), secPerBeat);
				for (size_t k = 0; k < group.size(); k++) {
					const Note& n = group.getNoteAtIndex(k);
					nlohmann::json noteJson = makeBaseNoteJson(n, false);
					int order = secToOrder(n.getTime(), secPerBeat);
					noteJson[NOTE_ORDER_KEY] = order;
					noteJson[NOTE_TIME_KEY] = calcTimeNormalized(order, secPerBeat);
					noteJson[NOTE_START_TIME_SEC_KEY] = startNote.getTime();
					noteJson[NOTE_END_TIME_SEC_KEY] = endNote.getTime();
					noteJson[NOTE_LENGTH_KEY] = endNoteOrder - startNoteOrder;
					noteJson[NOTE_DURATION_KEY] = calcTimeNormalized((endNoteOrder - startNoteOrder), secPerBeat);
					noteJson[NOTE_START_POSITION_SELECT_ORDER_KEY] = startNoteOrder;
					noteJson[NOTE_END_POSITION_SELECT_ORDER_KEY] = endNoteOrder;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_X_KEY] = 0.0;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Y_KEY] = 0.0;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Z_KEY] = 0.0;
					roadBlockNodes.push_back(noteJson);
				}
			}
			break;
		case GroupType::DEFAULT:
			for (size_t i = 0; i < group.size(); ++i) {
				appendFlatNoteJson(sphereNodes, roadBlockNodes, trapNodes, group.getNoteAtIndex(i), false);
			}
			break;
		case GroupType::STREAM:
		{
			nlohmann::json streamGroupJson;
			streamGroupJson[STREAM_TEMPLATE_GROUP_IDS_KEY] = nlohmann::json::array();
			for (uint64_t streamTemplateGroupID : group.getStreamGroupNotesIDs()) {
				streamGroupJson[STREAM_TEMPLATE_GROUP_IDS_KEY].push_back(streamTemplateGroupID);
			}
			streamGroupJson[STREAM_REPEAT_INTERVAL_SEC_KEY] = group.getStreamRepeatIntervalSec();
			streamGroupJson[STREAM_NODES_KEY] = nlohmann::json::array();
			for (size_t i = 0; i < group.size(); ++i) {
				const Note& node = group.getNoteAtIndex(i);
				nlohmann::json nodeJson;
				nodeJson[NOTE_TIME_SEC_KEY] = node.getTime();
				nodeJson[NOTE_FLOAT_X_KEY] = node.getFloatX();
				nodeJson[NOTE_FLOAT_Y_KEY] = node.getFloatY();
				nodeJson[NOTE_SEGMENT_MODE_KEY] = segmentModeToString(node.getSegmentMode());
				streamGroupJson[STREAM_NODES_KEY].push_back(nodeJson);
			}
			streamGroups.push_back(streamGroupJson);
			break;
		}
		default:
			assert(false && "[Beatmap::exportJson] unexpected group type value");
			break;
		}
	}

	for (const GroupNotes& templateGroup : vecStreamTemplates) {
		nlohmann::json templateJson;
		templateJson[STREAM_TEMPLATE_GROUP_ID_FILE_KEY] = templateGroup.getGroupID();
		templateJson[STREAM_TEMPLATE_GROUP_TYPE_KEY] = streamTemplateGroupTypeToString(templateGroup.getGroupType());
		templateJson[STREAM_TEMPLATE_NOTES_KEY] = nlohmann::json::array();
		for (size_t i = 0; i < templateGroup.size(); ++i) {
			const Note& templateNote = templateGroup.getNoteAtIndex(i);
			nlohmann::json templateNoteJson;
			templateNoteJson[NOTE_TIME_SEC_KEY] = templateNote.getTime();
			templateNoteJson[NOTE_TYPE_KEY] = Note::noteTypeToInt(templateNote.getType());
			templateNoteJson[NOTE_FLOAT_X_KEY] = templateNote.getFloatX();
			templateNoteJson[NOTE_FLOAT_Y_KEY] = templateNote.getFloatY();
			if (templateNote.getType() == NoteType::OBS || templateNote.getType() == NoteType::OBS_END) {
				templateNoteJson[NOTE_DURATION_KEY] = templateNote.getDuration();
			}
			templateJson[STREAM_TEMPLATE_NOTES_KEY].push_back(templateNoteJson);
		}
		streamTemplates.push_back(templateJson);
	}

	for (const GroupNotes& generatedGroup : vecGeneratedStreamGroupNotes) {
		switch (generatedGroup.getGroupType()) {
		case GroupType::DEFAULT:
			for (size_t i = 0; i < generatedGroup.size(); ++i) {
				appendFlatNoteJson(sphereNodes, roadBlockNodes, trapNodes, generatedGroup.getNoteAtIndex(i), true);
			}
			break;
		case GroupType::OBSTACLE:
			if (generatedGroup.size() != 2) {
				assert(false && "[Beatmap::exportJson] invalid generated obstacle stream template size");
				break;
			}
			{
				const Note& startNote = generatedGroup.getNoteAtIndex(0);
				const Note& endNote = generatedGroup.getNoteAtIndex(1);
				int startNoteOrder = secToOrder(startNote.getTime(), secPerBeat);
				int endNoteOrder = secToOrder(endNote.getTime(), secPerBeat);
				for (size_t i = 0; i < generatedGroup.size(); ++i) {
					const Note& n = generatedGroup.getNoteAtIndex(i);
					nlohmann::json noteJson = makeBaseNoteJson(n, true);
					int order = secToOrder(n.getTime(), secPerBeat);
					noteJson[NOTE_ORDER_KEY] = order;
					noteJson[NOTE_TIME_KEY] = calcTimeNormalized(order, secPerBeat);
					noteJson[NOTE_START_TIME_SEC_KEY] = startNote.getTime();
					noteJson[NOTE_END_TIME_SEC_KEY] = endNote.getTime();
					noteJson[NOTE_LENGTH_KEY] = endNoteOrder - startNoteOrder;
					noteJson[NOTE_DURATION_KEY] = calcTimeNormalized((endNoteOrder - startNoteOrder), secPerBeat);
					noteJson[NOTE_START_POSITION_SELECT_ORDER_KEY] = startNoteOrder;
					noteJson[NOTE_END_POSITION_SELECT_ORDER_KEY] = endNoteOrder;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_X_KEY] = 0.0;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Y_KEY] = 0.0;
					noteJson[NOTE_POSITION_OFFSET_KEY][NOTE_Z_KEY] = 0.0;
					roadBlockNodes.push_back(noteJson);
				}
			}
			break;
		default:
			assert(false && "[Beatmap::exportJson] unexpected generated stream group type");
			break;
		}
	}

	//export timing list data(unique in the DDEditor)
	for (size_t i = 0; i < getTimingListSize(); i++) {
		nlohmann::json timingJson;
		const TimingList& tl = getTimingList();
		const Timing& t = tl.getTiming(i);
		timingJson[TIMING_BPM_KEY] = t.getBPM();
		timingJson[TIMING_OFFSET_KEY] = t.getOffset();
		timingList.push_back(timingJson);
	}

	data[DATA_NAME_KEY] = "default";
	data[DATA_INTERVAL_PER_SECOND_KEY] = BPM / 10;
	data[DATA_GRID_SIZE_KEY][NOTE_X_KEY] = 0;
	data[DATA_GRID_SIZE_KEY][NOTE_Y_KEY] = 0;
	data[DATA_PLANE_SIZE_KEY][NOTE_X_KEY] = 0.0;
	data[DATA_PLANE_SIZE_KEY][NOTE_Y_KEY] = 0.0;
	data[DATA_ORDER_COUNT_PER_BEAT_KEY] = ORDER_PER_BEAT;
	data[DATA_SPHERE_NODES_KEY] = sphereNodes;
	data[DATA_LINE_NODES_KEY] = lineNodes;
	data[DATA_EFFECT_NODES_KEY] = effectNodes;
	data[DATA_ROADBLOCK_NODES_KEY] = roadBlockNodes;
	data[DATA_TRAP_NODES_KEY] = trapNodes;
	data[DATA_STREAM_GROUPS_KEY] = streamGroups;
	data[DATA_STREAM_TEMPLATES_KEY] = streamTemplates;
	j[BEATMAP_DATA_KEY] = data;
	j[BEATMAP_TIMINGLIST_KEY] = timingList;
	j[BEATMAP_BEATSUBS_KEY] = 0;
	j[BEATMAP_BPM_KEY] = BPM;
	j[BEATMAP_OFFSET_KEY] = offset;
	j[BEATMAP_DURATION_KEY] = getSongDuration();
	j[BEATMAP_NPS_KEY] = "0.0";
	j[BEATMAP_DEVELOPER_MODE_KEY] = false;
	j[BEATMAP_NOTE_SPEED_KEY] = 1.0;
	j[BEATMAP_NOTE_JUMP_OFFSET_KEY] = 0.0;
	j[BEATMAP_INTERVAL_KEY] = 1.0;
	j[BEATMAP_INFO_KEY] = "";
	j[BEATMAP_FORMAT_VERSION_KEY] = "1.0.0";
	j[BPM_EXPORT_MODE_KEY] = bpmExportModeToString(bpmExportMode);

	std::ofstream ofs(path);
	ofs << j.dump(2);   // indent count

	return true;
}

bool Beatmap::isLoaded() const {
	return loaded;
}

void Beatmap::addNote(const Note& note) {
	notes.push_back(note);
	sortNotesByTime();
	return;
}

void Beatmap::removeNoteAtIndex(size_t index) {
	if (index >= notes.size())  throw std::out_of_range("Beatmap::removeNoteAtIndex: index out of range");
	notes.erase(notes.begin() + index);
	return;
}

bool Beatmap::removeNoteByNoteID(uint64_t noteID) {
	auto it = std::find_if(notes.begin(), notes.end(),
		[noteID](const Note& n) { return n.getNoteID() == noteID; });

	if (it != notes.end()) {
		notes.erase(it);
		return false;
	}

	for (size_t g = 0; g < vecGroupNotes.size(); g++) {
		GroupNotes& gn = vecGroupNotes.at(g);
		for (size_t i = 0; i < gn.size(); i++) {
			Note& n = gn.getNoteAtIndex(i);
			if (n.getNoteID() != noteID) {
				continue;
			}

			const NoteType type = n.getType();
			const uint64_t groupID = n.getGroupID();
			const int index = n.getIndex();

			switch (type) {
			case NoteType::LONG_LEFT_FOOT:
			case NoteType::LONG_RIGHT_FOOT:
				if (removeLongNotesNode(groupID, index)) {
					return true;
				} else {
					return false;
				}
			case NoteType::OBS:
			case NoteType::OBS_END:
				removeGroupNotesWithGroupID(groupID);
				return false;
			case NoteType::STREAM_NODE:
			{
				const std::vector<uint64_t> streamGroupNoteIDs = gn.getStreamGroupNotesIDs();
				if (gn.removeNote(i)) {
					vecGroupNotes.erase(vecGroupNotes.begin() + g);
					for (uint64_t streamGroupNoteID : streamGroupNoteIDs) {
						removeStreamGroupNotesWithGroupID(streamGroupNoteID);
					}
					rebuildGeneratedStreamGroupNotes();
					return true;
				}
				rebuildGeneratedStreamGroupNotes();
				return false;
			}
			default:
				if (gn.removeNote(i)) {
					vecGroupNotes.erase(vecGroupNotes.begin() + g);
					return true;
				}
				return false;
			}
		}
	}
	throw std::out_of_range("Beatmap::removeNoteByNoteID: noteID not found");
}

GroupNotes& Beatmap::addGroupNotes(GroupNotes& _groupNotes) {
	uint64_t groupID = _groupNotes.getGroupID();
	const bool isStreamGroup = _groupNotes.getGroupType() == GroupType::STREAM;
	vecGroupNotes.push_back(_groupNotes);
	sortGroupNotesByTime();
	if (isStreamGroup) {
		rebuildGeneratedStreamGroupNotes();
	}
	GroupNotes& gn = getGroupNotesWithGroupID(groupID);
	return gn;
}

GroupNotes& Beatmap::addStreamGroupNotes(GroupNotes& _groupNotes) {
	uint64_t groupID = _groupNotes.getGroupID();
	vecStreamTemplates.push_back(_groupNotes);
	auto it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });
	if (it != vecStreamTemplates.end()) {
		return *it;
	}
	throw std::out_of_range("Beatmap::addStreamGroupNotes: groupID not found");
}

GroupNotes& Beatmap::addStreamInstance(const std::vector<uint64_t>& streamGroupNotesIDs, double repeatIntervalSec, const Note& firstNode) {
	GroupNotes streamGroup(streamGroupNotesIDs, repeatIntervalSec);
	streamGroup.addNote(firstNode);
	return addGroupNotes(streamGroup);
}

void Beatmap::addStreamNode(uint64_t streamGroupID, const Note& node, double repeatIntervalSec) {
	GroupNotes& streamGroup = getGroupNotesWithGroupID(streamGroupID);
	if (streamGroup.getGroupType() != GroupType::STREAM) {
		throw std::logic_error("Beatmap::addStreamNode: target group is not stream");
	}

	streamGroup.addNote(node);
	streamGroup.setStreamRepeatIntervalSec(repeatIntervalSec);
	rebuildGeneratedStreamGroupNotes();
}

void Beatmap::changeStreamNodeByNoteID(uint64_t noteID, float dx, float dy, double dt) {
	for (GroupNotes& group : vecGroupNotes) {
		if (group.getGroupType() != GroupType::STREAM) {
			continue;
		}
		for (size_t i = 0; i < group.size(); ++i) {
			if (group.getNoteAtIndex(i).getNoteID() != noteID) {
				continue;
			}
			group.changeStreamNodeByNoteID(noteID, dx, dy, dt);
			rebuildGeneratedStreamGroupNotes();
			return;
		}
	}

	throw std::out_of_range("Beatmap::changeStreamNodeByNoteID: noteID not found");
}

void Beatmap::setStreamNodePositionByNoteID(uint64_t noteID, float x, float y) {
	for (GroupNotes& group : vecGroupNotes) {
		if (group.getGroupType() != GroupType::STREAM) {
			continue;
		}
		for (size_t i = 0; i < group.size(); ++i) {
			Note& note = group.getNoteAtIndex(i);
			if (note.getNoteID() != noteID) {
				continue;
			}
			note.setFloatPosition(x, y);
			rebuildGeneratedStreamGroupNotes();
			return;
		}
	}

	throw std::out_of_range("Beatmap::setStreamNodePositionByNoteID: noteID not found");
}

void Beatmap::setStreamRepeatIntervalSec(uint64_t streamGroupID, double intervalSec) {
	GroupNotes& streamGroup = getGroupNotesWithGroupID(streamGroupID);
	if (streamGroup.getGroupType() != GroupType::STREAM) {
		throw std::logic_error("Beatmap::setStreamRepeatIntervalSec: target group is not stream");
	}

	streamGroup.setStreamRepeatIntervalSec(intervalSec);
	rebuildGeneratedStreamGroupNotes();
}

void Beatmap::setStreamSegmentDivisions(int divisions) {
	const int clampedDivisions = (std::max)(divisions, 1);
	if (streamSegmentDivisions == clampedDivisions) {
		return;
	}

	streamSegmentDivisions = clampedDivisions;
	rebuildGeneratedStreamGroupNotes();
}

void Beatmap::addObstacleGroupNotes(double startTime, double duration, int x, int y) {
	GroupNotes groupNotes(GroupType::OBSTACLE);
	groupNotes.addNote(Note(startTime, x, y, NoteType::OBS, duration, 0));
	groupNotes.addNote(Note(startTime + duration, x, y, NoteType::OBS_END, duration, 1));
	addGroupNotes(groupNotes);
	return;
}

void Beatmap::removeGroupNotesAtIndex(size_t index) {
	if (index >= vecGroupNotes.size()) {
		throw std::out_of_range("Beatmap::removeGroupNotesAtIndex: index out of range");
		return;
	}
	const GroupNotes& group = vecGroupNotes.at(index);
	const bool isStreamGroup = group.getGroupType() == GroupType::STREAM;
	const std::vector<uint64_t> streamTemplateIDs = isStreamGroup ? group.getStreamGroupNotesIDs() : std::vector<uint64_t>{};
	vecGroupNotes.erase(vecGroupNotes.begin() + index);
	if (isStreamGroup) {
		for (uint64_t streamTemplateID : streamTemplateIDs) {
			removeStreamGroupNotesWithGroupID(streamTemplateID);
		}
		rebuildGeneratedStreamGroupNotes();
	}
	return;
}

void Beatmap::removeGroupNotesWithGroupID(uint64_t groupID) {
	auto it = std::find_if(vecGroupNotes.begin(), vecGroupNotes.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecGroupNotes.end()) {
		const bool isStreamGroup = it->getGroupType() == GroupType::STREAM;
		const std::vector<uint64_t> streamTemplateIDs = isStreamGroup ? it->getStreamGroupNotesIDs() : std::vector<uint64_t>{};
		vecGroupNotes.erase(it);
		if (isStreamGroup) {
			for (uint64_t streamTemplateID : streamTemplateIDs) {
				removeStreamGroupNotesWithGroupID(streamTemplateID);
			}
			rebuildGeneratedStreamGroupNotes();
		}
	}
	return;

}

void Beatmap::removeStreamGroupNotesWithGroupID(uint64_t groupID) {
	auto it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecStreamTemplates.end()) {
		vecStreamTemplates.erase(it);
	}
}

bool Beatmap::removeLongNotesNode(uint64_t groupID, int index) {
	auto it = std::find_if(vecGroupNotes.begin(), vecGroupNotes.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecGroupNotes.end()) {
		if(it->removeLongNotesNode(index)) {
			vecGroupNotes.erase(it);
			return true;
		}
	}
	return false;
}

void Beatmap::resetSoundAlreadyPlayed() {
	for (Note& n : notes) {
		n.setAlreadyPlayed(false);
	}
	for (GroupNotes& gn : vecGroupNotes) {
		for (size_t i = 0; i < gn.size(); i++) {
			Note& n = gn.getNoteAtIndex(i);
			n.setAlreadyPlayed(false);
		}
	}
	for (GroupNotes& gn : vecGeneratedStreamGroupNotes) {
		for (size_t i = 0; i < gn.size(); i++) {
			Note& n = gn.getNoteAtIndex(i);
			n.setAlreadyPlayed(false);
		}
	}
	return;
}

void Beatmap::setSongDuration(double _songDuration) {
	songDuration = _songDuration;
	return;
}

double Beatmap::getSongDuration() const {
	return songDuration;
}

void Beatmap::setRawExportBPM(float bpm) {
	if (bpm <= 0.0f) {
		std::cerr << "Beatmap::setRawExportBPM: invalid BPM value" << std::endl;
		return;
	}
	exportBPM = bpm;
	return;
}

float Beatmap::getExportBPM() const {
	if (bpmExportMode == BpmExportMode::Multiple) {
		return exportBPM;
	}
	else {
		return getFirstBPM();
	}
}

float Beatmap::getRawExportBPM() const {
	return exportBPM;
}

BpmExportMode Beatmap::getBpmExportMode() const {
	return bpmExportMode;
}

void Beatmap::setBpmExportMode(BpmExportMode mode) {
	bpmExportMode = mode;
}

double Beatmap::orderToSec(int order, double secPerBeat) const {
	return static_cast<double>(order * secPerBeat / ORDER_PER_BEAT);
}

int Beatmap::secToOrder(double timeSec, double secPerBeat) const {
	return static_cast<int>(std::round(timeSec * static_cast<double>(ORDER_PER_BEAT) / secPerBeat));
}

double Beatmap::calcTimeNormalized(int order, double secPerBeat) const {
	if (getSongDuration() == 0.0) return 0.0;
	return (static_cast<double>(order) * secPerBeat) / (static_cast<double>(ORDER_PER_BEAT) * getSongDuration());
}

float Beatmap::calcPosition2DX(int x) const {
	return 0.918f * x - 3.1f;
}

float Beatmap::calcPosition2DY(int y) const {
	return 0.918f * y - 4.33f;
}

void Beatmap::addTiming(float BPM, double offset) {
	Timing timing(BPM, offset);
	timingList.addTiming(timing);
	return;
}

TimingList& Beatmap::getTimingList() {
	return timingList;
}

const TimingList& Beatmap::getTimingList() const {
	return timingList;
}

size_t Beatmap::getTimingListSize() const {
	return timingList.size();
}

size_t Beatmap::getCurrentTimingIndex() const {
	return timingList.getCurrentIndex();
}

Note& Beatmap::getNoteAtIndex(size_t index) {
	if (index >= notes.size()) throw std::out_of_range("Beatmap::getNoteAtIndex: index out of range");
	return notes.at(index);
}

Note& Beatmap::getNoteByID(uint64_t noteID) {
	for (Note& n : notes) {
		if (n.getNoteID() == noteID) {
			return n;
		}
	}

	for (GroupNotes& gn : vecGroupNotes) {
		for (size_t i = 0; i < gn.size(); i++) {
			Note& n = gn.getNoteAtIndex(i);
			if (n.getNoteID() == noteID) {
				return n;
			}
		}
	}
	
	for (GroupNotes& gn : vecStreamTemplates) {
		for (size_t i = 0; i < gn.size(); i++) {
			Note& n = gn.getNoteAtIndex(i);
			if (n.getNoteID() == noteID) {
				return n;
			}
		}
	}

	throw std::out_of_range("Beatmap::getNoteByID: noteID not found");
}

GroupNotes& Beatmap::getGroupNotesAtIndex(size_t index) {
	if (index >= vecGroupNotes.size()) throw std::out_of_range("Beatmap::getGroupNotesAtIndex: index out of range");
	return vecGroupNotes.at(index);
}

GroupNotes& Beatmap::getGroupNotesWithGroupID(uint64_t groupID) {
	auto it = std::find_if(vecGroupNotes.begin(), vecGroupNotes.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecGroupNotes.end()) {
		return *it;
	}

	it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecStreamTemplates.end()) {
		return *it;
	}

	throw std::out_of_range("Beatmap::getGroupNotesWithGroupID: groupID not found");
}

GroupNotes& Beatmap::getStreamGroupNotesWithGroupID(uint64_t groupID) {
	auto it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecStreamTemplates.end()) {
		return *it;
	}
	throw std::out_of_range("Beatmap::getStreamGroupNotesWithGroupID: groupID not found");
}

const GroupNotes& Beatmap::getStreamGroupNotesWithGroupID(uint64_t groupID) const {
	auto it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](const GroupNotes& gn) { return gn.getGroupID() == groupID; });

	if (it != vecStreamTemplates.end()) {
		return *it;
	}
	throw std::out_of_range("Beatmap::getStreamGroupNotesWithGroupID: groupID not found");
}

std::vector<GroupNotes>& Beatmap::getGeneratedStreamGroupNotes() {
	return vecGeneratedStreamGroupNotes;
}

const std::vector<GroupNotes>& Beatmap::getGeneratedStreamGroupNotes() const {
	return vecGeneratedStreamGroupNotes;
}

size_t Beatmap::getGroupNotesSize() const {
	return notes.size();
}

size_t Beatmap::getVecGroupNotesLength() const {
	return vecGroupNotes.size();
}

size_t Beatmap::getStreamGroupNotesSize() const {
	return vecStreamTemplates.size();
}

void Beatmap::organizeVecGroupNotesIndex() {
	for (GroupNotes& group : vecGroupNotes) {
		group.organizeNotesIndex();
	}
	return;
}

bool Beatmap::existInStreamTemplateWithGroupID(uint64_t groupID) const {
	auto it = std::find_if(vecStreamTemplates.begin(), vecStreamTemplates.end(),
		[groupID](const GroupNotes& gn) { return gn.getGroupID() == groupID; });
	return it != vecStreamTemplates.end();
}

void Beatmap::sortNotesByTime() {
	std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
		return a.getTime() < b.getTime();
		});
	return;
}

void Beatmap::sortGroupNotesByTime() {
	std::sort(vecGroupNotes.begin(), vecGroupNotes.end(), [](const GroupNotes& a, const GroupNotes& b) {
		return a.getStartTime() < b.getStartTime();
		});
	return;
}

void Beatmap::rebuildGeneratedStreamGroupNotes() {
	vecGeneratedStreamGroupNotes.clear();

	for (const GroupNotes& group : vecGroupNotes) {
		if (group.getGroupType() != GroupType::STREAM) {
			continue;
		}

		std::vector<GroupNotes> generatedGroups = EditorUtility::generateStreamNotes(group, *this, streamSegmentDivisions);
		vecGeneratedStreamGroupNotes.insert(
			vecGeneratedStreamGroupNotes.end(),
			generatedGroups.begin(),
			generatedGroups.end());
	}
}

// method for TimingList
bool Beatmap::timingListisEmpty() const {
	return timingList.isEmpty();
}

//update which timing to use
bool Beatmap::updateTimingList(double currentTime) {
	return timingList.update(currentTime);

}

void Beatmap::setBPM(float bpm) {
	timingList.setBPM(bpm);
	return;
}

float Beatmap::getBPM() const {
	return timingList.getBPM();
}

float Beatmap::getFirstBPM() const {
	return timingList.getFirstBPM();
}

void Beatmap::setOffset(double offset) {
	timingList.setOffset(offset);
	return;
}

double Beatmap::getOffset() const {
	return timingList.getOffset();
}

double Beatmap::getFirstOffset() const {
	return timingList.getFirstOffset();
}

double Beatmap::getActualOffset() const {
	return timingList.getActualOffset();
}

double Beatmap::getActualFirstOffset() const {
	return timingList.getActualFirstOffset();
}

double Beatmap::getOffsetInNoteTime() const {
	return timingList.getOffsetInNoteTime();
}

void Beatmap::setOffsetInEditor(double offset) {
	timingList.setOffsetInEditor(offset);
	return;
}

double Beatmap::getOffsetInEditor() const {
	return timingList.getOffsetInEditor();
}


double Beatmap::getsecPerBeat() const {
	return timingList.secPerBeat();
}

double Beatmap::getFirstSecPerBeat() const {
	return timingList.firstSecPerBeat();
}

int Beatmap::getSubbeat() const {
	return timingList.getSubbeat();
}

int Beatmap::getCurrentBeat() const {
	return timingList.getCurrentBeat();
}

double Beatmap::calcCurrentTime(int beat) const {
	return timingList.calcCurrentTime(beat);
}

bool Beatmap::checkNextBeat(double currentTime) {
	return timingList.checkNextBeat(currentTime);
}

void Beatmap::resetBeat() {
	timingList.resetCurrentBeat();
	return;
}
