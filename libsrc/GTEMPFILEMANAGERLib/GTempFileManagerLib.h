
#ifndef H_GTEMP_FILE_MANAGER_LIB
#define H_GTEMP_FILE_MANAGER_LIB
#pragma once

#include <ctime>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace gtempfilemanagerlib
{
	/**
	* @class CGTempFileManager
	* @brief Manages temporary files and directories created during runtime.
	*
	* This class creates temporary files and directories with user-defined
	* prefixes in the system's temporary directory. All created resources are
	* tracked and automatically removed when the manager is destroyed.
	*
	* Names never collide with existing files or directories: if the generated
	* name is already taken, a numeric suffix (_1, _2, ...) is appended. The
	* manager therefore never overwrites, and never deletes, anything it did
	* not create itself.
	*
	* The class is movable but not copyable, so that each resource is owned
	* (and removed) by exactly one manager.
	*
	* @par Example
	* @code
	* gtempfilemanagerlib::CGTempFileManager l_tmp;
	* std::filesystem::path l_file = l_tmp.CreateTempFile("mydata", ".txt");
	* std::filesystem::path l_dir  = l_tmp.CreateTempDir("work");
	* // ... use l_file and l_dir ...
	* // Both are removed automatically when l_tmp goes out of scope.
	* @endcode
	*/
	class CGTempFileManager
	{
	public:

		/**
		* @brief Constructs a new CGTempFileManager.
		*
		* No resources are created during construction.
		*/
		CGTempFileManager() = default;

		/** @brief Copying is disabled: each resource has a single owner. */
		CGTempFileManager(const CGTempFileManager&) = delete;

		/** @brief Copying is disabled: each resource has a single owner. */
		CGTempFileManager& operator=(const CGTempFileManager&) = delete;

		/**
		* @brief Move constructor: takes ownership of the tracked resources.
		*
		* @param f_other Manager whose resources are taken over; it is left
		*                tracking nothing.
		*/
		CGTempFileManager(CGTempFileManager&& f_other) noexcept
			: m_temp_files(std::move(f_other.m_temp_files)),
			  m_temp_dirs(std::move(f_other.m_temp_dirs))
		{
			f_other.m_temp_files.clear();
			f_other.m_temp_dirs.clear();
		}

		/**
		* @brief Move assignment: removes the current resources, then takes
		*        ownership of those of @p f_other.
		*
		* @param f_other Manager whose resources are taken over; it is left
		*                tracking nothing.
		* @return Reference to this manager.
		*/
		CGTempFileManager& operator=(CGTempFileManager&& f_other) noexcept
		{
			if (this != &f_other)
			{
				Cleanup();
				m_temp_files = std::move(f_other.m_temp_files);
				m_temp_dirs = std::move(f_other.m_temp_dirs);
				f_other.m_temp_files.clear();
				f_other.m_temp_dirs.clear();
			}
			return *this;
		}

		/**
		* @brief Creates an empty temporary file in the system's temp directory.
		*
		* The file name is built from the given prefix, optionally followed by
		* the current timestamp, and the given extension. If that name is
		* already taken, a numeric suffix is appended to make it unique. The
		* file is created empty and tracked for automatic removal.
		*
		* @param f_prefix             First part of the generated file name.
		* @param f_ext                File extension, including the dot
		*                             (e.g. ".txt"). May be empty.
		* @param f_time_stamp_in_name If true, a timestamp is included in the
		*                             file name.
		* @return The full path of the created temporary file.
		*
		* @throw std::filesystem::filesystem_error If the temp directory cannot
		*        be determined.
		* @throw std::runtime_error If the file cannot be created.
		*/
		inline std::filesystem::path CreateTempFile(const std::string& f_prefix,
													const std::string& f_ext,
													const bool f_time_stamp_in_name = true)
		{
			const std::filesystem::path l_path = MakeUniquePath(f_prefix, f_ext, f_time_stamp_in_name);

			std::ofstream l_file(l_path);
			if (!l_file)
			{
				throw std::runtime_error("CGTempFileManager: cannot create temporary file " + l_path.string());
			}
			l_file.close();

			m_temp_files.push_back(l_path);
			return l_path;
		}

		/**
		* @brief Creates a temporary directory in the system's temp directory.
		*
		* The directory name is built from the given prefix, optionally
		* followed by the current timestamp. If that name is already taken, a
		* numeric suffix is appended to make it unique. The directory is
		* tracked for automatic removal, together with all its contents.
		*
		* @param f_prefix             First part of the generated directory name.
		* @param f_time_stamp_in_name If true, a timestamp is included in the
		*                             directory name.
		* @return The full path of the created temporary directory.
		*
		* @throw std::filesystem::filesystem_error If the temp directory cannot
		*        be determined or the directory cannot be created.
		*/
		inline std::filesystem::path CreateTempDir(const std::string& f_prefix,
												   const bool f_time_stamp_in_name = true)
		{
			const std::filesystem::path l_path = MakeUniquePath(f_prefix, "", f_time_stamp_in_name);

			std::filesystem::create_directory(l_path);

			m_temp_dirs.push_back(l_path);
			return l_path;
		}

		/**
		* @brief Destructor that removes all tracked files and directories.
		*
		* Removes all the files and directories created via CreateTempFile()
		* and CreateTempDir(). Directories are removed together with their
		* contents. Errors during removal are ignored.
		*/
		~CGTempFileManager()
		{
			Cleanup();
		}

	private:

		/**
		* @brief Builds a path in the temp directory that does not exist yet.
		*
		* @param f_prefix             First part of the name.
		* @param f_ext                Extension appended at the end (may be empty).
		* @param f_time_stamp_in_name If true, a timestamp is appended to the prefix.
		* @return A path in the system's temp directory that is not taken.
		*/
		static std::filesystem::path MakeUniquePath(const std::string& f_prefix,
													const std::string& f_ext,
													const bool f_time_stamp_in_name)
		{
			std::string l_base = f_prefix;
			if (f_time_stamp_in_name)
			{
				l_base += "_" + std::to_string(std::time(nullptr));
			}

			const std::filesystem::path l_temp_dir = std::filesystem::temp_directory_path();
			std::filesystem::path l_path = l_temp_dir / (l_base + f_ext);

			std::error_code l_ec;
			for (unsigned int l_counter = 1; std::filesystem::exists(l_path, l_ec); ++l_counter)
			{
				l_path = l_temp_dir / (l_base + "_" + std::to_string(l_counter) + f_ext);
			}
			return l_path;
		}

		/**
		* @brief Removes all tracked files and directories, ignoring errors.
		*/
		void Cleanup() noexcept
		{
			for (const auto& l_file : m_temp_files)
			{
				std::error_code l_ec;
				std::filesystem::remove(l_file, l_ec);
			}
			for (auto l_it = m_temp_dirs.rbegin(); l_it != m_temp_dirs.rend(); ++l_it)
			{
				std::error_code l_ec;
				std::filesystem::remove_all(*l_it, l_ec);
			}
			m_temp_files.clear();
			m_temp_dirs.clear();
		}

		std::vector<std::filesystem::path> m_temp_files;

		std::vector<std::filesystem::path> m_temp_dirs;
	};
}

#endif
