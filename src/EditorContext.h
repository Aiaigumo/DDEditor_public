#pragma once
#include <vector>
#include <array>
#include <string>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <imgui.h>
#include "Note.h"
#include "AudioPlayer.h"

enum class EditMode {
	LEFT_HAND,
	RIGHT_HAND,
	LEFT_FOOT,
	RIGHT_FOOT,
	LONG_LEFT_FOOT,
	LONG_RIGHT_FOOT,
	BAR,
	OBSTACLE,
	TRAP,
	STREAM,
	MOVE,
	NONE
};

// Stores shared editor state used by UI, input handling, rendering, audio playback, and edit tools.
class EditorContext {
public:
	EditorContext()
	{
		updateViewOrtho();
		updateProjectionOrtho();
		updateViewPerspective();
		updateProjectionPerspective();
	}
	~EditorContext() = default;

	double songLength = 0.0;
	double currentPlayerTime = 0.0;
	bool isSongPlaying = false;
	
	//orthographic view
	float orthoDrawLeftBound = -2.0f;
	float orthoDrawRightBound = 11.0f;
	float orthoDrawMaxDistance = 32.0f;
	float orthoDrawMinDistance = 0.0f;
	float orthoDrawMaxHeight = 12.0f;
	float orthoDrawMinHeight = -1.0f;

	glm::mat4 viewOrtho;
	glm::mat4 projectionOrtho;
	void updateViewOrtho() {
		viewOrtho = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	}
	void updateProjectionOrtho() {
		projectionOrtho = glm::ortho(
			orthoDrawLeftBound,			// left
			orthoDrawRightBound,			// right
			orthoDrawMinDistance,			// bottom
			orthoDrawMaxDistance,			// top
			-orthoDrawMaxHeight,			// near clip
			-orthoDrawMinHeight			// far clip
		);
	}

