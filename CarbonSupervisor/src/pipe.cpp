#include "utils.h"
#include "sm.h"

using namespace Utils;

void Pipe::ResetPipe(bool reInformLauncher) {
	pipe = CreateFile(
		"\\\\.\\pipe\\CarbonPipe",
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	// Send the contraption state change so that the launcher knows the game didn't crash
	if (reInformLauncher) {
		SendPacket(STATECHANGE, std::to_string(SM::Contraption::GetInstance()->state));
	}
}

void Pipe::ValidatePipe() {
	if (pipe != nullptr && pipe != INVALID_HANDLE_VALUE) {
		return;
	}

	bool isPipeBroken = GetLastError() != ERROR_SUCCESS;

	// Continuously attempt to reconnect to the pipe
	while (pipe == nullptr || pipe == INVALID_HANDLE_VALUE || isPipeBroken) {
		ResetPipe(false);
		isPipeBroken = GetLastError() != ERROR_SUCCESS;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	isPipeBroken = GetLastError() != ERROR_SUCCESS;
	if (isPipeBroken) {
		spdlog::error("Failed to reconnect to pipe");
	}
}

void Pipe::SendPacket(PacketType packetType, int data) {
	SendPacket(packetType, std::to_string(data));
}

void Pipe::SendPacket(PacketType packetType) {
	SendPacket(packetType, "");
}

// TODO: Fix messages too large causing the pipe to break
void Pipe::SendPacket(PacketType packetType, const std::string& data) {
	if (data.size() > static_cast<unsigned long long>(1024) * 9) { // anything over 9kb is ridiculous
		spdlog::warn("Packet data too large, size: {}", data.size());
		SendPacket(packetType, fmt::format("{}... (truncated)", data.substr(0, 1024 * 8)));
		return;
	}

	std::lock_guard<std::mutex> lock(logMutex);

	ValidatePipe();

	auto send = [&](const std::string& packet) {
		DWORD bytesWritten = 0;
		BOOL res = false;
		res = WriteFile(pipe, packet.c_str(), (DWORD)packet.size(), &bytesWritten, nullptr);
		if (!res) {
			int error = GetLastError();
			char* errorStr = nullptr;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorStr, 0, nullptr);
			// Trim the \n off the end
			errorStr[strlen(errorStr) - 2] = '\0';
			spdlog::error("Failed to write to pipe: {} ({})", errorStr, error);
			LocalFree((HLOCAL)errorStr);
			ResetPipe(true);
			return;
		}
		spdlog::info("Sent packet: {}", packet);
		};

	std::string packetStr;
	switch (packetType) {
	case LOADED:
		packetStr = "LOADED-:-";
		break;
	case STATECHANGE:
		packetStr = "STATECHANGE-:-" + data;
		break;
	case LOG:
		packetStr = "LOG-:-" + data;
		break;
	default:
		packetStr = "UNKNOWNTYPE-:-" + data;
		break;
	}
	send(packetStr);
}

