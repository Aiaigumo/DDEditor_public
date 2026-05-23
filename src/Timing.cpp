#include "Timing.h"
#include <iostream>

double Timing::offsetInEditor = 0.0;

Timing::Timing(float bpm_, double offset_)
	: bpm(bpm_), offset(offset_)
{
	if (bpm_ <= 0.0f) {
		std::cerr << "[Timing.cpp] Timing::Timing: BPM must be greater than 0." << std::endl;
		bpm = 120.0f;
	}
	subbeat = SUBBEAT_MIN;
	resetCurrentBeat();
}

void Timing::setBPM(float bpm_) {
	if (bpm_ <= 0.0f) {
		std::cerr << "[Timing.cpp] Timing::setBPM: BPM must be greater than 0." << std::endl;
		return;
	}
	bpm = bpm_;
	resetCurrentBeat();
}

float Timing::getBPM() const {
	return bpm;
}

void Timing::setOffset(double offset_) {
	offset = offset_;
	resetCurrentBeat();
	return;
}

//use for presenting or exporting
double Timing::getOffset() const {
	return offset;
}

// use for calc in this application
double Timing::getActualOffset() const {
	return (offset - offsetInEditor);
}

void Timing::setOffsetInEditor(double offsetIE_) {
	offsetInEditor = offsetIE_;
}

double Timing::getOffsetInEditor() const {
	return offsetInEditor;
}

double Timing::secPerBeat() const {
	return 60.0 / static_cast<double>(getBPM());
}

int Timing::getSubbeat() const {
	return subbeat;
}

void Timing::setSubbeat(int subbeat_) {
	if (subbeat_ < SUBBEAT_MIN) {
		std::cerr << "[Timing.cpp] Timing::setSubbeat: subbeat must be at least " << SUBBEAT_MIN << std::endl;
		subbeat = SUBBEAT_MIN;
		resetCurrentBeat();
		return;
	}
	subbeat = subbeat_;
	resetCurrentBeat();
	return;
}

void Timing::resetCurrentBeat() {
	currentBeat = -1;
}

int Timing::getCurrentBeat() const {
	return currentBeat;
}

double Timing::calcCurrentTime(int beat) const {
	double timeSinceOffset = static_cast<double>(beat) * secPerBeat() / subbeat;
	return timeSinceOffset + getActualOffset();
}

bool Timing::checkNextBeat(double currentTime) {
	double EPSILON = 1e-9;		//small value to avoid floating point precision issues
	double timeSinceOffset = currentTime - getActualOffset();
	int beat = static_cast<int>(std::floor((static_cast<double>(subbeat) * timeSinceOffset / secPerBeat()) + EPSILON));

	//befort beat 0, no metronome
	if (currentBeat <= -2) {
		currentBeat = beat;
		return false;
	}

	if (beat > currentBeat) {
		currentBeat = beat;
		return true;
	}
	currentBeat = beat;
	return false;
}
