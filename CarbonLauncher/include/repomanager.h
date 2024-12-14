#pragma once

#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <mutex>

constexpr const char* REPOS_URL = "https://github.com/ScrappySM/CarbonLauncher/raw/refs/heads/main/repos.json";

namespace Carbon {
	struct Mod {
		// Name of the mode
		std::string name;

		// A list of all the authors of the mod
		std::vector<std::string> authors;

		// A short description of the mod
		std::string description;

		// The link to the mods GitHub page
		std::string user;
		std::string repoName;

		bool installed = false;
		bool hasUpdate = false;

		void Install();
		void Uninstall();
	};

	class RepoManager {
	public:
		// Initializes the RepoManager and downloads the repos.json file
		RepoManager();
		~RepoManager();

		// Converts a JSON object to a Repo object
		// @param json The JSON object to convert
		// @return The converted Repo object (`json` -> `Mod`)
		std::vector<Mod> URLToMods(const std::string& url);

		// Gets all the repos
		// @return A vector of all the mods
		std::vector<Mod>& GetMods() {
			std::lock_guard<std::mutex> lock(this->modMutex);
			return this->mods;
		}

		bool hasLoaded = false;

	private:
		Mod JSONToMod(nlohmann::json jMod);
		std::vector<Mod> mods = {};
		std::mutex modMutex;
	};
}; // namespace Carbon
