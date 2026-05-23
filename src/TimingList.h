#pragma once
#include <vector>
#include "Timing.h"

// Owns ordered Timing entries and selects the active timing for the current playback time.
class TimingList {
public:
	TimingList();
	~TimingList() = default;

	// Add a new Timing
	void addTiming(const Timing& timing);

	// Remove a Timing by index
	void removeTiming(size_t index);

	// Access a Timing
	Timing& getTiming(size_t index);
	const Timing& getTiming(size_t index) const;

	Timing& getCurrentTiming(double currentTime);
	const Timing& getCurrentTiming(double currentTime) const;

	// Get number of timings
	size_t size() const;

	bool isEmpty() const;

	// Update currentTimingIndex
	bool update(double currentTime);
	size_t getCurrentIndex() const;

	void setBPM(float bpm);
	void setBPM(float bpm, size_t index);
	float getBPM() const;
	float getBPM(size_t index) const;
	float getFirstBPM() const;

	void setOffset(double offset);
	void setOffset(double offset, size_t index);
	double getOffset() const;
	double getOffset(size_t index) const;
	double getFirstOffset() const;
	double getActualOffset() const;
	double getActualOffset(size_t index) const;
	double getActualFirstOffset() const;
	double getOffsetInNoteTime() const;
	double getOffsetInNoteTime(size_t index) const;
	void setOffsetInEditor(double offset);
	double getOffsetInEditor() const;

	double secPerBeat() const;
	double secPerBeat(size_t index) const;
	double firstSecPerBeat() const;

	int getSubbeat() const;
	int getSubbeat(size_t index) const;
	void setSubbeat(int subbeat);
	void setSubbeat(int subbeat, size_t index);

	int getCurrentBeat() const;
	double calcCurrentTime(int beat) const;
	bool checkNextBeat(double currentTime);

	void resetCurrentBeat();

private:
	void sortTimingsByOffset();
	size_t calcCurrentTimingIndex(double currentTime) const;
	std::vector<Timing> timings;
	size_t currentTimingIndex;
};
