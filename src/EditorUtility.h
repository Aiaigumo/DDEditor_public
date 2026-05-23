#pragma once

#include <glm.hpp>
#include <imgui.h>
#include <string>
#include "Beatmap.h"
#include "BeatmapStack.h"
#include "EditorContext.h"

class Editor;

// Provides UI and geometry helper functions that collect input and delegate data edits to Editor.
namespace EditorUtility {
float checkMouseRaySphere(const glm::vec3& notePos, const ImVec2& mousePos, const EditorContext& ec, bool is3d);
glm::vec3 ndcToWorld(const glm::vec4& ndc, const glm::mat4& invVP);
glm::vec4 worldToClip(const glm::vec3& worldPos, const glm::mat4& matPV);
ImVec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& matPV, ImVec2 windowOrigin, int width, int height);
float calcNotePosZ(double noteTimeSec, const Beatmap& beatmap, const EditorContext& ec);
glm::vec3 calcNotePos(const Note& note, const Beatmap& beatmap, const EditorContext& ec);
glm::vec2 mouseToWorldOnPlaneX(const ImVec2& mousePos, const EditorContext& ec, float planeX);
glm::vec2 mouseToWorldOnPlaneY(const ImVec2& mousePos, const EditorContext& ec, float planeY);
glm::vec2 mouseToWorldOnPlaneZ(const ImVec2& mousePos, const EditorContext& ec, float planeZ);
glm::ivec2 constrainNotePosXY(const glm::ivec2& pos, const EditorContext& ec);
bool isInRect(const ImVec2& point, const ImVec2& rectLeftTop, const ImVec2& rectRightBottom);
double calcTimeWithBeat(double time, const Beatmap& beatmap, const EditorContext& ec);
double snapToStep(double value, double step);
double calcNoteTimefromPosZ(float posZ, const Beatmap& beatmap, const EditorContext& ec);
bool checkOnHitLine(float z, float hitline_z, float rangeHitLine);
bool isThereAlreadyNote(glm::vec3 pos, const EditorContext& ec);
std::vector<glm::vec3> buildLongNoteRenderPolyline(const GroupNotes& longNoteGroup, const Beatmap& beatmap, const EditorContext& ec);
std::vector<glm::vec3> buildStreamRenderPolyline(const GroupNotes& streamGroup, const Beatmap& beatmap, const EditorContext& ec);
std::vector<GroupNotes> generateStreamNotes(const GroupNotes& streamGroup, const Beatmap& beatmap, int sampleDivision);
void playHitSound(Note& note, EditorContext& ec);
bool hasExistingExportFiles(const EditorContext& ec);
bool drawNoteProperty(uint64_t noteID, Editor& editor, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack);
}
