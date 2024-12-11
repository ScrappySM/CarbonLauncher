#include "pipemanager.h"

#include <spdlog/spdlog.h>

using namespace Carbon;

PipeManager::PipeManager() {
	this->pipeReader = std::thread([this]() {
		// Create a pipe that *other* processes can connect to
		auto pipe = CreateNamedPipe(
			L"\\\\.\\pipe\\CarbonPipe",
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			1024,
			1024,
			0,
			NULL
		);

		if (pipe == INVALID_HANDLE_VALUE) {
			spdlog::error("Failed to create pipe");
			return;
		}

		if (!ConnectNamedPipe(pipe, NULL)) {
			spdlog::error("Failed to connect to pipe");
			CloseHandle(pipe);
			return;
		}

		spdlog::info("Connected to pipe");

		// Infinitely read and parse packets and add them to the queue
		while (true) {
			char buffer[1024];
			DWORD bytesRead = 0;
			if (!ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL)) {
				spdlog::error("Failed to read from pipe");
				break;
			}
			// Parse the packet
			std::string packet(buffer, bytesRead);
			auto delimiter = packet.find("-:-");
			if (delimiter == std::string::npos) {
				spdlog::error("Invalid packet received");
				continue;
			}
			auto type = packet.substr(0, delimiter);
			auto data = packet.substr(delimiter + 3);
			PacketType packetType = PacketType::UNKNOWNTYPE;
			if (type == "LOADED") {
				packetType = PacketType::LOADED;
			}
			else if (type == "STATECHANGE") {
				packetType = PacketType::STATECHANGE;
			}
			else if (type == "LOG") {
				packetType = PacketType::LOG;
			}
			Packet parsedPacket;
			parsedPacket.type = packetType;

			parsedPacket.data = data;
			if (data.empty()) {
				parsedPacket.data = std::nullopt;
			}

			{
				std::lock_guard<std::mutex> lock(this->pipeMutex);
				this->packets.push_back(parsedPacket);
			}
		}
	});
}

PipeManager::~PipeManager() {
	this->pipeReader.join();
	this->packets = {};
}

std::queue<Packet> PipeManager::GetPacketsByType(PacketType packet) {
	std::queue<Packet> filteredPackets;
	std::lock_guard<std::mutex> lock(this->pipeMutex);

	for (auto& currentPacket : this->packets) {
		if (currentPacket.type == packet) {
			filteredPackets.push(currentPacket);

			// Delete the packet from the vector
			for (auto it = this->packets.begin(); it != this->packets.end(); ++it) {
				if (it->type == packet) {
					this->packets.erase(it);
					break;
				}
			}
		}
	}

	return filteredPackets;
}