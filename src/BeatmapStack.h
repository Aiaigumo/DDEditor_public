#pragma once
#include "Beatmap.h"
#include <vector>

// Stores previous Beatmap snapshots so editor operations can restore undo states.
class BeatmapStack {
public:
	BeatmapStack() = default;
	~BeatmapStack() = default;

	//record current beatmap
	void push(const Beatmap&);

	bool canUndo() const;

	// restore recoded state
	void undo(Beatmap&);

	// clear stack
	void clear();
private:
	std::vector<Beatmap> undoStack;
	inline static constexpr size_t MAX_STACK_SIZE = 256;
};
