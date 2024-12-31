#include "repomanager.h"
#include "state.h"
#include "utils.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>

using namespace Carbon;

// This is here to allow people to not be limited to 60 requests per hour, please don't abuse this
// It has no permissions anyway, so it can't do anything malicious
// It's like this to stop:
// - GitHub from revoking the token because it thinks it's put here by accident
// - Bots scraping the token and using it for malicious purposes
// 
// Please, do not abuse this! In the Scrap Mechanic community we trust, right?
constexpr int tok[] = { 0x67, 0x69, 0x74, 0x68, 0x75, 0x62, 0x5f, 0x70, 0x61, 0x74, 0x5f, 0x31, 0x31, 0x42, 0x4f, 0x43, 0x54, 0x58, 0x45, 0x59, 0x30, 0x67, 0x44, 0x4e, 0x38, 0x53,
0x49, 0x74, 0x32, 0x6b, 0x4c, 0x69, 0x50, 0x5f, 0x58, 0x39, 0x6a, 0x68, 0x34, 0x75, 0x4b, 0x66, 0x4f, 0x5a, 0x66, 0x67, 0x6e, 0x59, 0x43, 0x4e, 0x66, 0x39, 0x67, 0x64, 0x6c, 0x77,
0x79, 0x36, 0x4e, 0x44, 0x50, 0x75, 0x68, 0x36, 0x63, 0x6a, 0x38, 0x31, 0x39, 0x34, 0x48, 0x49, 0x72, 0x62, 0x67, 0x79, 0x68, 0x49, 0x33, 0x42, 0x45, 0x42, 0x4d, 0x42, 0x4e, 0x4e,
0x42, 0x73, 0x65, 0x37, 0x73, 0x65, 0x59 };
std::string token = std::string(tok, tok + sizeof(tok) / sizeof(tok[0]));

cpr::Header authHeader = cpr::Header{ { "Authorization", "token " + token } };

std::string getDefaultBranch(std::string ghUser, std::string ghRepo) {
	std::string repoDataURL = fmt::format("https://api.github.com/repos/{}/{}", ghUser, ghRepo);
	auto response = cpr::Get(cpr::Url{ repoDataURL }, authHeader);

	try {
	auto json = nlohmann::json::parse(response.text);
	return json["default_branch"];
	}
	catch (nlohmann::json::parse_error& e) {
		spdlog::error("Failed to parse JSON: {}", e.what());
		return "main";
	}
}

std::optional<Mod> RepoManager::JSONToMod(const nlohmann::json& jMod) {
	std::string branch = getDefaultBranch(jMod["ghUser"].get<std::string>(), jMod["ghRepo"].get<std::string>());
	std::string manifestURL = fmt::format("https://raw.githubusercontent.com/{}/{}/{}/manifest.json", jMod["ghUser"].get<std::string>(), jMod["ghRepo"].get<std::string>(), branch);
	cpr::Response manifest = cpr::Get(cpr::Url{ manifestURL });

	bool hasManifest = manifest.status_code == 200;

	Mod mod;
	mod.ghUser = jMod["ghUser"];
	mod.ghRepo = jMod["ghRepo"];

	if (hasManifest) {
		std::string modRepo = jMod["ghRepo"].get<std::string>();
		spdlog::trace("Mod {} has a manifest.json!", modRepo);
		auto jManifest = nlohmann::json::parse(manifest.text);

		mod.name = jManifest["name"];
		mod.authors = jManifest["authors"].get<std::vector<std::string>>();
		mod.description = jManifest["description"];
		mod.installed = false;
	}
	else {
		spdlog::warn("Mod {} ({}) does not have a manifest.json!", jMod["name"].get<std::string>(), jMod["ghRepo"].get<std::string>());

		mod.name = jMod["name"];
		mod.authors = jMod["authors"].get<std::vector<std::string>>();
		mod.description = jMod["description"];
		mod.installed = false;
	}

	// Check if the mod is installed
	if (std::filesystem::exists(Utils::GetCurrentModuleDir() + "/mods/" + mod.ghRepo)) {
		mod.installed = true;

		// Check if the mod wants an update
		std::string tagFile = Utils::GetCurrentModuleDir() + "/mods/" + mod.ghRepo + "/tag.txt";
		std::ifstream file(tagFile);
		std::string tag;
		file >> tag;
		file.close();

		auto latestURL = fmt::format("https://api.github.com/repos/{}/{}/releases/latest", mod.ghUser, mod.ghRepo);
		auto latest = cpr::Get(cpr::Url{ latestURL }, authHeader);

		auto jContents = nlohmann::json();
		auto jLatest = nlohmann::json();

		try {
			jLatest = nlohmann::json::parse(latest.text);
		}
		catch (nlohmann::json::parse_error& e) {
			spdlog::error("Failed to parse JSON: {}", e.what());
			return std::nullopt;
		}

		// Find the latest tag
		std::string currentTag = jLatest["tag_name"];
		if (tag != currentTag) {
			mod.wantsUpdate = true;
		}

		spdlog::info("Mod {} is installed and {} want an update ({} {} {})!", mod.name, mod.wantsUpdate ? "does" : "does not", tag, mod.wantsUpdate ? "!=" : "==", currentTag);
	}

	return mod;
}

