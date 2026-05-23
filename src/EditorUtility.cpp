#include "EditorUtility.h"
#include "Editor.h"
#include "CurvePath.h"
#include "CurveAdapters.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>

namespace EditorUtility {

	float checkMouseRaySphere(const glm::vec3& notePos, const ImVec2& mousePos, const EditorContext& ec, bool is3d) {
		float rayMaxDistance = FLT_MAX;

		glm::mat4 invVP(1.0f);
		float originX = 0.0f;
		float originY = 0.0f;
		int width = 0;
		int height = 0;
		if (is3d) {
			invVP = glm::inverse(ec.projectionPerspective * ec.viewPerspective);
			originX = ec.contentOrigin3d.x;
			originY = ec.contentOrigin3d.y;
			width = ec.psFBOWidth;
			height = ec.psFBOHeight;
			rayMaxDistance = 32.0f;
		}
		else {
			invVP = glm::inverse(ec.projectionOrtho * ec.viewOrtho);
			originX = ec.contentOrigin2d.x;
			originY = ec.contentOrigin2d.y;
			width = ec.orFBOWidth;
			height = ec.orFBOHeight;
		}

		glm::vec4 ndcNear = glm::vec4(
			((mousePos.x - originX) / width) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - originY) / height) * 2.0f,
			-1.0f,
			1.0f
		);
		glm::vec4 ndcFar = glm::vec4(
			((mousePos.x - originX) / width) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - originY) / height) * 2.0f,
			1.0f,
			1.0f
		);
		glm::vec3 worldNear = ndcToWorld(ndcNear, invVP);
		glm::vec3 worldFar = ndcToWorld(ndcFar, invVP);
		glm::vec3 rayDir = glm::normalize(worldFar - worldNear);

		glm::vec3 op = notePos - worldNear;
		float t = glm::dot(op, rayDir);
		if (t < 0 || t > rayMaxDistance) return -1.0f;
		glm::vec3 closestPointOnRay = worldNear + t * rayDir;
		float distance = glm::length(closestPointOnRay - notePos);
		if (distance < ec.hitRadius) {
			return t;
		}
		return -1.0f;
	}

	glm::vec3 ndcToWorld(const glm::vec4& ndc, const glm::mat4& invVP) {
		glm::vec4 w = invVP * ndc;
		return glm::vec3(w) / w.w;
	}

	glm::vec4 worldToClip(const glm::vec3& worldPos, const glm::mat4& matPV) {
		return matPV * glm::vec4(worldPos, 1.0f);
	}

	ImVec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& matPV, ImVec2 windowOrigin, int width, int height) {
		glm::vec4 clip = worldToClip(worldPos, matPV);
		if (clip.w <= 0) {
			return ImVec2(-1, -1);
		}
		glm::vec3 writendc = glm::vec3(clip) / clip.w;
		if (writendc.x < -1.0f || writendc.x > 1.0f ||
			writendc.y < -1.0f || writendc.y > 1.0f ||
			writendc.z < -1.0f || writendc.z > 1.0f) {
			return ImVec2(-1, -1);
		}
		ImVec2 screen;
		screen.x = windowOrigin.x + (writendc.x * 0.5f + 0.5f) * width;
		screen.y = windowOrigin.y + (1.0f - (writendc.y * 0.5f + 0.5f)) * height;
		return screen;
	}

	float calcNotePosZ(double noteTimeSec, const Beatmap& beatmap, const EditorContext& ec) {
		return static_cast<float>(-((noteTimeSec - beatmap.getOffsetInNoteTime()) / beatmap.getsecPerBeat() * ec.distPerBeat - ec.laneMoveDistance + ec.hitLineDistance));
	}

	glm::vec3 calcNotePos(const Note& note, const Beatmap& beatmap, const EditorContext& ec) {
		return glm::vec3(
			note.getFloatX(),
			note.getFloatY(),
			static_cast<float>(calcNotePosZ(note.getTime(), beatmap, ec))
		);
	}

	glm::vec2 mouseToWorldOnPlaneZ(const ImVec2& mousePos, const EditorContext& ec, float planeZ) {
		glm::mat4 invVP = glm::inverse(ec.projectionPerspective * ec.viewPerspective);

		glm::vec4 ndcNear = glm::vec4(
			((mousePos.x - ec.contentOrigin3d.x) / ec.psFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin3d.y) / ec.psFBOHeight) * 2.0f,
			-1.0f,
			1.0f
		);
		glm::vec4 ndcFar = glm::vec4(
			((mousePos.x - ec.contentOrigin3d.x) / ec.psFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin3d.y) / ec.psFBOHeight) * 2.0f,
			1.0f,
			1.0f
		);
		glm::vec3 worldNear = ndcToWorld(ndcNear, invVP);
		glm::vec3 worldFar = ndcToWorld(ndcFar, invVP);
		glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
		float t = (planeZ - worldNear.z) / rayDir.z;
		glm::vec3 intersectPos = worldNear + t * rayDir;
		return glm::vec2(intersectPos.x, intersectPos.y);
	}

	glm::vec2 mouseToWorldOnPlaneY(const ImVec2& mousePos, const EditorContext& ec, float planeY) {
		glm::mat4 invVP = glm::inverse(ec.projectionOrtho * ec.viewOrtho);

		glm::vec4 ndcNear = glm::vec4(
			((mousePos.x - ec.contentOrigin2d.x) / ec.orFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin2d.y) / ec.orFBOHeight) * 2.0f,
			-1.0f,
			1.0f
		);
		glm::vec4 ndcFar = glm::vec4(
			((mousePos.x - ec.contentOrigin2d.x) / ec.orFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin2d.y) / ec.orFBOHeight) * 2.0f,
			1.0f,
			1.0f
		);
		glm::vec3 worldNear = ndcToWorld(ndcNear, invVP);
		glm::vec3 worldFar = ndcToWorld(ndcFar, invVP);
		glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
		float t = (planeY - worldNear.y) / rayDir.y;
		glm::vec3 intersectPos = worldNear + t * rayDir;
		return glm::vec2(intersectPos.x, intersectPos.z);
	}

	glm::vec2 mouseToWorldOnPlaneX(const ImVec2& mousePos, const EditorContext& ec, float planeX) {
		glm::mat4 invVP = glm::inverse(ec.projectionPerspective * ec.viewPerspective);

		glm::vec4 ndcNear = glm::vec4(
			((mousePos.x - ec.contentOrigin3d.x) / ec.psFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin3d.y) / ec.psFBOHeight) * 2.0f,
			-1.0f,
			1.0f
		);
		glm::vec4 ndcFar = glm::vec4(
			((mousePos.x - ec.contentOrigin3d.x) / ec.psFBOWidth) * 2.0f - 1.0f,
			1.0f - ((mousePos.y - ec.contentOrigin3d.y) / ec.psFBOHeight) * 2.0f,
			1.0f,
			1.0f
		);
		glm::vec3 worldNear = ndcToWorld(ndcNear, invVP);
		glm::vec3 worldFar = ndcToWorld(ndcFar, invVP);
		glm::vec3 rayDir = glm::normalize(worldFar - worldNear);
		float t = (planeX - worldNear.x) / rayDir.x;
		glm::vec3 intersectPos = worldNear + t * rayDir;
		return glm::vec2(intersectPos.x, intersectPos.y);
	}

	glm::ivec2 constrainNotePosXY(const glm::ivec2& pos, const EditorContext& ec) {
		glm::ivec2 cpos;
		cpos.x = glm::clamp(pos.x, static_cast<int>(ec.PUTTING_AREA_MIN), static_cast<int>(ec.PUTTING_AREA_MAX));
		cpos.y = glm::clamp(pos.y, static_cast<int>(ec.PUTTING_AREA_MIN), static_cast<int>(ec.PUTTING_AREA_MAX));
		return cpos;
	}

	bool isInRect(const ImVec2& point, const ImVec2& rectLeftTop, const ImVec2& rectRightBottom) {
		return point.x >= rectLeftTop.x && point.x <= rectRightBottom.x &&
			point.y >= rectLeftTop.y && point.y <= rectRightBottom.y;
	}

	double calcTimeWithBeat(double time, const Beatmap& beatmap, const EditorContext& ec) {
		double unit = beatmap.getsecPerBeat() / static_cast<double>(ec.subbeat);
		double calctime = (std::round((time - beatmap.getOffsetInNoteTime()) / unit) * unit) + beatmap.getOffsetInNoteTime();
		return calctime;
	}

	double snapToStep(double value, double step) {
		if (step <= CurvePathDetail::TIME_EPSILON) {
			return value;
		}
		return std::round(value / step) * step;
	}

	double calcNoteTimefromPosZ(float posZ, const Beatmap& beatmap, const EditorContext& ec) {
		return beatmap.getOffsetInNoteTime() + ((-posZ + ec.laneMoveDistance - ec.hitLineDistance) / ec.distPerBeat) * beatmap.getsecPerBeat();
	}

	bool checkOnHitLine(float z, float hitline_z, float rangeHitLine) {
		return z > hitline_z - rangeHitLine / 2.0f && z < hitline_z + rangeHitLine / 2.0f;
	}

	bool isThereAlreadyNote(glm::vec3 pos, const EditorContext& ec) {
		float minDist = 0.4f;
		for (const glm::vec3& npos : ec.lhNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.rhNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.yhNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.lfNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.rfNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.yfNotes) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.bars) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec3& npos : ec.traps) {
			if (glm::distance(npos, pos) < minDist) return true;
		}
		for (const glm::vec4& npos : ec.obstacles) {
			glm::vec3 startPos = glm::vec3(npos.x, npos.y, npos.z);
			glm::vec3 endPos = glm::vec3(npos.x, npos.y, npos.w);
			if (glm::distance(startPos, pos) < minDist) return true;
			if (glm::distance(endPos, pos) < minDist) return true;
		}
		return false;
	}

	std::vector<glm::vec3> buildLongNoteRenderPolyline(const GroupNotes& longNoteGroup, const Beatmap& beatmap, const EditorContext& ec) {
		int divisionsPerSegment = ec.longNoteSegmentDivisions;
		std::vector<glm::vec3> points{};
		if (longNoteGroup.getGroupType() != GroupType::LONG_NOTE || longNoteGroup.size() == 0) {
			return points;
		}

		CurvePath<glm::vec3> curvePath{};
		for (size_t i = 0; i < longNoteGroup.size(); ++i) {
			const Note& note = longNoteGroup.getNoteAtIndex(i);
			curvePath.addNode(calcNotePos(note, beatmap, ec));
		}

		if (curvePath.nodeCount() < 2) {
			if (curvePath.nodeCount() == 1) {
				points.push_back(calcNotePos(longNoteGroup.getNoteAtIndex(0), beatmap, ec));
			}
			return points;
		}

		constexpr float samePosEpsilon = 0.0001f;
		curvePath.setAllNodeMode(SegmentMode::CatmullRom);
		for (size_t i = 0; i + 1 < curvePath.nodeCount(); ++i) {
			const glm::vec3& current = curvePath.getNode(i);
			const glm::vec3& next = curvePath.getNode(i + 1);
			const bool isLinearPair =
				std::fabs(current.x - next.x) <= samePosEpsilon ||
				std::fabs(current.z - next.z) <= samePosEpsilon;
			if (isLinearPair) {
				curvePath.setNodeMode(i, SegmentMode::Linear);
				curvePath.setNodeMode(i + 1, SegmentMode::Linear);
			}
		}

		const int clampedDivisions = (std::max)(divisionsPerSegment, 1);
		points = curvePath.sampleWholePath(clampedDivisions);
		return points;
	}

	std::vector<glm::vec3> buildStreamRenderPolyline(const GroupNotes& streamGroup, const Beatmap& beatmap, const EditorContext& ec) {
		std::vector<glm::vec3> points{};
		if (streamGroup.getGroupType() != GroupType::STREAM || streamGroup.size() == 0) {
			return points;
		}

		CurvePath<glm::vec3> curvePath{};
		for (size_t i = 0; i < streamGroup.size(); ++i) {
			const Note& node = streamGroup.getNoteAtIndex(i);
			if (node.getType() != NoteType::STREAM_NODE) {
				return {};
			}
			curvePath.addNode(calcNotePos(node, beatmap, ec));
			curvePath.setNodeMode(i, node.getSegmentMode());
		}

		if (curvePath.nodeCount() < 2) {
			if (curvePath.nodeCount() == 1) {
				points.push_back(calcNotePos(streamGroup.getNoteAtIndex(0), beatmap, ec));
			}
			return points;
		}

		const int clampedDivisions = (std::max)(ec.streamSegmentDivisions, 1);
		points = curvePath.sampleWholePath(clampedDivisions);
		return points;
	}

	std::vector<GroupNotes> generateStreamNotes(const GroupNotes& streamGroup, const Beatmap& beatmap, int sampleDivision) {
		std::vector<GroupNotes> generatedGroups{};
		if (streamGroup.getGroupType() != GroupType::STREAM || streamGroup.size() < 2) {
			return generatedGroups;
		}

		const std::vector<uint64_t>& templateGroupIDs = streamGroup.getStreamGroupNotesIDs();
		if (templateGroupIDs.empty()) {
			return generatedGroups;
		}

		CurvePath<glm::vec3> curvePath{};
		for (size_t i = 0; i < streamGroup.size(); ++i) {
			const Note& node = streamGroup.getNoteAtIndex(i);
			if (node.getType() != NoteType::STREAM_NODE) {
				return {};
			}
			curvePath.addNode(toGlm(node));
			curvePath.setNodeMode(i, node.getSegmentMode());
		}

		const int clampedDivisions = (std::max)(sampleDivision, 1);
		const std::vector<glm::vec3> sampledPoints = curvePath.sampleWholePathByAxis(
			static_cast<float>(streamGroup.getStreamRepeatIntervalSec()),
			[](const glm::vec3& point) { return point.z; },
			clampedDivisions,
			AxisSampleCrossingMode::First);
		if (sampledPoints.empty()) {
			return generatedGroups;
		}

		generatedGroups.reserve(sampledPoints.size() * templateGroupIDs.size());
		for (const glm::vec3& sampledPoint : sampledPoints) {
			for (uint64_t templateGroupID : templateGroupIDs) {
				const GroupNotes& templateGroup = beatmap.getStreamGroupNotesWithGroupID(templateGroupID);
				if (templateGroup.size() == 0) {
					continue;
				}
				GroupNotes generatedGroup = templateGroup;
				for (size_t i = 0; i < generatedGroup.size(); ++i) {
					Note& generatedNote = generatedGroup.getNoteAtIndex(i);
					const float snappedX = std::round(generatedNote.getFloatX() + sampledPoint.x);
					const float snappedY = std::round(generatedNote.getFloatY() + sampledPoint.y);
					generatedNote.setFloatPosition(snappedX, snappedY);
					generatedNote.setTime(generatedNote.getTime() + static_cast<double>(sampledPoint.z));
				}
				generatedGroups.push_back(generatedGroup);
			}
		}

		return generatedGroups;
	}

	void playHitSound(Note& note, EditorContext& ec) {
		if (!note.getAlreadyPlayed()) {
			switch (note.getType()) {
			case NoteType::LEFT_HAND:
			case NoteType::RIGHT_HAND:
				ec.hitHandPlayer.playFromStart();
				break;
			case NoteType::LEFT_FOOT:
			case NoteType::RIGHT_FOOT:
			case NoteType::LONG_LEFT_FOOT:
			case NoteType::LONG_RIGHT_FOOT:
				ec.hitFootPlayer.playFromStart();
				break;
			default:
				break;
			}
			note.setAlreadyPlayed(true);
		}
	}

	bool hasExistingExportFiles(const EditorContext& ec) {
		std::error_code fsError;
		std::filesystem::path outDir = ec.outPutBeatmapFolderPathBuffer;
		std::filesystem::path infoFile = outDir / ec.infoPath;
		std::filesystem::path levelFile = outDir / ec.outPutDiffPathBuffer;
		return std::filesystem::exists(infoFile, fsError) || std::filesystem::exists(levelFile, fsError);
	}

	bool drawNoteProperty(uint64_t noteID, Editor& editor, EditorContext& ec, Beatmap& beatmap, BeatmapStack& beatmapStack) {

		Note& note = beatmap.getNoteByID(noteID);
		NoteType type = note.getType();

		bool isChanged = false;
		bool durationChanged = false;
		int x = 0, y = 0, prevX = 0, prevY = 0;
		float floatX = 0.0f, floatY = 0.0f, prevFloatX = 0.0f, prevFloatY = 0.0f;
		double timeSec = 0.0, prevTimeSec = 0.0, duration = 0.0;

		double step = beatmap.getsecPerBeat() / static_cast<double>(ec.subbeat);

		switch (type) {
		case NoteType::LEFT_HAND:
		case NoteType::RIGHT_HAND:
		case NoteType::RIGHT_FOOT:
		case NoteType::LEFT_FOOT:
		case NoteType::BAR:
		case NoteType::TRAP:
		{
			x = prevX = note.getX();
			y = prevY = note.getY();
			timeSec = prevTimeSec = note.getTime();

			ImGui::Text(note.getTypeString().c_str());
			ImGui::PushItemWidth(ec.itemWidth3char);
			ImGui::Text("x:"); ImGui::SameLine();
			if (ImGui::InputInt("##x", &x, 0)) {
				isChanged = true;
			}
			ImGui::SameLine(); ImGui::Text("y:"); ImGui::SameLine();
			if (ImGui::InputInt("##y", &y, 0)) {
				isChanged = true;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(); ImGui::Text("time:"); ImGui::SameLine(); ImGui::SetNextItemWidth(ec.itemWidth7char + ec.itemWidthStep);
			if (ImGui::InputDouble("##time", &timeSec, step, 2.0 * step, "%.3f")) {
				isChanged = true;
			}
		}
		break;
		case NoteType::STREAM_NODE:
		{
			int index;
			floatX = prevFloatX = note.getFloatX();
			floatY = prevFloatY = note.getFloatY();
			timeSec = prevTimeSec = note.getTime();
			index = note.getIndex();

			ImGui::Text(note.getTypeString().c_str());
			ImGui::PushItemWidth(ec.itemWidth3char);
			ImGui::SameLine(); ImGui::Text("Index:"); ImGui::SameLine();
			ImGui::InputInt("##Index", &index, 0, 0, ImGuiInputTextFlags_ReadOnly);
			ImGui::PopItemWidth();
			ImGui::PushItemWidth(ec.itemWidth7char + ec.itemWidthStep);
			ImGui::Text("x:"); ImGui::SameLine();
			if (ImGui::InputFloat("##x", &floatX, 1.0f, 2.0f, "%.3f")) {
				isChanged = true;
			}
			ImGui::SameLine(); ImGui::Text("y:"); ImGui::SameLine();
			if (ImGui::InputFloat("##y", &floatY, 1.0f, 2.0f, "%.3f")) {
				isChanged = true;
			}

			ImGui::SameLine(); ImGui::Text("time:"); ImGui::SameLine();
			if (ImGui::InputDouble("##time", &timeSec, step, 2.0 * step, "%.3f")) {
				isChanged = true;
			}
			ImGui::PopItemWidth();
		}
		break;
		case NoteType::LONG_LEFT_FOOT:
		case NoteType::LONG_RIGHT_FOOT:

		{
			int index;
			uint64_t groupID;
			x = prevX = note.getX();
			y = prevY = note.getY();
			timeSec = prevTimeSec = note.getTime();
			index = note.getIndex();
			groupID = note.getGroupID();

			ImGui::Text(note.getTypeString().c_str());
			ImGui::PushItemWidth(ec.itemWidth3char);
			ImGui::SameLine(); ImGui::Text("Group ID:"); ImGui::SameLine();
			ImGui::InputScalar("##Group_ID", ImGuiDataType_U64, &groupID, nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine(); ImGui::Text("Index:"); ImGui::SameLine();
			ImGui::InputInt("##Index", &index, 0, 0, ImGuiInputTextFlags_ReadOnly);
			ImGui::Text("x:"); ImGui::SameLine();
			if (ImGui::InputInt("##x", &x, 0)) {
				isChanged = true;
			}
			ImGui::SameLine(); ImGui::Text("y:"); ImGui::SameLine();
			if (ImGui::InputInt("##y", &y, 0)) {
				isChanged = true;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(); ImGui::Text("time:"); ImGui::SameLine(); ImGui::SetNextItemWidth(ec.itemWidth7char + ec.itemWidthStep);;
			if (ImGui::InputDouble("##time", &timeSec, step, 2.0 * step, "%.3f")) {
				isChanged = true;
			}


		}
		break;
		case NoteType::OBS:
		case NoteType::OBS_END:

		{
			int index;
			uint64_t groupID;
			x = prevX = note.getX();
			y = prevY = note.getY();
			timeSec = prevTimeSec = note.getTime();
			index = note.getIndex();
			groupID = note.getGroupID();
			duration = note.getDuration();

			ImGui::Text(note.getTypeString().c_str());
			ImGui::PushItemWidth(ec.itemWidth3char);
			ImGui::SameLine(); ImGui::Text("Group ID:"); ImGui::SameLine();
			ImGui::InputScalar("##Group_ID", ImGuiDataType_U64, &groupID, nullptr, nullptr, nullptr, ImGuiInputTextFlags_ReadOnly);
			ImGui::SameLine(); ImGui::Text("Index:"); ImGui::SameLine();
			ImGui::InputInt("##Index", &index, 0, 0, ImGuiInputTextFlags_ReadOnly);
			ImGui::Text("x:"); ImGui::SameLine();
			ImGuiInputTextFlags obstaclePositionFlags = type == NoteType::OBS_END ? ImGuiInputTextFlags_ReadOnly : ImGuiInputTextFlags_None;
			if (ImGui::InputInt("##x", &x, 0, 0, obstaclePositionFlags)) {
				isChanged = true;
			}
			ImGui::SameLine(); ImGui::Text("y:"); ImGui::SameLine();
			if (ImGui::InputInt("##y", &y, 0, 0, obstaclePositionFlags)) {
				isChanged = true;
			}
			ImGui::PopItemWidth();
			ImGui::SameLine(); ImGui::Text("time:"); ImGui::SameLine(); ImGui::SetNextItemWidth(ec.itemWidth7char + ec.itemWidthStep);
			if (ImGui::InputDouble("##time", &timeSec, step, 2.0 * step, "%.3f")) {
				isChanged = true;
			}
			ImGui::SameLine(); ImGui::Text("duration:"); ImGui::SameLine(); ImGui::SetNextItemWidth(ec.itemWidth7char);
			if (ImGui::InputDouble("##duration", &duration, step, 2.0 * step, "%.3f s")) {
				durationChanged = true;
			}
		}
		break;
		default:
			break;
		}

		bool changed = false;
		if (durationChanged) {
			changed = editor.setObstacleDuration(note.getGroupID(), ec, beatmap, beatmapStack, duration) || changed;
		}
		if (isChanged) {
			changed = editor.changeNoteByDelta(noteID, ec, beatmap, beatmapStack, floatX - prevFloatX, floatY - prevFloatY, x - prevX, y - prevY, timeSec - prevTimeSec) || changed;
		}
		return changed;
	}

}
