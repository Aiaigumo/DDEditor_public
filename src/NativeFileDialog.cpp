#include "NativeFileDialog.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shobjidl.h>

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <vector>

namespace {

std::wstring utf8ToWide(const std::string& value) {
	if (value.empty()) {
		return {};
	}

	const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
	if (size <= 0) {
		return {};
	}

	std::wstring result(static_cast<size_t>(size - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
	return result;
}

std::string wideToUtf8(const wchar_t* value) {
	if (value == nullptr || value[0] == L'\0') {
		return {};
	}

	const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
	if (size <= 0) {
		return {};
	}

	std::string result(static_cast<size_t>(size - 1), '\0');
	WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
	return result;
}

std::string hresultToMessage(HRESULT hr) {
	std::ostringstream stream;
	stream << "HRESULT 0x"
		<< std::uppercase << std::hex << std::setw(8) << std::setfill('0')
		<< static_cast<unsigned long>(static_cast<unsigned int>(hr));

	std::string message = stream.str();
	if (hr == RPC_E_CHANGED_MODE) {
		message += " (RPC_E_CHANGED_MODE: COM was already initialized with a different threading model)";
	}
	else if (hr == CO_E_NOTINITIALIZED) {
		message += " (CO_E_NOTINITIALIZED)";
	}
	else if (hr == E_OUTOFMEMORY) {
		message += " (E_OUTOFMEMORY)";
	}
	else if (hr == E_ACCESSDENIED) {
		message += " (E_ACCESSDENIED)";
	}
	return message;
}

// Manages per-call COM initialization for native dialog operations.
class ComApartment {
public:
	ComApartment() {
		hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		shouldUninitialize = SUCCEEDED(hr);
	}

	~ComApartment() {
		if (shouldUninitialize) {
			CoUninitialize();
		}
	}

	HRESULT result() const {
		return hr;
	}

private:
	HRESULT hr = E_FAIL;
	bool shouldUninitialize = false;
};

void releaseShellItem(IShellItem*& item) {
	if (item != nullptr) {
		item->Release();
		item = nullptr;
	}
}

void releaseFileOpenDialog(IFileOpenDialog*& dialog) {
	if (dialog != nullptr) {
		dialog->Release();
		dialog = nullptr;
	}
}

bool setInitialFolder(IFileOpenDialog* dialog, const std::string& initialPath) {
	if (dialog == nullptr || initialPath.empty()) {
		return false;
	}

	std::error_code fsError;
	std::filesystem::path initialFolder = std::filesystem::path(utf8ToWide(initialPath));
	if (std::filesystem::is_regular_file(initialFolder, fsError)) {
		initialFolder = initialFolder.parent_path();
		fsError.clear();
	}

	if (!std::filesystem::exists(initialFolder, fsError) || !std::filesystem::is_directory(initialFolder, fsError)) {
		return false;
	}

	IShellItem* folderItem = nullptr;
	const HRESULT hr = SHCreateItemFromParsingName(initialFolder.c_str(), nullptr, IID_PPV_ARGS(&folderItem));
	if (FAILED(hr)) {
		return false;
	}

	dialog->SetFolder(folderItem);
	releaseShellItem(folderItem);
	return true;
}

}
#endif

namespace NativeFileDialog {

bool initializeComForCurrentThread(std::string& errorMessage) {
#ifdef _WIN32
	const HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (FAILED(hr)) {
		errorMessage = "Failed to initialize Windows folder dialog COM apartment: " + hresultToMessage(hr);
		return false;
	}

	errorMessage.clear();
	return true;
#else
	(void)errorMessage;
	return false;
#endif
}

void shutdownComForCurrentThread() {
#ifdef _WIN32
	CoUninitialize();
#endif
}

FolderDialogResult selectFolder(
	void* ownerWindow,
	const std::string& title,
	const std::string& initialPath
) {
	FolderDialogResult result;

#ifdef _WIN32
	ComApartment comApartment;
	const HRESULT comHr = comApartment.result();
	if (FAILED(comHr)) {
		result.errorMessage = "Failed to initialize Windows folder dialog: " + hresultToMessage(comHr);
		return result;
	}

	IFileOpenDialog* dialog = nullptr;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&dialog)
	);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to create Windows folder dialog: " + hresultToMessage(hr);
		return result;
	}

