
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

class CFileInfo;
class CInputStream;

/**
 * A class which can scan and load files of directories.
 */
class DLL_LINKAGE CFilesystemLoader : public ISimpleResourceLoader
{
public:
	/**
	 * Default c-tor.
	 */
	CFilesystemLoader();

	/**
	 * Ctor.
	 *
	 * @param baseDirectory Specifies the base directory and their sub-directories which should be indexed.
	 * @param depth - recursion depth of subdirectories search. 0 = no recursion
	 *
	 * @throws std::runtime_error if the base directory is not a directory or if it is not available
	 */
	explicit CFilesystemLoader(const std::string & baseDirectory, size_t depth = 16);

	/**
	 * Ctor.
	 *
	 * @param baseDirectory Specifies the base directory and their sub-directories which should be indexed.
	 *
	 * @throws std::runtime_error if the base directory is not a directory or if it is not available
	 */
	explicit CFilesystemLoader(const CFileInfo & baseDirectory, size_t depth = 16);

	/**
	 * Opens a base directory to be read and indexed.
	 *
	 * @param baseDirectory Specifies the base directory and their sub-directories which should be indexed.
	 *
	 * @throws std::runtime_error if the base directory is not a directory or if it is not available
	 */
	void open(const std::string & baseDirectory, size_t depth);

	/**
	 * Opens a base directory to be read and indexed.
	 *
	 * @param baseDirectory Specifies the base directory and their sub-directories which should be indexed.
	 *
	 * @throws std::runtime_error if the base directory is not a directory or if it is not available
	 */
	void open(const CFileInfo & baseDirectory, size_t depth);

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
	std::list<std::string> getEntries() const;

private:
	/** The base directory which is scanned and indexed. */
	std::string baseDirectory;

	/** A list of files in the directory */
	std::list<CFileInfo> fileList;
};
