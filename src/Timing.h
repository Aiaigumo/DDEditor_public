#pragma once

// Stores one BPM timing segment and tracks beat progression for playback.
class Timing {
public:
	Timing(float bpm = 120.0f, double offset = 0.0);

	void setBPM(float bpm);
	float getBPM() const;

	void setOffset(double offset);
	// use for presenting or exporting
	double getOffset() const;
	// use for calc in this application
	double getActualOffset() const;
	void setOffsetInEditor(double offset);
	double getOffsetInEditor() const;
	double secPerBeat() const;
	int getSubbeat() const;
	void setSubbeat(int subbeat);
	int getCurrentBeat() const;
	double calcCurrentTime(int beat) const;

	/**
	* @brief update current beat
	* @return return true, when the currentbeat changes.
	* return false, otherwise.
	*/
	bool checkNextBeat(double currentTime);
	void resetCurrentBeat();

private:
	float bpm;
	double offset;
	int currentBeat;
	int subbeat;
	inline static constexpr int SUBBEAT_MIN = 1;
	static double offsetInEditor;
};
