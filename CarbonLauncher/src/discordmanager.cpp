#include "discordmanager.h"
#include "guimanager.h"
#include "state.h"

#include <spdlog/spdlog.h>
#include <fmt/format.h>

constexpr auto discordClientId = "1315436867545595904";

using namespace Carbon;

DiscordManager::DiscordManager() {
	DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
	handlers.ready = [](const DiscordUser* connectedUser) {
		spdlog::info("Discord connected to user {}", connectedUser->username);
		};
	handlers.disconnected = [](int errorCode, const char* message) {
		spdlog::warn("Discord disconnected with error code {}: {}", errorCode, message);
		};
	handlers.errored = [](int errorCode, const char* message) {
		spdlog::error("Discord error with code {}: {}", errorCode, message);
		};
	handlers.joinGame = [](const char* secret) {
		spdlog::info("Joining game with secret {}", secret);
		};
	handlers.spectateGame = [](const char* secret) {
		spdlog::info("Spectating game with secret {}", secret);
		};
	handlers.joinRequest = [](const DiscordUser* request) {
		spdlog::info("Join request from {}#{}", request->username, request->discriminator);
		};
    Discord_Initialize(discordClientId, &handlers, 1, NULL);
	this->discordHandlers = handlers;

	spdlog::info("Initialized Discord instance");

	this->discordPresence = DiscordRichPresence{
		.details = "The most advanced mod loader for Scrap Mechanic",
		.state = "In the main menu",
		.largeImageKey = "icon",
		.largeImageText = "Carbon Launcher",
		.button1Label = "GitHub",
		.button1Url = "https://github.com/ScrappySM/CarbonLauncher",
		.button2Label = "Download",
		.button2Url = "https://github.com/ScrappySM/CarbonLauncher/releases/latest",
		.instance = 0,
	};

	Discord_UpdatePresence(&this->discordPresence);

	spdlog::info("Updated Discord presence");

	Discord_UpdateHandlers(&handlers);
}

void DiscordManager::UpdateState(const std::string& state) {
	this->discordPresence.state = state.c_str();
	Discord_UpdatePresence(&this->discordPresence);
}

void DiscordManager::UpdateDetails(const std::string& details) {
	this->discordPresence.details = details.c_str();
	Discord_UpdatePresence(&this->discordPresence);
}

DiscordManager::~DiscordManager() {
	spdlog::info("Destroying Discord instance");
}

DiscordRichPresence& DiscordManager::GetPresence() {
	return this->discordPresence;
}

void DiscordManager::Update() {
	auto statePackets = C.pipeManager.GetPacketsByType(PacketType::STATECHANGE);
	if (!statePackets.empty()) {
		auto& packet = statePackets.front();

		if (!packet.data.has_value()) {
			spdlog::warn("Received state change packet with no data");
			statePackets.pop();
			return;
		}

		int state = 0;
		try {
			state = std::stoi(packet.data.value());
		}
		catch (std::invalid_argument& e) {
			spdlog::error("Failed to parse state change packet: {}", e.what());
			statePackets.pop();
			return;
		}

		switch (state) {
		case 1:
			spdlog::trace("In a loading screen");
			this->UpdateState("In a loading screen");
			break;
		case 2:
			spdlog::trace("In a game!");

			this->UpdateState(fmt::format("In a game! ({} mods loaded)", C.gameManager.GetLoadedCustomModules()));
			break;
		case 3:
			spdlog::trace("In the main menu");
			this->UpdateState(fmt::format("In the main menu with {} mods loaded", C.gameManager.GetLoadedCustomModules()));
			break;
		};

		statePackets.pop();
	}

	Discord_RunCallbacks();
	Discord_UpdateHandlers(&this->discordHandlers);
}