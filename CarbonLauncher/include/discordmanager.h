#pragma once

#include <discord_rpc.h>

#include <string>
#include <memory>

namespace Carbon {
	// Manages the Discord RPC
	class DiscordManager {
	public:
		// Initializes the Discord RPC
		DiscordManager();
		~DiscordManager();

		// Updates the state of the Discord RPC
		// @param state The new state
		void UpdateState(const std::string& state);

		// Updates the details of the Discord RPC
		// @param details The new details
		void UpdateDetails(const std::string& details);

		// Gets the current activity
		DiscordRichPresence& GetPresence();

		// Updates the Discord RPC and listens for game state changes
		void Update();

	private:
		DiscordEventHandlers discordHandlers = { 0 };
		DiscordRichPresence discordPresence = { 0 };

		/*discord::User currentUser = discord::User{};
		discord::Activity currentActivity = discord::Activity{};

		std::unique_ptr<discord::Core> core = nullptr;*/
	};
}; // namespace Carbon
