#pragma once

#include <string>
#include <vector>

namespace NativeFileDialog {

struct FolderDialogResult {
	bool selected = false;
	std::string path;
	std::string errorMessage;
};

struct FileDialogResult {
	bool selected = false;
	std::string path;
	std::string errorMessage;
};

struct FileTypeFilter {
	std::string name;
	std::string pattern;
};

bool initializeComForCurrentThread(std::string& errorMessage);
void shutdownComForCurrentThread();

FolderDialogResult selectFolder(
	void* ownerWindow,
	const std::string& title,
	const std::string& initialPath
);

FileDialogResult selectFile(
	void* ownerWindow,
	const std::string& title,
	const std::string& initialPath,
	const std::vector<FileTypeFilter>& filters
);

}
