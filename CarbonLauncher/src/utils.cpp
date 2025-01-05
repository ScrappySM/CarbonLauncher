#include "utils.h"
#include "state.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::string Carbon::Utils::GetCurrentModulePath() {
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	return std::string(path);
}

std::string Carbon::Utils::GetCurrentModuleDir() {
	std::string path = GetCurrentModulePath();
	return path.substr(0, path.find_last_of('\\') + 1);
}

std::string Carbon::Utils::GetDataDir() {
	if (!std::filesystem::exists(C.settings.dataDir)) {
		if (!std::filesystem::create_directory(C.settings.dataDir)) {
			spdlog::error("Failed to create data directory");
			return GetCurrentModuleDir();
		}
	}

	return C.settings.dataDir;
}
