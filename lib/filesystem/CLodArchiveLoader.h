
/*
 * CLodArchiveLoader.h, part of VCMI engine
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
class CFileInputStream;

/**
 * A struct which holds information about the archive entry e.g. where it is located in space of the archive container.
 */
struct ArchiveEntry
{
	/**
	 * Default c-tor.
	 */
	ArchiveEntry();

	/** Entry name **/
	std::string name;

	/** Distance in bytes from beginning **/
	int offset;

	/** Size without compression in bytes **/
	int realSize;

	/** Size with compression in bytes or 0 if not compressed **/
	int size;
};

/**
 * A class which can scan and load files of a LOD archive.
 */
class DLL_LINKAGE CLodArchiveLoader : public ISimpleResourceLoader
{
public:
	/**
	 * Ctor.
	 *
	 * The file extension of the param archive determines the type of the Lod file.
	 * These are valid extensions: .LOD, .SND, .VID
	 *
	 * @param archive Specifies the file path to the archive which should be indexed and loaded.
	 *
	 * @throws std::runtime_error if the archive wasn't found or if the archive isn't supported
	 */
	explicit CLodArchiveLoader(const std::string & archive);

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const std::string & resourceName) const override;
	std::unordered_map<ResourceID, std::string> getEntries() const override;
	bool existsEntry(const std::string & resourceName) const override;
	std::string getOrigin() const override;

private:
	const ArchiveEntry * getArchiveEntry(const std::string & resourceName) const;

	/**
	 * Initializes a LOD archive.
	 *
	 * @param fileStream File stream to the .lod archive
	 */
	void initLODArchive(CFileInputStream & fileStream);

	/**
	 * Initializes a VID archive.
	 *
	 * @param fileStream File stream to the .vid archive
	 */
	void initVIDArchive(CFileInputStream & fileStream);

	/**
	 * Initializes a SND archive.
	 *
	 * @param fileStream File stream to the .snd archive
	 */
	void initSNDArchive(CFileInputStream & fileStream);

	/** The file path to the archive which is scanned and indexed. */
	std::string archive;

	/** Holds all entries of the archive file. An entry can be accessed via the entry name. **/
	std::unordered_map<std::string, ArchiveEntry> entries;
};
