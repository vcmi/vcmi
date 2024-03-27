/*
 * CSaveFile.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinarySerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CSaveFile : public IBinaryWriter
{
public:
	BinarySerializer serializer;

	boost::filesystem::path fName;
	std::unique_ptr<std::fstream> sfile;

	CSaveFile(const boost::filesystem::path &fname); //throws!
	~CSaveFile();
	int write(const std::byte * data, unsigned size) override;

	void openNextFile(const boost::filesystem::path &fname); //throws!
	void clear();
	void reportState(vstd::CLoggerBase * out) override;

	void putMagicBytes(const std::string &text);

	template<class T>
	CSaveFile & operator<<(const T &t)
	{
		serializer & t;
		return * this;
	}
};

VCMI_LIB_NAMESPACE_END
