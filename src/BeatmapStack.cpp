#include "BeatmapStack.h"

void BeatmapStack::push(const Beatmap& beatmap) {
	if (undoStack.size() >= MAX_STACK_SIZE) {
		undoStack.erase(undoStack.begin()); // remove the oldest state
	}
	undoStack.push_back(beatmap);
}

bool BeatmapStack::canUndo() const {
	return !undoStack.empty();
}

void BeatmapStack::undo(Beatmap& beatmap) {
	if (!canUndo()) return;
	beatmap = undoStack.back();
	undoStack.pop_back();
}

void BeatmapStack::clear() {
	undoStack.clear();
}