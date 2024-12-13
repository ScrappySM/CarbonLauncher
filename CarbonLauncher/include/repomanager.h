#pragma once

#include <nlohmann/json.hpp>

#include <vector>
#include <string>

constexpr const char* REPOS_URL = "https://github.com/ScrappySM/CarbonLauncher/raw/refs/heads/main/repos.json";

namespace Carbon {
	struct Mod {
		// Name of the mode
		std::string name;

		// A list of all the authors of the mod
		std::vector<std::string> authors;

		// A short description of the mod
		std::string description;

		// The link to the GitHub repo of the mod
		std::string repo;

		// A list of all the dependencies of the mod
		std::vector<std::string> dependencies;

		// The git repo tag to download
		std::string tag;

		// A list of all the files to download from the GitHub releases
		std::vector<std::string> files;

		// The supported game version
		std::string supported;

		bool installed = false;
	};

	struct Repo {
		// The name of the repository (e.g. ScrappySM, Scrap-Mods)
		std::string name;

		// The link to the repositories website
		std::string link;

		// A list of all the mods
		std::vector<Mod> mods;
	};

	class RepoManager {
	public:
		RepoManager();
		~RepoManager();

		std::vector<Repo> URLToRepos(const std::string& url);
		std::vector<Repo>& GetRepos() { return repos; }

	private:
		Repo JSONToRepo(nlohmann::json json);
		std::vector<Repo> repos = {};
	};
}; // namespace Carbon
