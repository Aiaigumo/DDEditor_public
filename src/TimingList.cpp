#include "TimingList.h"
#include <stdexcept>
#include <algorithm>
#include <iostream>

TimingList::TimingList() {
	addTiming(Timing());    //at first has one timing
	update(0.0);           //initialize currentTimingIndex
}

void TimingList::addTiming(const Timing& timing) {
	timings.push_back(timing);
	sortTimingsByOffset();
}

void TimingList::removeTiming(size_t index)
{
	if (index >= timings.size()) throw std::out_of_range("TimingList::removeTiming: index out of range");
	if (index == 0) { std::cerr << "TimingList::removeTiming: cannot remove the first timing" << std::endl; return; } //cannot remove first timing
	timings.erase(timings.begin() + index);
}

Timing& TimingList::getTiming(size_t index)
{
	if (index >= timings.size()) throw std::out_of_range("TimingList::getTiming: index out of range");
	return timings.at(index);
}

const Timing& TimingList::getTiming(size_t index) const
{
	if (index >= timings.size()) throw std::out_of_range("TimingList::getTiming: index out of range");
	return timings.at(index);
}

Timing& TimingList::getCurrentTiming(double currentTime)
{
	size_t index = calcCurrentTimingIndex(currentTime);
	if (index >= timings.size()) throw std::out_of_range("TimingList::getCurrentTiming: index out of range");
	return timings.at(index);
}

const Timing& TimingList::getCurrentTiming(double currentTime) const
{
	size_t index = calcCurrentTimingIndex(currentTime);
	if (index >= timings.size()) throw std::out_of_range("TimingList::getCurrentTiming: index out of range");
	return timings.at(index);
}

size_t TimingList::size() const
{
	return timings.size();
}

void TimingList::sortTimingsByOffset()
{
	std::sort(timings.begin(), timings.end(), [](const Timing& a, const Timing& b) {
		return a.getOffset() < b.getOffset();
		});
}

size_t TimingList::calcCurrentTimingIndex(double currentTime) const
{
	size_t index = 0;
	for (size_t i = 0; i < timings.size(); ++i) {
		if (currentTime >= timings.at(i).getActualOffset()) {
			index = i;
		}
		else {
			break;
		}
	}
	return index;
}

bool TimingList::isEmpty() const {
	return size() < 1;
}

bool TimingList::update(double currentTime) {
	size_t calcIndex = calcCurrentTimingIndex(currentTime);
	if (calcIndex != currentTimingIndex) {
		currentTimingIndex = calcIndex;
		resetCurrentBeat();
		return true;
	}
	return false;
}

size_t TimingList::getCurrentIndex() const {
	return currentTimingIndex;
}

void TimingList::setBPM(float bpm) {
	timings.at(currentTimingIndex).setBPM(bpm);
	return;
}

void TimingList::setBPM(float bpm, size_t index) {
	if (index >= timings.size()) throw std::out_of_range("TimingList::setBPM: index out of range");
	timings.at(index).setBPM(bpm);
	return;
}

float TimingList::getBPM() const {
	return timings.at(currentTimingIndex).getBPM();
}

float TimingList::getBPM(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getBPM: index out of range");
	return timings.at(index).getBPM();
}

float TimingList::getFirstBPM() const {
	return timings.at(0).getBPM();
}

void TimingList::setOffset(double offset) {
	setOffset(offset, currentTimingIndex);
	return;
}

void TimingList::setOffset(double offset, size_t index) {
	if (index >= timings.size()) throw std::out_of_range("TimingList::setOffset: index out of range");
	timings.at(index).setOffset(offset);
	sortTimingsByOffset();
	return;
}

double TimingList::getOffset() const {
	return timings.at(currentTimingIndex).getOffset();
}

double TimingList::getOffset(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getOffset: index out of range");
	return timings.at(index).getOffset();
}

double TimingList::getFirstOffset() const {
	return timings.at(0).getOffset();
}

double TimingList::getActualOffset() const {
	return timings.at(currentTimingIndex).getActualOffset();
}

double TimingList::getActualOffset(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getActualOffset: index out of range");
	return timings.at(index).getActualOffset();
}

double TimingList::getActualFirstOffset() const {
	return getActualOffset(0);
}

double TimingList::getOffsetInNoteTime() const {
	return getOffsetInNoteTime(currentTimingIndex);
}

double TimingList::getOffsetInNoteTime(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getOffsetInNoteTime: index out of range");
	if (index == 0) {
		return 0.0;
	}
	else {
		return  timings.at(index).getActualOffset() - timings.at(0).getActualOffset();
	}
}

void TimingList::setOffsetInEditor(double offset) {
	timings.at(currentTimingIndex).setOffsetInEditor(offset);
	return;
}

double TimingList::getOffsetInEditor() const {
	return timings.at(currentTimingIndex).getOffsetInEditor();
}

double TimingList::secPerBeat() const {
	return timings.at(currentTimingIndex).secPerBeat();
}

double TimingList::secPerBeat(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getOffsetInNoteTime: index out of range");
	return timings.at(index).secPerBeat();
}

double TimingList::firstSecPerBeat() const {
	return timings.at(0).secPerBeat();
}

int TimingList::getSubbeat() const {
	return getSubbeat(currentTimingIndex);
}

int TimingList::getSubbeat(size_t index) const {
	if (index >= timings.size()) throw std::out_of_range("TimingList::getSubbeat: index out of range");
	return timings.at(index).getSubbeat();
}

void TimingList::setSubbeat(int subbeat) {
	setSubbeat(subbeat, currentTimingIndex);
	return;
}

void TimingList::setSubbeat(int subbeat, size_t index) {
	if (index >= timings.size()) throw std::out_of_range("TimingList::setSubbeat: index out of range");
	timings.at(index).setSubbeat(subbeat);
	return;
}


int TimingList::getCurrentBeat() const {
	return timings.at(currentTimingIndex).getCurrentBeat();
}

double TimingList::calcCurrentTime(int beat) const {
	return timings.at(currentTimingIndex).calcCurrentTime(beat);
}

bool TimingList::checkNextBeat(double currentTime) {
	return timings.at(currentTimingIndex).checkNextBeat(currentTime);
}

void TimingList::resetCurrentBeat() {
	for (size_t i = 0; i < timings.size(); ++i) {
		timings.at(i).resetCurrentBeat();
	}
	return;
}
