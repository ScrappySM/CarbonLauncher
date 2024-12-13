#pragma once

#include "guimanager.h"
#include "discordmanager.h"
#include "gamemanager.h"
#include "pipemanager.h"
#include "repomanager.h"

namespace Carbon {
	class CarbonState_t {
	public:
		Carbon::GUIManager guiManager;
		Carbon::DiscordManager discordManager;
		Carbon::GameManager gameManager;
		Carbon::PipeManager pipeManager;
		Carbon::RepoManager repoManager;

		const char* processTarget = "ScrapMechanic.exe";
		// const char* processTarget = "DummyGame.exe";
	};
}; // namespace Carbon

extern Carbon::CarbonState_t C;
