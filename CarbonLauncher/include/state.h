#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "guimanager.h"
#include "discordmanager.h"
#include "processmanager.h"
#include "pipemanager.h"
#include "modmanager.h"

namespace Carbon {
	struct LogMessage {
		int colour = 0;
		std::string message;
		std::string time;
	};

	struct Settings {
		//std::string dataDir = "C:\\Program Files\\CarbonLauncher"; // Needs admin permissions
		std::string dataDir = "C:\\Users\\" + std::string(getenv("USERNAME")) + "\\AppData\\Local\\CarbonLauncher\\"; // Doesn't need admin permissions
	};

	// The main state of the Carbon Launcher
	// Contains all the managers and the process target
	class CarbonState_t {
	public:
		CarbonState_t() {
			//#ifndef NDEBUG
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
		Carbon::ProcessManager processManager;
		Carbon::PipeManager pipeManager;
		Carbon::ModManager modManager;

		// The settings for the Carbon Launcher
		Carbon::Settings settings;

		std::vector<LogMessage> logMessages;

		// The target process to manage (e.g. ScrapMechanic.exe)
		// This should never be a process that does not have a Contraption
		// located in the same place as ScrapMechanic.exe
		// const char* processTarget = "ScrapMechanic.exe";
		// const char* processTarget = "DummyGame.exe";
	};
}; // namespace Carbon


extern Carbon::CarbonState_t C;
