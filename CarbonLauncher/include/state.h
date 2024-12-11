#pragma once

#include "guimanager.h"
#include "discordmanager.h"
#include "gamemanager.h"
#include "pipemanager.h"

namespace Carbon {
	class CarbonState_t {
	public:
		Carbon::GUIManager* guiManager;
		Carbon::DiscordManager* discordManager;
		Carbon::GameManager* gameManager;
		Carbon::PipeManager* pipeManager;

		//const char* processTarget = "DummyGame.exe"; (NOTE: this is not included in this repo at the moment)
		const char* processTarget = "ScrapMechanic.exe";
	};
}; // namespace Carbon

extern Carbon::CarbonState_t C;
