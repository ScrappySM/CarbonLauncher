#pragma once
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

namespace Carbon::Utils {
	/*
	 * Get the current module path
	 *
	 * @return The current module path
	 */
	std::string GetCurrentModulePath();

	/*
	 * Get the current module directory
	 *
	 * @return The current module directory
	 */
	std::string GetCurrentModuleDir();
}; // namespace Carbon::Utils
