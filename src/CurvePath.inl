#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <glm.hpp>

namespace CurvePathDetail {
inline constexpr float TIME_EPSILON = 1e-6f;

template <typename T>
inline T lerp(const T& a, const T& b, float t) {
	return a + (b - a) * t;
}

template <typename T>
inline float pointDistance(const T& a, const T& b) {
	return glm::distance(a, b);
}

template <typename T>
inline T interpolateByTime(const T& a, const T& b, float ta, float tb, float t) {
	const float denom = tb - ta;
	if (std::fabs(denom) <= TIME_EPSILON) {
		return a;
	}
	const float wa = (tb - t) / denom;
	const float wb = (t - ta) / denom;
	return a * wa + b * wb;
}

template <typename TCurvePath, typename TPoint>
inline std::vector<TPoint> buildApproximatedPath(const TCurvePath& curvePath, int sampleDivision) {
	const int clampedSampleDivision = std::max(sampleDivision, 1);
	std::vector<TPoint> approximatedPath{};
	approximatedPath.reserve((curvePath.nodeCount() - 1) * static_cast<size_t>(clampedSampleDivision) + 1);
	approximatedPath.push_back(curvePath.getNode(0));
	for (size_t i = 0; i + 1 < curvePath.nodeCount(); ++i) {
		for (int j = 1; j <= clampedSampleDivision; ++j) {
			const float u = static_cast<float>(j) / static_cast<float>(clampedSampleDivision);
			approximatedPath.push_back(curvePath.evaluate(i, u));
		}
	}
	return approximatedPath;
}

template <typename T>
inline void appendIfDistinct(std::vector<T>& points, const T& point) {
	if (points.empty() || pointDistance(points.back(), point) > TIME_EPSILON) {
		points.push_back(point);
	}
}
} // namespace CurvePathDetail

template <typename T>
void CurvePath<T>::clear() {
	nodes_.clear();
	nodeModes_.clear();
}

template <typename T>
void CurvePath<T>::addNode(const T& point) {
	nodes_.push_back(point);
	syncNodeModes();
}

template <typename T>
bool CurvePath<T>::removeNode(size_t nodeIndex) {
	nodes_.at(nodeIndex);
	nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(nodeIndex));
	syncNodeModes();
	return nodes_.size() < 2;
}

template <typename T>
T& CurvePath<T>::getNode(size_t nodeIndex) {
	return nodes_.at(nodeIndex);
}

template <typename T>
const T& CurvePath<T>::getNode(size_t nodeIndex) const {
	return nodes_.at(nodeIndex);
}

template <typename T>
void CurvePath<T>::setNode(size_t nodeIndex, const T& point) {
	nodes_.at(nodeIndex) = point;
}

template <typename T>
void CurvePath<T>::changeNode(size_t nodeIndex, const T& delta) {
	nodes_.at(nodeIndex) += delta;
}

template <typename T>
size_t CurvePath<T>::nodeCount() const {
	return nodes_.size();
}

template <typename T>
void CurvePath<T>::setDefaultNodeMode(SegmentMode mode) {
	defaultNodeMode_ = mode;
}

template <typename T>
void CurvePath<T>::setAllNodeMode(SegmentMode mode) {
	defaultNodeMode_ = mode;
	for (size_t i = 0; i < nodeModes_.size(); ++i) {
		nodeModes_.at(i) = mode;
	}
}

template <typename T>
void CurvePath<T>::setNodeMode(size_t nodeIndex, SegmentMode mode) {
	nodeModes_.at(nodeIndex) = mode;
}

template <typename T>
SegmentMode CurvePath<T>::getNodeMode(size_t nodeIndex) const {
	return nodeModes_.at(nodeIndex);
}

template <typename T>
template <typename Predicate>
void CurvePath<T>::applyNodeModeRule(Predicate&& predicate) {
	if (nodes_.empty()) {
		return;
	}

	for (size_t i = 0; i < nodes_.size(); ++i) {
		const size_t i0 = (i == 0) ? 0 : i - 1;
		const size_t i1 = i;
		const size_t i2 = (i + 1 < nodes_.size()) ? i + 1 : nodes_.size() - 1;
		nodeModes_.at(i) = predicate(nodes_.at(i0), nodes_.at(i1), nodes_.at(i2), i);
	}
}

