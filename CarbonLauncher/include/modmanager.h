#pragma once

#include <nlohmann/json.hpp>

#include <vector>
#include <string>
#include <mutex>

constexpr const char* REPOS_URL = "https://github.com/ScrappySM/CarbonRepo/raw/refs/heads/main/repos.json";

namespace Carbon {
	struct Mod {
		// Name of the mode
		std::string name;

		// A list of all the authors of the mod
		std::vector<std::string> authors;

		// A short description of the mod
		std::string description;

		// The link to the mods GitHub page
		std::string ghUser;
		std::string ghRepo;

		bool installed = false;
		bool wantsUpdate = false;

		void Install();
		void Uninstall();
		void Update();
	};

	enum ModTarget {
		Game, // Scrap Mechanic
		ModTool, // Scrap Mechanic Mod Tool
	};

	class ModManager {
	public:
		// Initializes the RepoManager and downloads the repos.json file
		ModManager();
		~ModManager();

		// Converts a JSON object to a Repo object
		// @param json The JSON object to convert
		// @return The converted Repo object (`json` -> `Repo`)
		std::pair<std::vector<Mod>, std::vector<Mod>> URLToMods(const std::string& url);

		// Gets all the repos
		// @return A vector of all the repos
		std::vector<Mod>& GetMods(ModTarget target) {
			std::lock_guard<std::mutex> lock(this->repoMutex);
			return target == ModTarget::Game ? gameMods : modToolMods;
		}

		bool hasLoaded = false;

	private:
		std::vector<Mod> gameMods;
		std::vector<Mod> modToolMods;

		std::optional<Mod> JSONToMod(const nlohmann::json& jMod);
		std::mutex repoMutex;
	};
}; // namespace Carbon
