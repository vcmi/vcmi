
/*
 * CFilesystemLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ISimpleResourceLoader.h"
#include "CResourceLoader.h"

class CFileInfo;
class CInputStream;

/**
 * A class which can scan and load files of directories.
 */
class DLL_LINKAGE CFilesystemLoader : public ISimpleResourceLoader
{
public:
	/**
	 * Ctor.
	 *
	 * @param baseDirectory Specifies the base directory and their sub-directories which should be indexed.
	 * @param depth - recursion depth of subdirectories search. 0 = no recursion
	 *
	 * @throws std::runtime_error if the base directory is not a directory or if it is not available
	 */
	explicit CFilesystemLoader(const std::string & baseDirectory, size_t depth = 16, bool initial = false);

	/**
	 * Loads a resource with the given resource name.
	 *
	 * @param resourceName The unqiue resource name in space of the filesystem.
	 * @return a input stream object, not null
	 */
	std::unique_ptr<CInputStream> load(const std::string & resourceName) const;

	/**
	 * Checks if the file entry exists.
	 *
	 * @return true if the entry exists, false if not.
	 */
	bool existsEntry(const std::string & resourceName) const;

	/**
	 * Gets all entries in the filesystem.
	 *
	 * @return a list of all entries in the filesystem.
	 */
	std::unordered_map<ResourceID, std::string> getEntries() const;

	/**
	 * Gets the origin of the archive loader.
	 *
	 * @return the file path to directory with archive (e.g. path/to/h3/mp3)
	 */
	std::string getOrigin() const;

	bool createEntry(std::string filename);

private:
	/** The base directory which is scanned and indexed. */
	std::string baseDirectory;

	/** A list of files in the directory
	 * key = ResourceID for resource loader
	 * value = name that can be used to access file
	*/
	std::unordered_map<ResourceID, std::string> fileList;

	/**
	 * Returns a list of pathnames denoting the files in the directory denoted by this pathname.
	 *
	 * If the pathname of this directory is absolute, then the file info pathnames are absolute as well. If the pathname of this directory is relative
	 * then the file info pathnames are relative to the basedir as well.
	 *
	 * @return a list of pathnames denoting the files and directories in the directory denoted by this pathname
	 * The array will be empty if the directory is empty. Ptr is null if the directory doesn't exist or if it isn't a directory.
	 */
	std::unordered_map<ResourceID, std::string> listFiles(size_t depth, bool initial) const;
};