template <typename T>
SegmentMode CurvePath<T>::getSegmentModeByNodes(size_t segmentIndex) const {
	if (nodes_.size() < 2) {
		throw std::out_of_range("CurvePath::getSegmentModeByNodes: no segments");
	}
	if (segmentIndex >= nodes_.size() - 1) {
		throw std::out_of_range("CurvePath::getSegmentModeByNodes: segmentIndex out of range");
	}
	const SegmentMode start = getNodeMode(segmentIndex);
	const SegmentMode end = getNodeMode(segmentIndex + 1);
	if (start == SegmentMode::Linear && end == SegmentMode::Linear) {
		return SegmentMode::Linear;
	}
	return SegmentMode::CatmullRom;
}

template <typename T>
T CurvePath<T>::evaluate(size_t segmentIndex, float u) const {
	if (nodes_.size() < 2) {
		throw std::out_of_range("CurvePath::evaluate: no segments");
	}

	if (segmentIndex >= nodes_.size() - 1) {
		throw std::out_of_range("CurvePath::evaluate: segmentIndex out of range");
	}

	const float clampedU = std::clamp(u, 0.0f, 1.0f);
	const SegmentMode mode = getSegmentModeByNodes(segmentIndex);
	if (mode == SegmentMode::Linear) {
		return evaluateLinear(segmentIndex, clampedU);
	}
	return evaluateCatmullRom(segmentIndex, clampedU);
}

template <typename T>
std::vector<T> CurvePath<T>::sampleBySegment(size_t segmentIndex, int divisions) const {
	if (nodes_.size() < 2 || segmentIndex >= nodes_.size() - 1) {
		throw std::out_of_range("CurvePath::sampleBySegment: segmentIndex out of range");
	}

	std::vector<T> points{};
	const int clampedDivisions = std::max(divisions, 1);
	points.reserve(static_cast<size_t>(clampedDivisions) + 1);
	for (int i = 0; i <= clampedDivisions; ++i) {
		const float u = static_cast<float>(i) / static_cast<float>(clampedDivisions);
		points.push_back(evaluate(segmentIndex, u));
	}
	return points;
}

template <typename T>
std::vector<T> CurvePath<T>::sampleWholePath(int divisionsPerSegment) const {
	std::vector<T> sampledPoints{};
	if (nodes_.empty()) {
		return sampledPoints;
	}

	if (nodes_.size() == 1) {
		sampledPoints.push_back(nodes_.front());
		return sampledPoints;
	}

	for (size_t i = 0; i + 1 < nodes_.size(); ++i) {
		std::vector<T> segmentPoints = sampleBySegment(i, divisionsPerSegment);
		if (i > 0 && !segmentPoints.empty()) {
			segmentPoints.erase(segmentPoints.begin());
		}
		sampledPoints.insert(sampledPoints.end(), segmentPoints.begin(), segmentPoints.end());
	}
	return sampledPoints;
}

template <typename T>
std::vector<T> CurvePath<T>::sampleWholePathByArcLength(float step, int sampleDivision) const {
	std::vector<T> sampledPoints{};
	if (nodes_.empty()) {
		return sampledPoints;
	}

	if (nodes_.size() == 1) {
		sampledPoints.push_back(nodes_.front());
		return sampledPoints;
	}

	step = std::fabs(step);
	if (step <= CurvePathDetail::TIME_EPSILON) {
		return sampledPoints;
	}

	std::vector<T> approximatedPath = CurvePathDetail::buildApproximatedPath<CurvePath<T>, T>(*this, sampleDivision);

	sampledPoints.push_back(approximatedPath.front());

	float traversed = 0.0f;
	float nextSampleDistance = step;
	for (size_t i = 1; i < approximatedPath.size(); ++i) {
		const T& previous = approximatedPath.at(i - 1);
		const T& current = approximatedPath.at(i);
		const float segmentLength = CurvePathDetail::pointDistance(previous, current);
		if (segmentLength <= CurvePathDetail::TIME_EPSILON) {
			continue;
		}

		while (traversed + segmentLength >= nextSampleDistance) {
			const float localDistance = nextSampleDistance - traversed;
			const float t = localDistance / segmentLength;
			sampledPoints.push_back(CurvePathDetail::lerp(previous, current, t));
			nextSampleDistance += step;
		}

		traversed += segmentLength;
	}

	CurvePathDetail::appendIfDistinct(sampledPoints, approximatedPath.back());

	return sampledPoints;
}

