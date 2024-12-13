#pragma once

#include "guimanager.h"
#include "discordmanager.h"
#include "gamemanager.h"
#include "pipemanager.h"
#include "repomanager.h"

namespace Carbon {
	// The main state of the Carbon Launcher
	// Contains all the managers and the process target
	class CarbonState_t {
	public:
		Carbon::GUIManager guiManager;
		Carbon::DiscordManager discordManager;
		Carbon::GameManager gameManager;
		Carbon::PipeManager pipeManager;
		Carbon::RepoManager repoManager;

		// The target process to manage (e.g. ScrapMechanic.exe)
		// This should never be a process that does not have a Contraption
		// located in the same place as ScrapMechanic.exe
		const char* processTarget = "ScrapMechanic.exe";

		// const char* processTarget = "DummyGame.exe";
	};
}; // namespace Carbon

extern Carbon::CarbonState_t C;
