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
	enum class PacketType {
		LOADED, // Sent when the game is loaded
		STATECHANGE, // Sent when the game state changes (e.g. menu -> game)
		LOG, // (e.g.game log [will be implemented later])
		UNKNOWNTYPE // Unknown packet type
	};

	class Packet {
	public:
		PacketType type = PacketType::UNKNOWNTYPE;
		std::optional<std::string> data;
	};

	class PipeManager {
	public:
		PipeManager();
		~PipeManager();

		bool pipeInitialized = false;

		std::mutex pipeMutex;
		std::vector<Packet> packets = {};

		std::vector<Packet>& GetPackets() {
			std::lock_guard<std::mutex> lock(this->pipeMutex);
			return this->packets;
		}
		
		std::queue<Packet> GetPacketsByType(PacketType packet);

	private:
		std::thread pipeReader;
	};
}; // namespace Carbon
