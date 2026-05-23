#pragma once

#include <glm.hpp>
#include <imgui.h>
#include "Note.h"

inline glm::vec2 toGlm(const ImVec2& v) {
	return glm::vec2(v.x, v.y);
}

inline ImVec2 toImVec2(const glm::vec2& v) {
	return ImVec2(v.x, v.y);
}

inline glm::vec3 toGlm(const Note& note) {
	return glm::vec3(note.getFloatX(), note.getFloatY(), static_cast<float>(note.getTime()));
}

inline void toNote(const glm::vec3& v, Note& note) {
	note.setFloatPosition(v.x, v.y);
	note.setTime(static_cast<double>(v.z));
	return;
}
