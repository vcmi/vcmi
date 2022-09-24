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

#include <boost/utility/string_ref.hpp>

VCMI_LIB_NAMESPACE_BEGIN

namespace FileInfo
{

/**
 * Returns the name of the file.
 *
 * @return the name of the file. E.g. foo.txt
 */
boost::string_ref DLL_LINKAGE GetFilename(boost::string_ref path);

/**
 * Gets the file extension.
 *
 * @return the file extension. E.g. .ext
 */
boost::string_ref DLL_LINKAGE GetExtension(boost::string_ref path);

/**
 * Gets the file name exclusive the extension of the file.
 *
 * @return the file name exclusive the extension and the path of the file. E.g. foo
 */
boost::string_ref DLL_LINKAGE GetStem(boost::string_ref path);

/**
 * Gets the path to the file only.
 *
 * @return the path to the file only. E.g. ./dir/
 */
boost::string_ref DLL_LINKAGE GetParentPath(boost::string_ref path);

/**
 * Gets the file name + path exclusive the extension of the file.
 *
 * @return the file name exclusive the extension of the file. E.g. ./dir/foo
 */
boost::string_ref DLL_LINKAGE GetPathStem(boost::string_ref path);

}

VCMI_LIB_NAMESPACE_END
