#pragma once

#include <discord-game-sdk/discord_game_sdk.h>
#include <discord-game-sdk/discord.h>

#include <string>
#include <memory>

namespace Carbon {
	class DiscordManager {
	public:
		DiscordManager();
		~DiscordManager();

		void UpdateState(const std::string& state);
		void UpdateDetails(const std::string& details);

		void UpdateActivity() const;
		discord::Activity& GetActivity();

		void Update();

		discord::User currentUser = discord::User{};
		discord::Activity currentActivity = discord::Activity{};

		std::unique_ptr<discord::Core> core = nullptr;
	};
}; // namespace Carbon
