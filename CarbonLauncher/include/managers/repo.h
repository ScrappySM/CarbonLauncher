#pragma once

#include "pch.h"

namespace CL {
	struct ModFile {
		char name[128] = { 0 };
		char url[256] = { 0 };
		char hash[64] = { 0 };
	};

	struct Mod {
		char name[64] = { 0 };
		std::vector<std::string> authors = {};
		char description[256] = { 0 };
		char commitHash[16] = { 0 };
		std::vector<ModFile> files = {};
	};

	class RepoManager {
	public:
		static RepoManager& GetInstance() {
			static RepoManager instance;
			return instance;
		}

		/// <summary>
		/// Load a cached repository from a .json file
		/// Should not be called manually, this is for quick loading and when internet is not available
		/// </summary>
		/// <param name="url">The URL to the repository</param>
		/// <returns>True if the repository was loaded successfully</returns>
		bool LoadCachedRepo(const std::string_view& url) noexcept;

		/// <summary>
		/// Load a repository from a URL to a .json file
		/// </summary>
		/// <param name="url">The URL to the repository</param>
		/// <returns>True if the repository was loaded successfully</returns>
		bool LoadRepo(const std::string_view& url) noexcept;

		// TODO: Put back in private
		/// <returns>True if the repository was loaded successfully</returns>
		bool LoadRepoFromJson(const std::string_view& json) noexcept;

		std::vector<Mod>& GetMods() noexcept { return mods; }

	private:
		RepoManager() = default;
		~RepoManager() = default;

		RepoManager(const RepoManager&) = delete;
		RepoManager& operator=(const RepoManager&) = delete;

		std::vector<Mod> mods;
	};
} // namespace CL
