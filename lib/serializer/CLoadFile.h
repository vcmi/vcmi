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
public:
	BinaryDeserializer serializer;

	std::string fName;
	std::unique_ptr<std::fstream> sfile;

	CLoadFile(const boost::filesystem::path & fname, ESerializationVersion minimalVersion = ESerializationVersion::CURRENT); //throws!
	virtual ~CLoadFile();
	int read(std::byte * data, unsigned size) override; //throws!

	void openNextFile(const boost::filesystem::path & fname, ESerializationVersion minimalVersion); //throws!
	void clear();
	void reportState(vstd::CLoggerBase * out) override;

	void checkMagicBytes(const std::string & text);

	template<class T>
	CLoadFile & operator>>(T &t)
	{
		serializer & t;
		return * this;
	}
};

VCMI_LIB_NAMESPACE_END
