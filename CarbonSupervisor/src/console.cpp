#include "sm.h"
#include "utils.h"
#include "globals.h"

using LogFunction = void(SM::Console::*)(const std::string&, WORD, WORD);
LogFunction oLogFunction;
LogFunction oLogNoRepeatFunction;

static void HookedLog(SM::Console* console, const std::string& message, WORD colour, WORD type) {
	static Globals_t* globals = Globals_t::GetInstance();
	globals->logMessages.emplace(LogMessage{ colour, message });
	(console->*oLogFunction)(message, colour, type);
}

static void HookedLogNoRepeat(SM::Console* console, const std::string& message, WORD colour, WORD type) {
	static Globals_t* globals = Globals_t::GetInstance();
	globals->logMessages.emplace(LogMessage{ colour, message });
	(console->*oLogNoRepeatFunction)(message, colour, type);
}

static void HookVTableFunc(void** vtable, int index, void* newFunction, void** originalFunction = nullptr) {
	DWORD oldProtect;
	VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
	if (originalFunction) *originalFunction = vtable[index];
	vtable[index] = newFunction;
	VirtualProtect(&vtable[index], sizeof(void*), oldProtect, &oldProtect);
}

void SM::Console::Hook() {
	spdlog::info("Hooking console functions");

	// Hook the consoles `Log` and `LogNoRepeat` functions (to the same thing)
	// Make them send a packet to the pipe with LOG:-:<message>
	void** vtable = *reinterpret_cast<void***>(this);
	HookVTableFunc(vtable, 1, HookedLog, reinterpret_cast<void**>(&oLogFunction));
	HookVTableFunc(vtable, 2, HookedLogNoRepeat, reinterpret_cast<void**>(&oLogNoRepeatFunction));

	// Start a thread to send log messages to the launcher
	std::thread([]() {
		auto pipe = Utils::Pipe::GetInstance();
		static Globals_t* globals = Globals_t::GetInstance();
		auto& logMessages = globals->logMessages;

		while (true) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			if (!logMessages.empty()) {
				auto& message = logMessages.front();
				pipe->SendPacket(LOG, fmt::format("{}-|-{}", message.colour, message.message));
				logMessages.pop();
			}
		}
		}).detach();
}

void Utils::InitLogging() {
	AllocConsole();
	freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);

	auto console = spdlog::stdout_color_mt("carbon");
	spdlog::set_default_logger(console);
	spdlog::set_level(spdlog::level::trace);
	spdlog::set_pattern("%^[ %H:%M:%S |  %-8l] %n: %v%$");
}
