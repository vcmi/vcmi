/*
 * SavegamePath.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class Calendar;
class CMapHeader;
struct StartInfo;

/// Constructs paths for saves that belong to a particular game session.
class DLL_LINKAGE SavegamePath final
{
public:
	static std::string generateGameDirectoryName(const StartInfo & startInfo, const CMapHeader & mapHeader);
	static std::string getGameDirectoryName(const StartInfo & startInfo, const CMapHeader & mapHeader);
	static std::string getAutosavePath(const StartInfo & startInfo,
		const CMapHeader & mapHeader,
		const Calendar & calendar);
	static bool isAutosaveName(const std::string & filename);
	static std::string getPath(const StartInfo & startInfo,
		const CMapHeader & mapHeader,
		const std::string & filename);
};