template <typename T>
template <typename AxisGetter>
std::vector<T> CurvePath<T>::sampleWholePathByAxis(
	float step,
	const AxisGetter& axis,
	int sampleDivision,
	AxisSampleCrossingMode crossingMode) const {
	std::vector<T> sampledPoints{};
	if (nodes_.empty()) {
		return sampledPoints;
	}

	if (nodes_.size() == 1) {
		sampledPoints.push_back(nodes_.front());
		return sampledPoints;
	}

	step = std::fabs(step);
	if (step <= CurvePathDetail::TIME_EPSILON) {
		return sampledPoints;
	}

	const std::vector<T> approximatedPath = CurvePathDetail::buildApproximatedPath<CurvePath<T>, T>(*this, sampleDivision);
	sampledPoints.push_back(approximatedPath.front());

	const float startAxis = static_cast<float>(axis(approximatedPath.front()));
	const float endAxis = static_cast<float>(axis(approximatedPath.back()));
	const float direction = (endAxis >= startAxis) ? 1.0f : -1.0f;
	const float signedStartAxis = startAxis * direction;
	const float signedEndAxis = endAxis * direction;
	const float totalAxisSpan = signedEndAxis - signedStartAxis;
	if (totalAxisSpan <= CurvePathDetail::TIME_EPSILON) {
		CurvePathDetail::appendIfDistinct(sampledPoints, approximatedPath.back());
		return sampledPoints;
	}

	const size_t targetCount = static_cast<size_t>(
		std::floor((totalAxisSpan + CurvePathDetail::TIME_EPSILON) / step));
	if (targetCount == 0) {
		CurvePathDetail::appendIfDistinct(sampledPoints, approximatedPath.back());
		return sampledPoints;
	}

	std::vector<T> deferredSamples{};
	std::vector<bool> hasDeferredSample{};
	if (crossingMode != AxisSampleCrossingMode::All) {
		deferredSamples.assign(targetCount, approximatedPath.front());
		hasDeferredSample.assign(targetCount, false);
	}

	for (size_t i = 1; i < approximatedPath.size(); ++i) {
		const T& previous = approximatedPath.at(i - 1);
		const T& current = approximatedPath.at(i);
		const float previousAxis = static_cast<float>(axis(previous));
		const float currentAxis = static_cast<float>(axis(current));
		const float axisDelta = currentAxis - previousAxis;
		if (std::fabs(axisDelta) <= CurvePathDetail::TIME_EPSILON) {
			continue;
		}

		const float signedPreviousAxis = previousAxis * direction;
		const float signedCurrentAxis = currentAxis * direction;
		const float minSignedAxis = (std::min)(signedPreviousAxis, signedCurrentAxis);
		const float maxSignedAxis = (std::max)(signedPreviousAxis, signedCurrentAxis);

		int firstTargetIndex = static_cast<int>(
			std::ceil((minSignedAxis - signedStartAxis - CurvePathDetail::TIME_EPSILON) / step));
		int lastTargetIndex = static_cast<int>(
			std::floor((maxSignedAxis - signedStartAxis + CurvePathDetail::TIME_EPSILON) / step));
		firstTargetIndex = (std::max)(firstTargetIndex, 1);
		lastTargetIndex = (std::min)(lastTargetIndex, static_cast<int>(targetCount));
		if (firstTargetIndex > lastTargetIndex) {
			continue;
		}

		const int targetIndexStep = (signedCurrentAxis >= signedPreviousAxis) ? 1 : -1;
		const int targetIndexBegin = (targetIndexStep > 0) ? firstTargetIndex : lastTargetIndex;
		const int targetIndexEnd = (targetIndexStep > 0) ? lastTargetIndex : firstTargetIndex;
		for (int targetIndex = targetIndexBegin;; targetIndex += targetIndexStep) {
			const float signedTargetAxis = signedStartAxis + static_cast<float>(targetIndex) * step;
			const float targetAxis = signedTargetAxis * direction;
			const float t = (targetAxis - previousAxis) / axisDelta;
			const float clampedT = std::clamp(t, 0.0f, 1.0f);
			const T sampledPoint = CurvePathDetail::lerp(previous, current, clampedT);

			if (crossingMode == AxisSampleCrossingMode::All) {
				CurvePathDetail::appendIfDistinct(sampledPoints, sampledPoint);
			}
			else {
				const size_t storedIndex = static_cast<size_t>(targetIndex - 1);
				if (crossingMode == AxisSampleCrossingMode::First) {
					if (!hasDeferredSample.at(storedIndex)) {
						deferredSamples.at(storedIndex) = sampledPoint;
						hasDeferredSample.at(storedIndex) = true;
					}
				}
				else {
					deferredSamples.at(storedIndex) = sampledPoint;
					hasDeferredSample.at(storedIndex) = true;
				}
			}

			if (targetIndex == targetIndexEnd) {
				break;
			}
		}
	}

	if (crossingMode != AxisSampleCrossingMode::All) {
		for (size_t i = 0; i < deferredSamples.size(); ++i) {
			if (!hasDeferredSample.at(i)) {
				continue;
			}
			CurvePathDetail::appendIfDistinct(sampledPoints, deferredSamples.at(i));
		}
	}

	CurvePathDetail::appendIfDistinct(sampledPoints, approximatedPath.back());
	return sampledPoints;
}