	//perspective view 
	static inline constexpr float DEFAULT_CAMERA_YAW = -90.0f;
	static inline constexpr float DEFAULT_CAMERA_PITCH = -10.0f;
	float yaw = DEFAULT_CAMERA_YAW;
	float pitch = DEFAULT_CAMERA_PITCH;
	float cameraSensitivity = 0.2f;
	float cameraMoveSpeed = 5.0f;
	const glm::vec3 cameraPosInitial = glm::vec3(5.0f, 7.0f, 5.0f);			// Camera position
	const glm::vec3 cameraUpInitial = glm::vec3(0.0f, 1.0f, 0.0f);			// Direction of "up"
	glm::vec3 cameraPos = cameraPosInitial;									// Camera position
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);;					// Look at direction
	glm::vec3 cameraUp = cameraUpInitial;									// Direction of "up"
	static inline constexpr float DEFAULT_CAMERA_FOV = 90.0f;
	float FOV3d = DEFAULT_CAMERA_FOV;
	float perspectiveDrawMaxDistance = 64.0f;
	float perspectiveDrawMinDistance = 0.0f;
	float perspectiveCameraNearClip = 0.1f;
	float perspectiveCameraFarClip = 100.0f;

	glm::mat4 viewPerspective;
	glm::mat4 projectionPerspective;
	void updateViewPerspective() {
		viewPerspective = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	}
	void updateProjectionPerspective() {
		projectionPerspective = glm::perspective(
			glm::radians(FOV3d),				// fov
			(float)psFBOWidth / (float)psFBOHeight,						// aspect ratio
			perspectiveCameraNearClip,		// near clip
			perspectiveCameraFarClip			// far clip
		);
	}

	// scales for drawing objects
	glm::vec3 handnotesScale = glm::vec3(0.4f);
	glm::vec3 selectedAdditionalScale = glm::vec3(0.05f);
	glm::vec3 editingLongNotesLineAdditionalScale = glm::vec3(0.05f, -0.05f, 0.0f);
	glm::vec3 footnotesScale = glm::vec3(0.8f, 0.2f, 0.5f);
	glm::vec3 barScale = glm::vec3(11.0f, 1.0f, 1.0f);
	glm::vec2 obsScale = glm::vec2(1.0f, 6.0f); // (x, y)
	glm::vec3 trapScale = glm::vec3(1.0f, 0.2f, 0.2f);
	glm::vec3 nodeScale = glm::vec3(0.3f, 0.3f, 0.3f);

	//rendering variables
	bool drawAxis = false;
	bool drawLaneNum3d = false, drawLaneNum2d = false;
	const float DEFAULT_DIST_PER_BEAT = 6.0f;
	float distPerBeat = DEFAULT_DIST_PER_BEAT;
	float hitLineDistance = distPerBeat;
	float editingZ = -hitLineDistance;
	float laneMoveDistance = 0.0f;
	float hitRadius = handnotesScale.x;
	float rangeHitLine = 1.0f;
	int subbeat = 1;
	int longNoteSegmentDivisions = 32;
	int streamSegmentDivisions = 32;
	glm::vec2 streamLineScale = glm::vec2(0.08f, 0.08f);

	static inline constexpr float PUTTING_AREA_MIN = 0.0f;
	static inline constexpr float PUTTING_AREA_MAX = 10.0f;
	

	// note data in rendering range
	std::vector<glm::vec3> slhNotes;	//  selected hand notes
	std::vector<glm::vec3> slfNotes;	//  selected foot notes
	std::vector<glm::vec3> lhNotes;		//	left hand
	std::vector<glm::vec3> rhNotes;		//	right hand
	std::vector<glm::vec3> lfNotes;		//	left foot
	std::vector<glm::vec3> rfNotes;		//	right foot
	std::vector<glm::vec3> llfLines;	//	long left foot lines
	std::vector<glm::vec3> lrfLines;	//	long right foot lines
	std::vector<glm::vec3> elfLines;	//	editing long foot lines
	std::vector<glm::vec3> yhNotes;		//	hand notes on hitline (draw in yellow)
	std::vector<glm::vec3> yfNotes;		//	foot notes on hitline (draw in yellow)
	std::vector<glm::vec3> bars;		//	bar
	std::vector<glm::vec3> slbars;		//	selected bar
	std::vector<glm::vec4> obstacles;	//	obscacles (x,y,startZ, endZ)
	std::vector<glm::vec4> slobstacles;	//	selected obscacles (x,y,startZ, endZ)
	std::vector<glm::vec3> traps;		//	trap
	std::vector<glm::vec3> sltraps;		//	selected trap
	std::vector<glm::vec3> nodes;		//	stream nodes
	std::vector<glm::vec3> slnodes;		//	selected stream nodes
	std::vector<glm::vec3> enodes;      //  editing nodes
	std::vector<glm::vec3> streamLines; //  stream curve lines
	std::vector<glm::vec3> editingStreamLines; // editing stream curve lines
	std::vector<float> timingLinesOnSubbeat;	// timing lines on subbeat
	std::vector<float> timingLinesOnBeat;	// timing lines on beat
	std::vector<float> timingStartLines;	// timing start lines
	void clearDrawingPositions() {
		slhNotes.clear();			//  selected hand notes
		slfNotes.clear();			//  selected foot notes
		lhNotes.clear();				//	left hand
		rhNotes.clear();				//	right hand
		lfNotes.clear();				//	left foot
		rfNotes.clear();				//	right foot
		llfLines.clear();			//	long left foot
		lrfLines.clear();			//	long right foot
		elfLines.clear();			//	long right foot
		yhNotes.clear();				//	hand notes on hitline (draw in yellow)
		yfNotes.clear();				//	foot notes on hitline (draw in yellow)
		bars.clear();				//	bar
		slbars.clear();				//	selected bar
		obstacles.clear();			//	obscacles (x,y,startZ, endZ)
		slobstacles.clear();			//	selected obscacles (x,y,startZ, endZ)
		traps.clear();				//	trap
		sltraps.clear();				//	selected trap
		nodes.clear();
		slnodes.clear();
		enodes.clear();
		streamLines.clear();
		editingStreamLines.clear();
		timingLinesOnSubbeat.clear();	// timing lines on subbeat
		timingLinesOnBeat.clear();		// timing lines on beat
		timingStartLines.clear();	// timing start lines
	}

	// window status
	float deltaTime = 0.0f;
	float lastTime = 0.0f;
	ImVec2 contentOrigin3d = ImVec2(0.0f, 0.0f);
	ImVec2 contentOrigin2d = ImVec2(0.0f, 0.0f);
	int psFBOWidth = 512;
	int psFBOHeight = 1024;
	int orFBOWidth = 1024;
	int orFBOHeight = 1024;
	int prevPsFBOWidth = psFBOWidth;
	int prevPsFBOHeight = psFBOHeight;
	int prevOrFBOWidth = orFBOWidth;
	int prevOrFBOHeight = orFBOHeight;
	bool isHovered2d = false, isHoveredContent2d = false;
	bool isHovered3d = false, isHoveredContent3d = false;
	bool isDraggingNotes3d = false, isDraggingNotes2d = false;
	glm::vec3 dragStartWorldPos{ 0.0f };	// world pos where note dragging started
	glm::vec3 dragGridWorldPos{ 0.0f };

	// Beatmap loader states
	inline static const std::vector<std::string> beatmapDiff = { "Easy", "Normal" ,"Hard" ,"Expert" ,"Master" ,"Legend" };
	std::string jsonExtension = ".json";
	std::string infoPath = "info.json";
	std::string beatmapFolderPathBuffer = "ExampleBeatmap";
	std::string diffPathBuffer = ".json";
	std::string outPutDiffPathBuffer = "Easy.json";
	std::string outPutBeatmapFolderPathBuffer = "OutputBeatmap";
	std::array<bool, 6> diffLoaded = { false, false, false, false, false, false };
	int currentDiffIndex = 0; // no diff selected(-1)  0-5 : diff index

	// Song player states
	inline static const std::vector<std::string> audioExtensions = { ".ogg", ".wav", ".wave", ".mp3" };
	inline static const std::string soundEffectFolder = "soundeffect";
	AudioPlayer songPlayer = AudioPlayer();
	std::string songPathBuffer = "audio.mp3";
	std::string loadedSongFilePath = "";
	bool hasLoadedBeatmapFolder = false;
	bool pendingConfirmNewBeatmapFromSongLoadModal = false;
	std::string pendingSongLoadPath = "";
	bool pendingSongCopyStatusModal = false;
	std::string songCopyStatusMessage = "";
	float songVolume = 0.1f;
	

	// GUI constants
	float itemWidth3char = 30.0f;
	float itemWidth5char = 45.0f;
	float itemWidth7char = 60.0f;
	float itemWidthSlider = 200.0f;
	float itemWidthStep = 45.0f;

	//Editor status variables
	EditMode editMode = EditMode::MOVE;
	bool isNotesPlaceConstraint = true;
	int obstaclePutLength = 1; //in subbeats
	NoteType streamNoteType = NoteType::LEFT_HAND;
	double streamRepeatIntervalSec = 1.0;
	
	bool duringRectSelect2d = false;
	bool duringRectSelect3d = false;
	ImVec2 rectSelectStartPos{ 0.0f, 0.0f };
	ImVec2 rectSelectEndPos{ 0.0f, 0.0f };
	
	//Editing objects
	std::vector<uint64_t> selectedNotes{};
	uint64_t groupNotesEditing = 0;
	void clearSelectedNotes() {
		selectedNotes.clear();
		groupNotesEditing = 0;
	}

	//SoundEffects
	float metronomeVolume = 0.1f;
	AudioPlayer metronomePlayer = AudioPlayer();
	float hitSoundVolume = 0.1f;
	AudioPlayer hitHandPlayer = AudioPlayer();
	AudioPlayer hitFootPlayer = AudioPlayer();

};
