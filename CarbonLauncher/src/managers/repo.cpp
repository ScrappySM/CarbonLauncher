#include "pch.h"

#include "managers/repo.h"

using namespace CL;

bool RepoManager::LoadCachedRepo(const std::string_view& path) noexcept {
	spdlog::error("Loading cached repository is not implemented yet.");
	return false;
}

bool RepoManager::LoadRepo(const std::string_view& url) noexcept {
#ifdef _DEBUG
	this->mods.clear();
#endif

	auto start = std::chrono::high_resolution_clock::now();

	cpr::Header headers = { { "User-Agent", "CarbonLauncher" } };

#ifdef _DEBUG
	cpr::Response response = cpr::Get(cpr::Url{ fmt::format("{}?a={}", url.data(), std::rand()).c_str() }, headers);
#else
	cpr::Response response = cpr::Get(cpr::Url{ url.data() }, headers);
#endif

	if (response.status_code != 200) {
		spdlog::error("Failed to load repository from URL, status code: {}. Attempting to load cached repository...", response.status_code);
		return this->LoadCachedRepo(url);
	}

	if (response.text.empty()) {
		spdlog::error("Failed to load repository from URL, response is empty.");
		return false;
	}

	auto requestEnd = std::chrono::high_resolution_clock::now();
	auto requestDuration = std::chrono::duration_cast<std::chrono::milliseconds>(requestEnd - start).count();
	spdlog::info("Made request in {}ms", requestDuration);
	start = std::chrono::high_resolution_clock::now();

	auto resp = this->LoadRepoFromJson(response.text);
	if (!resp) spdlog::error("Failed to load repository from URL, JSON parsing failed.");

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	spdlog::info("Parsed repository in {}ms", duration);

	return resp;
}

bool RepoManager::LoadRepoFromJson(const std::string_view& json) noexcept {
	nlohmann::json j = nlohmann::json::parse(json, nullptr, false, true);
	if (j.is_discarded())
		return false;

	// Array of dll mods
	if (!j.contains("dlls"))
		return false;

	const auto& dlls = j["dlls"];
	if (!dlls.is_array())
		return false;

	this->mods.reserve(dlls.size());

	for (const auto& mod : dlls) {
		spdlog::info("Mod: {}", mod["name"].get<std::string>());

		if (!mod.contains("name") || !mod.contains("authors") || !mod.contains("description") || !mod.contains("commitHash") || !mod.contains("files"))
			continue;

		if (!mod["name"].is_string()) continue;
		const std::string& nameStr = mod["name"].get_ref<const std::string&>();

		auto& jAuthors = mod["authors"];
		if (!jAuthors.is_array()) continue;
		std::vector<std::string> authors = {};
		for (const auto& author : jAuthors) {
			if (!author.is_string()) continue;
			authors.emplace_back(author.get<std::string>());
		}

		if (!mod["description"].is_string()) continue;
		const std::string& descriptionStr = mod["description"].get_ref<const std::string&>();

		if (!mod["commitHash"].is_string()) continue;
		const std::string& commitHashStr = mod["commitHash"].get_ref<const std::string&>();

		auto& files = mod["files"];
		if (!files.is_array()) continue;
		std::vector<ModFile> modFiles = {};

		for (const auto& file : files) {
			if (!file.contains("name") || !file.contains("url") || !file.contains("hash"))
				continue;

			if (!file["name"].is_string() || !file["url"].is_string() || !file["hash"].is_string())
				continue;

			const std::string& nameStr = file["name"].get_ref<const std::string&>();
			const std::string& urlStr = file["url"].get_ref<const std::string&>();
			const std::string& hashStr = file["hash"].get_ref<const std::string&>();

			ModFile modFile{};
			strcpy_s(modFile.name, nameStr.c_str());
			strcpy_s(modFile.url, urlStr.c_str());
			strcpy_s(modFile.hash, hashStr.c_str());

			modFiles.emplace_back(modFile);
		}

		Mod mod;
		strcpy_s(mod.name, nameStr.c_str());
		strcpy_s(mod.description, descriptionStr.c_str());
		strcpy_s(mod.commitHash, commitHashStr.c_str());

		mod.authors = std::move(authors);
		mod.files = std::move(modFiles);
		this->mods.emplace_back(mod);
	}
	
	return true;
}
