#include "repomanager.h"
#include "utils.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <cpr/cpr.h>

using namespace Carbon;

Repo RepoManager::JSONToRepo(nlohmann::json json) {
	Repo repo;

	repo.name = json["name"];
	repo.link = json["link"];

	for (const auto& mod : json["mods"]) {
		Mod m{
			.name = mod["name"],
			.description = mod["description"],
			.repo = mod["repo"],
			.tag = mod["tag"],
			.supported = mod["supported"]
		};

		for (const auto& author : mod["authors"]) {
			m.authors.push_back(author);
		}

		for (const auto& dependency : mod["dependencies"]) {
			m.dependencies.push_back(dependency);
		}

		for (const auto& file : mod["files"]) {
			std::string sFile = file;
			spdlog::info("File: {}", sFile);
			m.files.push_back(sFile);
		}

		repo.mods.push_back(m);
	}

	return repo;
}

std::vector<Repo> RepoManager::URLToRepos(const std::string& url) {
	cpr::Response response = cpr::Get(cpr::Url{ url });

	nlohmann::json json = nlohmann::json::parse(response.text);

	std::vector<Repo> repos;
	for (const auto& repo : json["repositories"]) {
		repos.push_back(JSONToRepo(repo));
	}

	return repos;
}

RepoManager::RepoManager() {
	this->repos = URLToRepos(REPOS_URL);

	// Scan the filesystem for all of the `files` inside each mod, if they exist, set `installed` to true
	for (auto& repo : this->repos) {
		for (auto& mod : repo.mods) {
			std::string path = Utils::GetCurrentModuleDir() + "modules\\" + repo.name + "\\" + mod.files[0];
			if (std::filesystem::exists(path)) {
				mod.installed = true;
			}
		}
	}
}

RepoManager::~RepoManager() {
	this->repos.clear();
}
