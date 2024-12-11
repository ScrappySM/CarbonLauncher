#include "utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::string Carbon::Utils::GetCurrentModulePath() {
	char path[MAX_PATH];
	GetModuleFileNameA(NULL, path, MAX_PATH);
	return std::string(path);
}

std::string Carbon::Utils::GetCurrentModuleDir() {
	std::string path = Carbon::Utils::GetCurrentModulePath();
	return (path.substr(0, path.find_last_of('\\')) + "\\");
}
