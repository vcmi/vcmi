/*
 * ModIncompatibility.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ModIncompatibility: public std::exception
{
public:
	using StringPair = std::pair<const std::string, const std::string>;
	using ModList = std::list<StringPair>;

	ModIncompatibility(ModList && _missingMods):
		missingMods(std::move(_missingMods))
	{
		std::ostringstream _ss;
		for(const auto & m : missingMods)
			_ss << m.first << ' ' << m.second << std::endl;
		message = _ss.str();
	}

	const char * what() const noexcept override
	{
		return message.c_str();
	}

private:
	//list of mods required to load the game
	// first: mod name
	// second: mod version
	const ModList missingMods;
	std::string message;
};

VCMI_LIB_NAMESPACE_END
