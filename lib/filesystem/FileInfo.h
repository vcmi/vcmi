/*
 * FileInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

namespace FileInfo
{

/**
 * Returns the name of the file.
 *
 * @return the name of the file. E.g. foo.txt
 */
std::string_view DLL_LINKAGE GetFilename(std::string_view path);

/**
 * Gets the file extension.
 *
 * @return the file extension. E.g. .ext
 */
std::string_view DLL_LINKAGE GetExtension(std::string_view path);

/**
 * Gets the file name exclusive the extension of the file.
 *
 * @return the file name exclusive the extension and the path of the file. E.g. foo
 */
std::string_view DLL_LINKAGE GetStem(std::string_view path);

/**
 * Gets the path to the file only.
 *
 * @return the path to the file only. E.g. ./dir/
 */
std::string_view DLL_LINKAGE GetParentPath(std::string_view path);

/**
 * Gets the file name + path exclusive the extension of the file.
 *
 * @return the file name exclusive the extension of the file. E.g. ./dir/foo
 */
std::string_view DLL_LINKAGE GetPathStem(std::string_view path);

}
