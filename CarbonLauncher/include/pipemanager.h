#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <optional>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

namespace Carbon {
	// The type of packet
	enum class PacketType {
		LOADED, // Sent when the game is loaded
		STATECHANGE, // Sent when the game state changes (e.g. menu -> game)
		LOG, // (e.g.game log [will be implemented later])
		UNKNOWNTYPE // Unknown packet type
	};

	// A packet sent over the pipe
	class Packet {
	public:
		PacketType type = PacketType::UNKNOWNTYPE;
		std::optional<std::string> data;
	};

	// Manages the pipe connection to the game
	class PipeManager {
	public:
		// Initializes the pipe manager and starts a thread
		// listening for packets from the game
		PipeManager();
		~PipeManager();

		// Gets all the packets received from the game
		// @return A vector of all the packets received from the game
		// @note This function is thread-safe
		std::vector<Packet>& GetPackets() {
			std::lock_guard<std::mutex> lock(this->pipeMutex);
			return this->packets;
		}

		bool IsConnected() const { return this->connected; }
		void SetConnected(bool connected) { this->connected = connected; }
		
		// Gets all the packets of a specific type, removing them from the vector
		// @param packet The type of packet to get
		// @return A queue of all the packets of the specified type
		std::queue<Packet> GetPacketsByType(PacketType packet);

	private:
		// Used to read packets from the pipe
		std::mutex pipeMutex;
		std::thread pipeReader;
		std::vector<Packet> packets = {};

		bool connected = false;
	};
}; // namespace Carbon