template <typename T>
T CurvePath<T>::evaluateLinear(size_t segmentIndex, float u) const {
	return CurvePathDetail::lerp(nodes_.at(segmentIndex), nodes_.at(segmentIndex + 1), u);
}

template <typename T>
T CurvePath<T>::evaluateCatmullRom(size_t segmentIndex, float u) const {
	if (catmullRomType_ != CatmullRomType::Centripetal || nodes_.size() < 2) {
		return evaluateLinear(segmentIndex, u);
	}

	const size_t i0 = (segmentIndex == 0) ? 0 : segmentIndex - 1;
	const size_t i1 = segmentIndex;
	const size_t i2 = segmentIndex + 1;
	const size_t i3 = (segmentIndex + 2 < nodes_.size()) ? segmentIndex + 2 : nodes_.size() - 1;
	const SegmentMode startMode = getNodeMode(i1);
	const SegmentMode endMode = getNodeMode(i2);
	const size_t p0Index = (startMode == SegmentMode::Linear) ? i1 : i0;
	const size_t p3Index = (endMode == SegmentMode::Linear) ? i2 : i3;
	const T& p0 = nodes_.at(p0Index);
	const T& p1 = nodes_.at(i1);
	const T& p2 = nodes_.at(i2);
	const T& p3 = nodes_.at(p3Index);

	const float alpha = 0.5f; // centripetal
	auto nextT = [alpha](float t, const T& a, const T& b) {
		const float dist = CurvePathDetail::pointDistance(a, b);
		if (dist <= CurvePathDetail::TIME_EPSILON) {
			return t + 1e-3f;
		}
		return t + std::pow(dist, alpha);
	};

	const float t0 = 0.0f;
	const float t1 = nextT(t0, p0, p1);
	const float t2 = nextT(t1, p1, p2);
	const float t3 = nextT(t2, p2, p3);

	if (std::fabs(t2 - t1) <= CurvePathDetail::TIME_EPSILON) {
		return evaluateLinear(segmentIndex, u);
	}

	const float t = t1 + (t2 - t1) * u;

	const T a1 = CurvePathDetail::interpolateByTime(p0, p1, t0, t1, t);
	const T a2 = CurvePathDetail::interpolateByTime(p1, p2, t1, t2, t);
	const T a3 = CurvePathDetail::interpolateByTime(p2, p3, t2, t3, t);
	const T b1 = CurvePathDetail::interpolateByTime(a1, a2, t0, t2, t);
	const T b2 = CurvePathDetail::interpolateByTime(a2, a3, t1, t3, t);
	return CurvePathDetail::interpolateByTime(b1, b2, t1, t2, t);
}

template <typename T>
void CurvePath<T>::syncNodeModes() {
	if (nodeModes_.size() < nodes_.size()) {
		const size_t before = nodeModes_.size();
		nodeModes_.resize(nodes_.size(), defaultNodeMode_);
		for (size_t i = before; i < nodeModes_.size(); ++i) {
			nodeModes_.at(i) = defaultNodeMode_;
		}
	}
	else if (nodeModes_.size() > nodes_.size()) {
		nodeModes_.resize(nodes_.size());
	}
}
