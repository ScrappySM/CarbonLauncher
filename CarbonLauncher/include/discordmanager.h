#pragma once

#include <discord-game-sdk/discord_game_sdk.h>
#include <discord-game-sdk/discord.h>

#include <string>
#include <memory>

namespace Carbon {
	// Manages the Discord RPC
	class DiscordManager {
	public:
		// Initializes the Discord RPC and starts a thread
		// listening for game state changes (through PipeManager)
		DiscordManager();
		~DiscordManager();

		// Updates the state of the Discord RPC
		// @param state The new state
		void UpdateState(const std::string& state);

		// Updates the details of the Discord RPC
		// @param details The new details
		void UpdateDetails(const std::string& details);

		// Updates the activity of the Discord RPC
		void UpdateActivity() const;

		// Gets the current activity
		discord::Activity& GetActivity();

		// Updates the Discord RPC and listens for game state changes
		void Update();

	private:
		discord::User currentUser = discord::User{};
		discord::Activity currentActivity = discord::Activity{};

		std::unique_ptr<discord::Core> core = nullptr;
	};
}; // namespace Carbon
