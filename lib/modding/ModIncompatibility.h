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
	using ModListWithVersion = std::vector<std::pair<const std::string, const std::string>>;
	using ModList = std::vector<std::string>;

	ModIncompatibility(const ModListWithVersion & _missingMods)
	{
		std::ostringstream _ss;
		for(const auto & m : _missingMods)
			_ss << m.first << ' ' << m.second << std::endl;
		messageMissingMods = _ss.str();
	}
	
	ModIncompatibility(const ModListWithVersion & _missingMods, ModList & _excessiveMods)
		: ModIncompatibility(_missingMods)
	{
		std::ostringstream _ss;
		for(const auto & m : _excessiveMods)
			_ss << m << std::endl;
		messageExcessiveMods = _ss.str();
	}
	
	const char * what() const noexcept override
	{
		static const std::string w("Mod incompatibility exception");
		return w.c_str();
	}
	
	const std::string & whatMissing() const noexcept
	{
		return messageMissingMods;
	}
	
	const std::string & whatExcessive() const noexcept
	{
		return messageExcessiveMods;
	}

private:
	std::string messageMissingMods;
	std::string messageExcessiveMods;
};

VCMI_LIB_NAMESPACE_END
