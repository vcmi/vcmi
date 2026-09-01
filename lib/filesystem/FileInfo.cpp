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

namespace FileInfo
{

std::string_view GetFilename(std::string_view path)
{
	const auto pos = path.find_last_of("/\\");

	if (pos != std::string_view::npos)
		return path.substr(pos + 1);

	return path;
}

std::string_view GetExtension(std::string_view path)
{
	const auto dotPos = path.find_last_of('.');

	if(dotPos != std::string_view::npos)
		return path.substr(dotPos);

	return std::string_view{};
}

std::string_view GetStem(std::string_view path)
{
	auto begin	= path.find_last_of("/\\");
	auto end	= path.find_last_of('.');

	if (begin == std::string_view::npos)
		begin = 0;
	else
		begin += 1;

	if (end < begin)
		end = std::string_view::npos;

	return path.substr(begin, end);
}

std::string_view GetParentPath(std::string_view path)
{
	const auto pos = path.find_last_of("/\\");
	return path.substr(0, pos);
}

std::string_view GetPathStem(std::string_view path)
{
	const auto dotPos = path.find_last_of('.');
	return path.substr(0, dotPos);
}

}
