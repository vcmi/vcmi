/*
 * FileInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "FileInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace FileInfo
{

boost::string_ref GetFilename(boost::string_ref path)
{
	const auto pos = path.find_last_of("/\\");

	if (pos != boost::string_ref::npos)
		return path.substr(pos + 1);

	return path;
}

boost::string_ref GetExtension(boost::string_ref path)
{
	const auto dotPos = path.find_last_of('.');

	if(dotPos != boost::string_ref::npos)
		return path.substr(dotPos);

	return boost::string_ref{};
}

boost::string_ref GetStem(boost::string_ref path)
{
	auto begin	= path.find_last_of("/\\");
	auto end	= path.find_last_of('.');

	if (begin == boost::string_ref::npos)
		begin = 0;
	else
		begin += 1;

	if (end < begin)
		end = boost::string_ref::npos;

	return path.substr(begin, end);
}

boost::string_ref GetParentPath(boost::string_ref path)
{
	const auto pos = path.find_last_of("/\\");
	return path.substr(0, pos);
}

boost::string_ref GetPathStem(boost::string_ref path)
{
	const auto dotPos = path.find_last_of('.');
	return path.substr(0, dotPos);
}

}

VCMI_LIB_NAMESPACE_END
