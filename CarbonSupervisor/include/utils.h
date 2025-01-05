#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

enum PacketType {
	LOADED,
	STATECHANGE,
	LOG,
	UNKNOWNTYPE
};

namespace Utils {
	void InitLogging();

	class Pipe {
	public:
		Pipe() = default;

		static Pipe* GetInstance() {
			static Pipe instance;
			return &instance;
		}

		// Delete copy constructor and assignment operator
		Pipe(Pipe const&) = delete;
		void operator=(Pipe const&) = delete;

		void ResetPipe(bool reInformLauncher = false);

		void SendPacket(PacketType packetType, const std::string& data);
		void SendPacket(PacketType packetType, int data);
		void SendPacket(PacketType packetType);

		void ValidatePipe();

	private:
		std::mutex logMutex{};
		HANDLE pipe = INVALID_HANDLE_VALUE;
	};
} // namespace Utils
