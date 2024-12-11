#include "discordmanager.h"
#include "guimanager.h"
#include "state.h"

#include <spdlog/spdlog.h>

constexpr auto discordClientId = 1315436867545595904;

using namespace Carbon;

DiscordManager::DiscordManager() {
    discord::Core* core{};
	auto result = discord::Core::Create(discordClientId, DiscordCreateFlags_NoRequireDiscord, &core);

    this->core.reset(core);

	if (!this->core) {
		spdlog::warn("Failed to create Discord core");
	}

	else {
		auto dCore = this->core.get();

		dCore->SetLogHook(discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
			spdlog::debug("[Discord] {}", message);
			});

		dCore->SetLogHook(discord::LogLevel::Info, [](discord::LogLevel level, const char* message) {
			spdlog::info("[Discord] {}", message);
			});

		dCore->SetLogHook(discord::LogLevel::Warn, [](discord::LogLevel level, const char* message) {
			spdlog::warn("[Discord] {}", message);
			});

		dCore->SetLogHook(discord::LogLevel::Error, [](discord::LogLevel level, const char* message) {
			spdlog::error("[Discord] {}", message);
			});

		this->core->ActivityManager().RegisterCommand("carbonlauncher://run");

		this->currentActivity = {};
		auto& activity = this->GetActivity();

		activity.SetDetails("The latest modded launcher for Scrap Mechanic!");
		activity.SetState("Not in game... https://github.com/ScrappySM/CarbonLauncher!");

		activity.GetAssets().SetLargeImage("carbonlauncher");
		activity.GetAssets().SetLargeText("Carbon Launcher");
		activity.GetAssets().SetSmallImage("carbonlauncher");
		activity.GetAssets().SetSmallText("Carbon Launcher");

		activity.SetType(discord::ActivityType::Playing);

		activity.SetSupportedPlatforms((uint32_t)discord::ActivitySupportedPlatformFlags::Desktop);

		this->core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
			if (result != discord::Result::Ok) {
				spdlog::error("Failed to update Discord RPC (error: {})", (int)result);
			}
			});
	}


	spdlog::info("Created Discord instance");
}

void DiscordManager::UpdateState(const std::string& state) {
	spdlog::info("Updating state to {}", state);
	if (!this->core) {
		return;
	}

	this->currentActivity.SetState(state.c_str());
	this->UpdateActivity();
}

void DiscordManager::UpdateDetails(const std::string& details) {
	spdlog::info("Updating details to {}", details);
	if (!this->core) {
		return;
	}

	this->currentActivity.SetDetails(details.c_str());
	this->UpdateActivity();
}

DiscordManager::~DiscordManager() {
	spdlog::info("Destroying Discord instance");
}

void DiscordManager::UpdateActivity() const {
	spdlog::info("Updating activity");

	if (!this->core) {
		return;
	}

	this->core->ActivityManager().UpdateActivity(this->currentActivity, [](discord::Result result) {
		if (result != discord::Result::Ok) {
			spdlog::error("Failed to update activity");
		}
		});
}

discord::Activity& DiscordManager::GetActivity() {
	spdlog::info("Getting activity");

	return this->currentActivity;
}

void DiscordManager::Update() {
	if (!this->core) {
		return;
	}

	auto statePackets = C.pipeManager->GetPacketsByType(PacketType::STATECHANGE);
	if (!statePackets.empty()) {
		auto& packet = statePackets.front();

		if (!packet.data.has_value()) {
			spdlog::warn("Received state change packet with no data");
			statePackets.pop();
			return;
		}

		int state = std::stoi(packet.data.value());

		switch (state) {
		case 1:
			this->UpdateState("Loading into a game...");
			break;
		case 2:
			this->UpdateState("In a game!");
			break;
		case 3:
			this->UpdateState("In the main menu");
			break;
		};

		statePackets.pop();
	}

	this->core->RunCallbacks();
}