#include "discordmanager.h"
#include "guimanager.h"
#include "state.h"

#include <spdlog/spdlog.h>
#include <fmt/format.h>

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

		activity.SetDetails("https://github.com/ScrappySM/CarbonLauncher!");
		activity.SetState("In the launcher!");

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
	if (!this->core) {
		return;
	}

	this->currentActivity.SetState(state.c_str());
	this->UpdateActivity();
}

void DiscordManager::UpdateDetails(const std::string& details) {
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
	return this->currentActivity;
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

	if (this->core)
		this->core->RunCallbacks();
}