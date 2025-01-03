#include "pipemanager.h"
#include "state.h"

#include <spdlog/spdlog.h>

#include <ctime>

using namespace Carbon;

PipeManager::PipeManager() {
	this->pipeReader = std::thread([this]() {
		// Create a pipe that *other* processes can connect to
		auto pipe = CreateNamedPipe(
			L"\\\\.\\pipe\\CarbonPipe",
			PIPE_ACCESS_INBOUND,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			1024 * 128,
			1024 * 128,
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
			char buffer[1024]{};
			DWORD bytesRead = 0;
			auto res = ReadFile(pipe, buffer, sizeof(buffer), &bytesRead, NULL);
			if (!res) {
				//spdlog::error("Failed to read from pipe");
				int code = GetLastError();
				const char* error = nullptr;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&error, 0, nullptr);
				spdlog::error("Failed to read from pipe: {}", error);
				LocalFree((HLOCAL)error);

				C.discordManager.UpdateState("In the launcher!");

				// In this case, it is most likely the supervisor process has closed the pipe
				// We should wait for the supervisor to reconnect

				// Close the pipe
				CloseHandle(pipe);

				// Wait for the supervisor to reconnect
				pipe = CreateNamedPipe(
					L"\\\\.\\pipe\\CarbonPipe",
					PIPE_ACCESS_INBOUND,
					PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
					PIPE_UNLIMITED_INSTANCES,
					1024 * 1024,
					1024 * 1024,
					PIPE_WAIT,
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

				spdlog::info("Reconnected to pipe");
				continue;
			}

			// Parse the packet
			std::string packet(buffer, bytesRead);
			auto delimiter = packet.find("-:-");
			if (delimiter == std::string::npos) {
				spdlog::error("Invalid packet received, data: `{}`", packet);
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

				// just use libs to get timestamp, technically we could use the game's time
				// but that would require the game to send the time and this doesn't feel important enough

				// h, m, s
				std::time_t currentTime = std::time(nullptr);
				std::tm timeInfo;

				localtime_s(&timeInfo, &currentTime);

				std::string time = fmt::format("{:02}:{:02}:{:02}", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

				LogMessage logMessage;
				logMessage.message = data;
				logMessage.time = time;
				C.logMessages.emplace_back(logMessage);

				// If we have more than 200 messages, remove the oldest one
				if (C.logMessages.size() > 200) {
					C.logMessages.erase(C.logMessages.begin());
				}

				spdlog::trace("G-L : `{}` @ {}", !data.empty() ? data : "null", type);
				// Early return here, we don't want to add the log packet to the queue
				continue;
			}
			else {
				spdlog::error("Unknown packet type received, data: `{}`", packet);
				spdlog::trace("G-L : `{}` @ {}", !data.empty() ? data : "null", type);
				continue;
			}

			spdlog::trace("G-L : `{}` @ {}", !data.empty() ? data : "null", type);

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

			// Allow the thread some breathing room
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});
	this->pipeReader.detach();
}

PipeManager::~PipeManager() { }

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