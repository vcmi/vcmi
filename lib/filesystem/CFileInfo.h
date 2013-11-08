#pragma once

/*
 * CFileInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ResourceID.h"

/**
 * A class which holds information about a file.
 */
class DLL_LINKAGE CFileInfo
{
public:
	/**
	 * Default ctor.
	 */
	CFileInfo();

	/**
	 * Ctor.
	 *
	 * @param name The path and name of the file.
	 */
	explicit CFileInfo(std::string name);

	/**
	 * Checks if the file exists.
	 *
	 * @return true if the file exists, false if not.
	 */
	bool exists() const;

	/**
	 * Checks if the file is a directory.
	 *
	 * @return true if the file is a directory, false if not.
	 */
	bool isDirectory() const;

	/**
	 * Sets the name.
	 *
	 * @param name The name of the file
	 */
	void setName(const std::string & name);

	/**
	 * Gets the name of the file.
	 *
	 * @return the path and name of the file. E.g. ./dir/file.ext
	 */
	std::string getName() const;

	/**
	 * Gets the path to the file only.
	 *
	 * @return the path to the file only. E.g. ./dir/
	 */
	std::string getPath() const;

	/**
	 * Gets the file extension.
	 *
	 * @return the file extension. E.g. .ext
	 */
	std::string getExtension() const;

	/**
	 * Returns the name of the file.
	 *
	 * @return the name of the file. E.g. foo.txt
	 */
	std::string getFilename() const;

	/**
	 * Gets the file name + path exclusive the extension of the file.
	 *
	 * @return the file name exclusive the extension of the file. E.g. ./dir/foo
	 */
	std::string getStem() const;

	/**
	 * Gets the file name exclusive the extension of the file.
	 *
	 * @return the file name exclusive the extension and a path of the file. E.g. foo
	 */
	std::string getBaseName() const;

	/**
	 * Gets the extension type as a EResType enumeration.
	 *
	 * @return the extension type as a EResType enumeration.
	 */
	EResType::Type getType() const;

	/**
	 * Gets the timestamp of the file.
	 *
	 * @return the timestamp of the file, 0 if no timestamp was set
	 */
	std::time_t getDate() const;

private:
	/** Contains the original URI(not modified) e.g. ./dir/foo.txt */
	std::string name;
};
