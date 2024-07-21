/*
 * TextIdentifier.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class TextIdentifier
{
	std::string identifier;
public:
	const std::string & get() const
	{
		return identifier;
	}

	TextIdentifier(const char * id):
		identifier(id)
	{}

	TextIdentifier(const std::string & id):
		identifier(id)
	{}

	template<typename... T>
	TextIdentifier(const std::string & id, size_t index, T... rest):
		TextIdentifier(id + '.' + std::to_string(index), rest...)
	{}

	template<typename... T>
	TextIdentifier(const std::string & id, const std::string & id2, T... rest):
		TextIdentifier(id + '.' + id2, rest...)
	{}
};

VCMI_LIB_NAMESPACE_END
