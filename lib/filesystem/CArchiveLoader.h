/*
 * CArchiveLoader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ISimpleResourceLoader.h"
#include "ResourceID.h"

VCMI_LIB_NAMESPACE_BEGIN

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
	int fullSize;

	/** Size with compression in bytes or 0 if not compressed **/
	int compressedSize;
};

/**
 * A class which can scan and load files of a LOD archive.
 */
class DLL_LINKAGE CArchiveLoader : public ISimpleResourceLoader
{
public:
	/**
	 * Ctor.
	 *
	 * The file extension of the param archive determines the type of the Lod file.
	 * These are valid extensions: .LOD, .SND, .VID
	 *
	 * @param archive Specifies the file path to the archive which should be indexed and loaded.
	 * @param extractArchives Specifies if the original H3 archives should be extracted to a separate folder.
	 *
	 * @throws std::runtime_error if the archive wasn't found or if the archive isn't supported
	 */
	CArchiveLoader(std::string mountPoint, boost::filesystem::path archive, bool extractArchives = false);

	/// Interface implementation
	/// @see ISimpleResourceLoader
	std::unique_ptr<CInputStream> load(const ResourceID & resourceName) const override;
	bool existsResource(const ResourceID & resourceName) const override;
	std::string getMountPoint() const override;
	void updateFilteredFiles(std::function<bool(const std::string &)> filter) const override {}
	std::unordered_set<ResourceID> getFilteredFiles(std::function<bool(const ResourceID &)> filter) const override;
	/** Extracts one archive entry to the specified subfolder. Used for Video and Sound */
	void extractToFolder(std::string outputSubFolder, CFileInputStream & fileStream, ArchiveEntry entry);
	/** Extracts one archive entry to the specified subfolder. Used for Images, Sprites, etc */
	void extractToFolder(std::string outputSubFolder, const std::string &mountPoint, ArchiveEntry entry);

private:
	/**
	 * Initializes a LOD archive.
	 *
	 * @param fileStream File stream to the .lod archive
	 */
	void initLODArchive(const std::string &mountPoint, CFileInputStream & fileStream);

	/**
	 * Initializes a VID archive.
	 *
	 * @param fileStream File stream to the .vid archive
	 */
	void initVIDArchive(const std::string &mountPoint, CFileInputStream & fileStream);

	/**
	 * Initializes a SND archive.
	 *
	 * @param fileStream File stream to the .snd archive
	 */
	void initSNDArchive(const std::string &mountPoint, CFileInputStream & fileStream);

	/** The file path to the archive which is scanned and indexed. */
	boost::filesystem::path archive;

	std::string mountPoint;

	/** Holds all entries of the archive file. An entry can be accessed via the entry name. **/
	std::unordered_map<ResourceID, ArchiveEntry> entries;

	/** Specifies if Original H3 archives should be extracted to a separate folder **/
	bool extractArchives;
};

/** Constructs the file path for the extracted file. Creates the subfolder hierarchy aswell **/
boost::filesystem::path createExtractedFilePath(std::string outputSubFolder, std::string entryName);

VCMI_LIB_NAMESPACE_END
