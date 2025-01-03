#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <codecvt>

#include <cstdio>
#include <thread>
#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <MinHook.h>

// TODO: Send packets in a separate thread
// TODO: Fix messages too large causing the pipe to break

const uintptr_t ContraptionOffset = 0x1267538;

class Console {
	virtual ~Console() {}
	virtual void Log(const std::string&, WORD colour, WORD LogType) = 0;
	virtual void LogNoRepeat(const std::string&, WORD colour, WORD LogType) = 0;
};

class Contraption {
private:
	/* 0x0000 */ char pad_0x0000[0x58];
public:
	/* 0x0058 */ Console* console;
private:
	/* 0x0060 */ char pad_0x0060[0x11C];
public:
	/* 0x017C */ int state;

public:
	static Contraption* GetInstance() {
		// TODO sig scan for this (90 48 89 05 ? ? ? ? + 0x4 @ Contraption)
		return *reinterpret_cast<Contraption**>((uintptr_t)GetModuleHandle(nullptr) + ContraptionOffset);
	}
};

using LogFunction = void(Console::*)(const std::string&, WORD, WORD);

HANDLE gPipe = nullptr;
LogFunction oLogFunction = nullptr;
LogFunction oLogNoRepeatFunction = nullptr;
std::mutex gLogMutex;

enum PacketType {
	LOADED,
	STATECHANGE,
	LOG,
	UNKNOWNTYPE
};

static void ResetPipe() {
	if (gPipe != nullptr && gPipe != INVALID_HANDLE_VALUE) {
		CloseHandle(gPipe);
		gPipe = nullptr;
	}

	gPipe = CreateFile(
		"\\\\.\\pipe\\CarbonPipe",
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	if (gPipe == INVALID_HANDLE_VALUE) {
		spdlog::error("Failed to create pipe");
	}
	else {
		spdlog::info("Reconnected to pipe");
	}
}

static void ValidatePipe() {
	if (gPipe != nullptr && gPipe != INVALID_HANDLE_VALUE) {
		return;
	}

	int lastError = GetLastError();
	bool isPipeBroken = lastError != ERROR_SUCCESS;

	// Continuously attempt to reconnect to the pipe
	while (gPipe == nullptr || gPipe == INVALID_HANDLE_VALUE || isPipeBroken) {
		ResetPipe();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	spdlog::info("Reconnected to pipe");
}

static void SendPacket(PacketType packetType, const std::string& data = "") {
	std::lock_guard<std::mutex> lock(gLogMutex);
	ValidatePipe();

	auto send = [&](const std::string& packet) {
		DWORD bytesWritten = 0;
		BOOL res = false;
		res = WriteFile(gPipe, packet.c_str(), (DWORD)packet.size(), &bytesWritten, nullptr);
		if (!res) {
			int error = GetLastError();
			char* errorStr = nullptr;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorStr, 0, nullptr);
			// Trim the \n off the end
			errorStr[strlen(errorStr) - 2] = '\0';
			spdlog::error("Failed to write to pipe: {} ({})", errorStr, error);
			LocalFree((HLOCAL)errorStr);
			ResetPipe();
			return;
		}
		spdlog::info("Sent packet: {}", packet);
		};

	// Check if there are new lines in the data
	auto delimiter = data.find('\n');
	std::vector<std::string> packets;
	if (delimiter != std::string::npos) {
		size_t start = 0;
		while (delimiter != std::string::npos) {
			packets.push_back(data.substr(start, delimiter - start));
			start = delimiter + 1;
			delimiter = data.find('\n', start);
		}
		packets.push_back(data.substr(start));
	}
	else {
		packets.push_back(data);
	}

	if (packets.size() == 0) {
		return;
	}
	else if (packets.size() > 1) {
		spdlog::warn("Splitting packet into {} parts", packets.size());
	}

	for (auto& packet : packets) {
		std::string packetStr;
		switch (packetType) {
		case LOADED:
			packetStr = "LOADED-:-";
			break;
		case STATECHANGE:
			packetStr = "STATECHANGE-:-" + packet;
			break;
		case LOG:
			packetStr = "LOG-:-" + packet;
			break;
		default:
			packetStr = "UNKNOWNTYPE-:-" + packet;
			break;
		}
		send(packetStr);
	}
}

static void HookedLog(Console* console, const std::string& message, WORD color1, WORD color2) {
	SendPacket(LOG, message);
    (console->*oLogFunction)(message, color1, color2);
}

static void HookVTableFunc(void** vtable, int index, void* newFunction, void** originalFunction) {
	DWORD oldProtect;
	VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	*originalFunction = vtable[index];
	vtable[index] = newFunction;
	VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);
}

/*
 * DLL main thread
 * 
 * @param lpParam The parameter, in this case the module handle
 */
DWORD WINAPI DllMainThread(LPVOID lpParam) {
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

	auto console = spdlog::stdout_color_mt("carbon");
	spdlog::set_default_logger(console);
	spdlog::set_level(spdlog::level::trace);

	auto hModule = static_cast<HMODULE>(lpParam);

	// Get contraption instance
	auto contraption = Contraption::GetInstance();
	while (contraption == nullptr || contraption->state < 1 || contraption->state > 3) {
		contraption = Contraption::GetInstance();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		static int i = 0;
		if (i++ % 30 == 0) { // Every 3 seconds
			spdlog::warn("Waiting for Contraption...");
		}
	}

	// Check for the state changing off of 1 (not loading anymore)
	while (contraption->state == 1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	spdlog::info("Contraption state: {}", contraption->state);

	gPipe = CreateFile(
		"\\\\.\\pipe\\CarbonPipe",
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	ValidatePipe();
	spdlog::info("Connected to pipe");

	// Send the loaded packet
	SendPacket(LOADED);

	// Hook the consoles `Log` and `LogNoRepeat` functions (to the same thing)
	// Make them send a packet to the pipe with LOG:-:<message>
    void** vtable = *reinterpret_cast<void***>(contraption->console);
	// ~Console, Log, LogNoRepeat
	// 0x0, 0x8, 0x10
	HookVTableFunc(vtable, 0x8 / sizeof(void*), HookedLog, reinterpret_cast<void**>(&oLogFunction));
	HookVTableFunc(vtable, 0x10 / sizeof(void*), HookedLog, reinterpret_cast<void**>(&oLogNoRepeatFunction));

	// Main loop
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		static int lastState = 0;
		if (lastState != contraption->state) {
			lastState = contraption->state;
			// Send the state change packet
			SendPacket(STATECHANGE, std::to_string(contraption->state));
		}
	}

	FreeLibraryAndExitThread(hModule, 0);
	return 0;
}

/*
 * DLL entry point
 * 
 * @param hModule The module handle
 * @param ulReason The reason for calling this function
 * @param lpReserved Reserved
 * @return The exit code of the application
 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
	if (ulReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, DllMainThread, hModule, 0, nullptr);
	}

	return TRUE;
}