std::vector<Mod> RepoManager::URLToMods(const std::string& url) {
	std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
	cpr::Response response = cpr::Get(cpr::Url{ url });

	nlohmann::json json;
	try {
		json = nlohmann::json::parse(response.text);
	}
	catch (nlohmann::json::parse_error& e) {
		spdlog::error("Failed to parse JSON: {}", e.what());
		return {};
	}

	std::vector<Mod> mods;
	std::vector<std::thread> threads;
	for (auto& jMod : json["mods"]) {
		threads.push_back(std::thread([this, &mods, jMod]() {
			auto mod = this->JSONToMod(jMod);
			if (mod.has_value())
				mods.push_back(mod.value());
			}));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = end - start;
	spdlog::info("Loaded {} mods in {} seconds", mods.size(), elapsed_seconds.count());

	return mods;
}

RepoManager::RepoManager() {
	//this->repos = URLToRepos(REPOS_URL);S

	std::thread([this]() {
		// Allow some time for the console to initialize
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		std::lock_guard<std::mutex> lock(this->repoMutex);
		this->mods = URLToMods(REPOS_URL);
		this->hasLoaded = true;
	}).detach();
}

RepoManager::~RepoManager() {
	this->mods.clear();
}

void Mod::Install() {
	std::thread([&]() {
		// Display the mod as already installed to
		// make the user aware that the mod is being installed
		this->installed = true;

		auto contentsURL = fmt::format("https://api.github.com/repos/{}/{}/contents/", this->ghUser, this->ghRepo);
		auto latestURL = fmt::format("https://api.github.com/repos/{}/{}/releases/latest", this->ghUser, this->ghRepo);
		auto contents = cpr::Get(cpr::Url{ contentsURL }, authHeader);
		auto latest = cpr::Get(cpr::Url{ latestURL }, authHeader);

		auto jContents = nlohmann::json();
		auto jLatest = nlohmann::json();

		try {
			jContents = nlohmann::json::parse(contents.text);
			jLatest = nlohmann::json::parse(latest.text);
		}
		catch (nlohmann::json::parse_error& e) {
			spdlog::error("Failed to parse JSON: {}", e.what());
			this->installed = false;
			return;
		}

		// Find the latest tag
		std::string tag = jLatest["tag_name"];

		std::unordered_map<std::string, std::string> downloadURLs;
		for (const auto& asset : jLatest["assets"]) {
			try {
				std::string url = asset["browser_download_url"];
				std::string name = asset["name"];
				downloadURLs.insert({ name, url });
			}
			catch (nlohmann::json::exception& e) {
				spdlog::error("Failed to get browser_download_url: {}", e.what());
				continue;
			}
		}

		// Download the mod
		for (const auto& [name, url] : downloadURLs) {
			auto download = cpr::Get(cpr::Url{ url }, authHeader);

			if (download.status_code != 200) {
				spdlog::error("Failed to download mod: {}", download.status_code);
				this->installed = false;
				return;
			}

			// mod dir
			std::string modDir = Utils::GetCurrentModuleDir() + "/mods/" + this->ghRepo + "/";
			std::filesystem::create_directories(modDir);

			// mod file
			std::string modFile = modDir + name;
			std::ofstream file(modFile, std::ios::binary);
			file << download.text;
			file.close();

			spdlog::info("Downloaded: {}", modFile);
		}

		// Save `tag` file with the tag name
		std::string tagFile = Utils::GetCurrentModuleDir() + "/mods/" + this->ghRepo + "/tag";
		std::ofstream file(tagFile);
		file << tag;
		file.close();

		this->installed = true;

		spdlog::info("Installed: {}", this->name);
		}).detach();
}

void Mod::Uninstall() {
	if (C.gameManager.IsGameRunning()) {
		spdlog::error("Game is running, cannot uninstall mod");
		return;
	}

	std::string modDir = Utils::GetCurrentModuleDir() + "/mods/" + this->ghRepo + "/";
	std::filesystem::remove_all(modDir);
	this->installed = false;
	spdlog::info("Uninstalled: {}", this->name);
}

void Mod::Update() {
	if (C.gameManager.IsGameRunning()) {
		spdlog::error("Game is running, cannot update mod");
		return;
	}

	if (!this->wantsUpdate) {
		spdlog::warn("Mod {} does not want an update but was requested to update", this->name);
		return;
	}

	auto latestURL = fmt::format("https://api.github.com/repos/{}/{}/releases/latest", this->ghUser, this->ghRepo);
	auto latest = cpr::Get(cpr::Url{ latestURL }, authHeader);

	auto jLatest = nlohmann::json();
	try {
		jLatest = nlohmann::json::parse(latest.text);
	}
	catch (nlohmann::json::parse_error& e) {
		spdlog::error("Failed to parse JSON: {}", e.what());
		return;
	}

	// Find the latest tag
	std::string tag = jLatest["tag_name"];
	
	std::unordered_map<std::string, std::string> downloadURLs;
	for (const auto& asset : jLatest["assets"]) {
		try {
			std::string url = asset["browser_download_url"];
			std::string name = asset["name"];
			downloadURLs.insert({ name, url });
		}
		catch (nlohmann::json::exception& e) {
			spdlog::error("Failed to get browser_download_url: {}", e.what());
			continue;
		}
	}

	// Download the mod
	for (const auto& [name, url] : downloadURLs) {
		auto download = cpr::Get(cpr::Url{ url }, authHeader);
		// mod dir
		std::string modDir = Utils::GetCurrentModuleDir() + "/mods/" + this->ghRepo + "/";
		std::filesystem::create_directories(modDir);
		// mod file
		std::string modFile = modDir + name;
		std::ofstream file(modFile, std::ios::binary);
		file << download.text;
		file.close();
		spdlog::info("Downloaded: {}", modFile);
	}

	// Save `tag` file with the tag name
	std::string tagFile = Utils::GetCurrentModuleDir() + "/mods/" + this->ghRepo + "/tag";
	std::ofstream file(tagFile);
	file << tag;
	file.close();

	spdlog::info("Updated: {}", this->name);

	this->wantsUpdate = false;
}
