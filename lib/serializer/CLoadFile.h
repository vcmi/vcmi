/*
 * CLoadFile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinaryDeserializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CLoadFile : public IBinaryReader
{
	BinaryDeserializer serializer;
	std::fstream sfile;

	int read(std::byte * data, unsigned size) override; //throws!

public:
	CLoadFile(const boost::filesystem::path & fname, IGameInfoCallback * cb); //throws!

	template<class T>
	void load(T & data)
	{
		serializer & data;
	}

	bool hasFeature(BinaryDeserializer::Version v) const
	{
		return serializer.version >= v;
	}
};

VCMI_LIB_NAMESPACE_END
