#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "guimanager.h"
#include "discordmanager.h"
#include "gamemanager.h"
#include "pipemanager.h"
#include "repomanager.h"

namespace Carbon {
	struct LogMessage {
		std::string message;
		std::string time;
	};

	// The main state of the Carbon Launcher
	// Contains all the managers and the process target
	class CarbonState_t {
	public:
		CarbonState_t() {
			//#ifndef NDEBUG
						// Tell winapi to show a console window
			AllocConsole();
			FILE* file;
			freopen_s(&file, "CONOUT$", "w", stdout);

			auto console = spdlog::stdout_color_mt("carbon");      // Create a new logger with color support
			spdlog::set_default_logger(console);                   // Set the default logger to the console logger
			console->set_level(spdlog::level::trace);			   // Set the log level to info
			spdlog::set_pattern("%^[ %H:%M:%S |  %-8l] %n: %v%$"); // Nice log format
			//#endif
		}

		Carbon::GUIManager guiManager;
		Carbon::DiscordManager discordManager;
		Carbon::GameManager gameManager;
		Carbon::PipeManager pipeManager;
		Carbon::RepoManager repoManager;

		//std::vector<std::string> logMessages;

		std::vector<LogMessage> logMessages;

		// The target process to manage (e.g. ScrapMechanic.exe)
		// This should never be a process that does not have a Contraption
		// located in the same place as ScrapMechanic.exe
		const char* processTarget = "ScrapMechanic.exe";

		// const char* processTarget = "DummyGame.exe";
	};
}; // namespace Carbon


extern Carbon::CarbonState_t C;