	DWORD options = 0;
	hr = dialog->GetOptions(&options);
	if (SUCCEEDED(hr)) {
		hr = dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
	}
	if (FAILED(hr)) {
		result.errorMessage = "Failed to configure Windows folder dialog: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	const std::wstring titleWide = utf8ToWide(title);
	if (!titleWide.empty()) {
		dialog->SetTitle(titleWide.c_str());
	}

	setInitialFolder(dialog, initialPath);

	hr = dialog->Show(static_cast<HWND>(ownerWindow));
	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		releaseFileOpenDialog(dialog);
		return result;
	}
	if (FAILED(hr)) {
		result.errorMessage = "Windows folder dialog failed: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	IShellItem* selectedItem = nullptr;
	hr = dialog->GetResult(&selectedItem);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to read selected folder: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	PWSTR selectedPath = nullptr;
	hr = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to read selected folder path: " + hresultToMessage(hr);
		releaseShellItem(selectedItem);
		releaseFileOpenDialog(dialog);
		return result;
	}

	result.selected = true;
	result.path = wideToUtf8(selectedPath);

	CoTaskMemFree(selectedPath);
	releaseShellItem(selectedItem);
	releaseFileOpenDialog(dialog);
	return result;
#else
	(void)ownerWindow;
	(void)title;
	(void)initialPath;
	result.errorMessage = "Native folder selection is only supported on Windows.";
	return result;
#endif
}

FileDialogResult selectFile(
	void* ownerWindow,
	const std::string& title,
	const std::string& initialPath,
	const std::vector<FileTypeFilter>& filters
) {
	FileDialogResult result;

#ifdef _WIN32
	ComApartment comApartment;
	const HRESULT comHr = comApartment.result();
	if (FAILED(comHr)) {
		result.errorMessage = "Failed to initialize Windows file dialog: " + hresultToMessage(comHr);
		return result;
	}

	IFileOpenDialog* dialog = nullptr;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&dialog)
	);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to create Windows file dialog: " + hresultToMessage(hr);
		return result;
	}

	DWORD options = 0;
	hr = dialog->GetOptions(&options);
	if (SUCCEEDED(hr)) {
		hr = dialog->SetOptions(options | FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST);
	}
	if (FAILED(hr)) {
		result.errorMessage = "Failed to configure Windows file dialog: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	const std::wstring titleWide = utf8ToWide(title);
	if (!titleWide.empty()) {
		dialog->SetTitle(titleWide.c_str());
	}

	std::vector<std::wstring> filterNames;
	std::vector<std::wstring> filterPatterns;
	std::vector<COMDLG_FILTERSPEC> filterSpecs;
	filterNames.reserve(filters.size());
	filterPatterns.reserve(filters.size());
	filterSpecs.reserve(filters.size());
	for (const FileTypeFilter& filter : filters) {
		filterNames.push_back(utf8ToWide(filter.name));
		filterPatterns.push_back(utf8ToWide(filter.pattern));
		filterSpecs.push_back(COMDLG_FILTERSPEC{ filterNames.back().c_str(), filterPatterns.back().c_str() });
	}
	if (!filterSpecs.empty()) {
		hr = dialog->SetFileTypes(static_cast<UINT>(filterSpecs.size()), filterSpecs.data());
		if (FAILED(hr)) {
			result.errorMessage = "Failed to configure Windows file dialog filters: " + hresultToMessage(hr);
			releaseFileOpenDialog(dialog);
			return result;
		}
		dialog->SetFileTypeIndex(1);
	}

	setInitialFolder(dialog, initialPath);

	hr = dialog->Show(static_cast<HWND>(ownerWindow));
	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		releaseFileOpenDialog(dialog);
		return result;
	}
	if (FAILED(hr)) {
		result.errorMessage = "Windows file dialog failed: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	IShellItem* selectedItem = nullptr;
	hr = dialog->GetResult(&selectedItem);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to read selected file: " + hresultToMessage(hr);
		releaseFileOpenDialog(dialog);
		return result;
	}

	PWSTR selectedPath = nullptr;
	hr = selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &selectedPath);
	if (FAILED(hr)) {
		result.errorMessage = "Failed to read selected file path: " + hresultToMessage(hr);
		releaseShellItem(selectedItem);
		releaseFileOpenDialog(dialog);
		return result;
	}

	result.selected = true;
	result.path = wideToUtf8(selectedPath);

	CoTaskMemFree(selectedPath);
	releaseShellItem(selectedItem);
	releaseFileOpenDialog(dialog);
	return result;
#else
	(void)ownerWindow;
	(void)title;
	(void)initialPath;
	(void)filters;
	result.errorMessage = "Native file selection is only supported on Windows.";
	return result;
#endif
}

}
