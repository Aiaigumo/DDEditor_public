#pragma once

#include <cstddef>
#include <vector>

enum class SegmentMode {
	Linear,
	CatmullRom
};

enum class CatmullRomType {
	Centripetal
};

enum class AxisSampleCrossingMode {
	All,
	First,
	Last
};

template <typename T>
// Owns ordered curve nodes and sampling rules for linear or Catmull-Rom path evaluation.
class CurvePath {
public:
	CurvePath() = default;
	~CurvePath() = default;

	void clear();
	void addNode(const T& point);
	bool removeNode(size_t nodeIndex);
	T& getNode(size_t nodeIndex);
	const T& getNode(size_t nodeIndex) const;
	void setNode(size_t nodeIndex, const T& point);
	void changeNode(size_t nodeIndex, const T& delta);
	size_t nodeCount() const;

	void setDefaultNodeMode(SegmentMode mode);
	void setAllNodeMode(SegmentMode mode);
	void setNodeMode(size_t nodeIndex, SegmentMode mode);
	SegmentMode getNodeMode(size_t nodeIndex) const;

	// Predicate format:
	// SegmentMode(const T& prev, const T& current, const T& next, size_t nodeIndex)
	// For boundary nodes, prev/next are clamped to the edge nodes.
	template <typename Predicate>
	void applyNodeModeRule(Predicate&& predicate);

	T evaluate(size_t segmentIndex, float u) const;
	std::vector<T> sampleBySegment(size_t segmentIndex, int divisions) const;
	std::vector<T> sampleWholePath(int divisionsPerSegment) const;
	std::vector<T> sampleWholePathByArcLength(float step, int sampleDivision = 32) const;
	// AxisGetter format: float(const T& point)
	// When the path crosses the same axis target multiple times, crossingMode controls
	// whether all, the first, or the last crossing is used.
	template <typename AxisGetter>
	std::vector<T> sampleWholePathByAxis(
		float step,
		const AxisGetter& axis,
		int sampleDivision = 32,
		AxisSampleCrossingMode crossingMode = AxisSampleCrossingMode::All) const;

private:
	SegmentMode getSegmentModeByNodes(size_t segmentIndex) const;
	T evaluateLinear(size_t segmentIndex, float u) const;
	T evaluateCatmullRom(size_t segmentIndex, float u) const;
	void syncNodeModes();

	std::vector<T> nodes_{};
	std::vector<SegmentMode> nodeModes_{};
	SegmentMode defaultNodeMode_ = SegmentMode::CatmullRom;
	CatmullRomType catmullRomType_ = CatmullRomType::Centripetal;
};

#include "CurvePath.inl"
