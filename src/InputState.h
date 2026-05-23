#pragma once
#include <array>
#include <imgui.h>

enum class Action {
	SelectPut,
	MultipleSelect,
	GroupNotesSelect,
	SelectDelete,
	MultipleSelectDelete,
	PlayToggle,
	SeekForward,
	SeekRewind,
	Undo,
	CameraMove,
	CameraForward,
	CameraBackward,
	CameraLeft,
	CameraRight,
	CameraUp,
	CameraDown,
	Count	// the last element is number of actions
};

enum class BindKind{
	KeyboardKey,
	MouseClick,    
	MouseDouble,   
	WheelX,
	WheelY,
	None
};

enum class Modifier : std::uint8_t {
	None = 0,
	Ctrl = 1 << 0,
	Shift = 1 << 1,
	Alt = 1 << 2,
	Super = 1 << 3
};

inline constexpr Modifier operator|(Modifier a, Modifier b) noexcept {
	return static_cast<Modifier>(static_cast<int>(a) | static_cast<int>(b));
}

inline constexpr bool operator&(Modifier a, Modifier b) noexcept {
	return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

// Tracks the frame-level state of one logical input binding.
struct KeyState {
	bool down = false; // current state
	bool pressed = false; // true only on the frame the key is pressed
	bool released = false; // true only on the frame the key is released
};

// Maps one editor action to a keyboard, mouse, or wheel input source.
struct Binding {
	BindKind kind{};
	ImGuiKey key = ImGuiKey_None;						// use only when kind==KeyboardKey 
	ImGuiMouseButton mouseBtn = ImGuiMouseButton_Left;	// use only when kind==MouseClick or MouseDouble
	int wheelDir = 0;									// for Wheel(+1 or -1)
	Modifier modifier = Modifier::None; 
	
	KeyState state; // current state of input
};

// Collects ImGui input state and exposes it as editor actions for the current frame.
class InputState {
public:
	InputState();
	~InputState() = default;
	void initializeKeyBindings();
	void updateInput();

	ImVec2 getMousePos() const;
	ImVec2 getMouseDelta() const;
	float getWheelX() const;
	float getWheelY() const;

	bool isActionPressed(Action action) const;
	bool isActionDown(Action action) const;
	bool isActionReleased(Action action) const;

private: 
	float wheelX = 0.0f;
	float wheelY = 0.0f;
	ImVec2 mousePos{};
	ImVec2 mousePosDelta{};
	bool wantCaptureKeyboard = false;
	bool wantCaptureMouse = false;

	std::array<Binding, (int)Action::Count> actionBindings;
};
