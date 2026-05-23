#include "InputState.h"

InputState::InputState() {
	initializeKeyBindings();
}

void InputState::initializeKeyBindings() {
	// Initialize default key bindings
	actionBindings[(int)Action::SelectPut] =			{ BindKind::MouseClick,		ImGuiKey_None,	ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::MultipleSelect] =		{ BindKind::MouseClick,		ImGuiKey_None,	ImGuiMouseButton_Left,		0,	Modifier::Ctrl };
	actionBindings[(int)Action::GroupNotesSelect] =		{ BindKind::MouseDouble,	ImGuiKey_None,	ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::SelectDelete] =			{ BindKind::MouseClick,		ImGuiKey_None,	ImGuiMouseButton_Right,		0,	Modifier::None };
	actionBindings[(int)Action::MultipleSelectDelete] = { BindKind::MouseClick,		ImGuiKey_None,	ImGuiMouseButton_Right,		0,	Modifier::Ctrl };
	actionBindings[(int)Action::PlayToggle] =			{ BindKind::KeyboardKey,	ImGuiKey_Space,	ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::SeekForward] =			{ BindKind::WheelY,			ImGuiKey_None,	ImGuiMouseButton_Left,		1,	Modifier::None };
	actionBindings[(int)Action::SeekRewind] =			{ BindKind::WheelY,			ImGuiKey_None,	ImGuiMouseButton_Left,		-1, Modifier::None };
	actionBindings[(int)Action::Undo] =					{ BindKind::KeyboardKey,	ImGuiKey_Z,		ImGuiMouseButton_Left,		0,	Modifier::Ctrl };
	actionBindings[(int)Action::CameraMove] =			{ BindKind::MouseClick,		ImGuiKey_None,	ImGuiMouseButton_Middle,	0,	Modifier::None };
	actionBindings[(int)Action::CameraForward] =		{ BindKind::KeyboardKey,	ImGuiKey_W,		ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::CameraBackward] =		{ BindKind::KeyboardKey,	ImGuiKey_S,		ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::CameraLeft] =			{ BindKind::KeyboardKey,	ImGuiKey_A,		ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::CameraRight] =			{ BindKind::KeyboardKey,	ImGuiKey_D,		ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::CameraUp] =				{ BindKind::KeyboardKey,	ImGuiKey_E,		ImGuiMouseButton_Left,		0,	Modifier::None };
	actionBindings[(int)Action::CameraDown] =			{ BindKind::KeyboardKey,	ImGuiKey_Q,		ImGuiMouseButton_Left,		0,	Modifier::None };
}

void InputState::updateInput() {

	ImGuiIO& io = ImGui::GetIO();
	// Update mouse position and delta
	mousePos = io.MousePos;
	mousePosDelta = io.MouseDelta;
	// Update wheel input
	wheelY = io.MouseWheel;
	wheelX = io.MouseWheelH;

	// Update keyboard/mouse capture state
	wantCaptureKeyboard = io.WantCaptureKeyboard;
	wantCaptureMouse = io.WantCaptureMouse;

	bool isCtrlDown = io.KeyCtrl;
	bool isShiftDown = io.KeyShift;
	bool isAltDown = io.KeyAlt;

	//update action bindings state
	for(int i = 0; i < (int)Action::Count; ++i) {
		Binding& binding = actionBindings[i];

		const bool isKeyboardBinding = binding.kind == BindKind::KeyboardKey;
		if ((isKeyboardBinding && wantCaptureKeyboard)) {
			binding.state.down = false;
			binding.state.pressed = false;
			binding.state.released = false;
			continue;
		}
		
		// Check modifiers key input
		if (((binding.modifier & Modifier::Ctrl) != isCtrlDown) ||
			((binding.modifier & Modifier::Shift) != isShiftDown) ||
			((binding.modifier & Modifier::Alt) != isAltDown)		) {
			binding.state.down = false;
			binding.state.pressed = false;
			binding.state.released = false;
			continue;
		}

		switch (binding.kind) {
			case BindKind::KeyboardKey:
				binding.state.down = ImGui::IsKeyDown(binding.key);
				binding.state.pressed = ImGui::IsKeyPressed(binding.key);
				binding.state.released = ImGui::IsKeyReleased(binding.key);
				break;
			case BindKind::MouseClick:
				binding.state.down = ImGui::IsMouseDown(binding.mouseBtn);
				binding.state.pressed = ImGui::IsMouseClicked(binding.mouseBtn);
				binding.state.released = ImGui::IsMouseReleased(binding.mouseBtn);
				break;
			case BindKind::MouseDouble:
				binding.state.down = ImGui::IsMouseDown(binding.mouseBtn); 
				binding.state.pressed = ImGui::IsMouseDoubleClicked(binding.mouseBtn);
				binding.state.released = ImGui::IsMouseReleased(binding.mouseBtn);
				break;
			case BindKind::WheelX:
				binding.state.down = wheelX * binding.wheelDir > 0; 
				binding.state.pressed = binding.state.down && !binding.state.pressed; // pressed if it's down now but wasn't pressed before
				binding.state.released = false;
				break;
			case BindKind::WheelY:
				binding.state.down = wheelY * binding.wheelDir > 0; 
				binding.state.pressed = binding.state.down && !binding.state.pressed; // pressed if it's down now but wasn't pressed before
				binding.state.released = false;
				break;
			case BindKind::None:
			default:
				binding.state.down = false;
				binding.state.pressed = false;
				binding.state.released = false;
				break;
		}
	}
}

float InputState::getWheelX() const {
	return wheelX;
}

float InputState::getWheelY() const {
	return wheelY;
}

ImVec2 InputState::getMousePos() const {
	return mousePos;
}

ImVec2 InputState::getMouseDelta() const {
	return mousePosDelta;
}

bool InputState::isActionPressed(Action action) const {
	if ((int)action >= (int)Action::Count) return false;
	return actionBindings[(int)action].state.pressed;
}

bool InputState::isActionDown(Action action) const {
	if ((int)action >= (int)Action::Count) return false;
	return actionBindings[(int)action].state.down;
}

bool InputState::isActionReleased(Action action) const {
	if ((int)action >= (int)Action::Count) return false;
	return actionBindings[(int)action].state.released;
}
