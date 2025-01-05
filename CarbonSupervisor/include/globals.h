#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <queue>

struct LogMessage {
	WORD colour;
	std::string message;
};

class Globals_t {
public:
	std::queue<LogMessage> logMessages = {};

	static Globals_t* GetInstance() {
		static Globals_t instance;
		return &instance;
	}
};
