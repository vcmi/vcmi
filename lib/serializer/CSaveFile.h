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
#include "CSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ISaveFile : public IBinaryWriter
{
protected:
	BinarySerializer serializer;

public:
	ISaveFile()
		: serializer(this)
	{}

	template<class T>
	void save(const T & data)
	{
		static_assert(is_serializeable<BinarySerializer, T>::value, "This class can't be serialized (possible pointer?)");
		serializer & data;
	}

};

class DLL_LINKAGE SaveFile final : public ISaveFile
{
	std::vector<std::byte> saveData;

	int write(const std::byte * data, unsigned size) final;
public:
	SaveFile();

	void writeFile(const boost::filesystem::path & fileName);

	const std::vector<std::byte> & currentContent() const
	{
		return saveData;
	}
};

class DLL_LINKAGE VerifyFile final : public ISaveFile
{
	std::vector<std::byte> testSaveData;
	size_t testPosition;

	int write(const std::byte * data, unsigned size) final;
public:
	VerifyFile(const std::vector<std::byte> & saveData);
};

VCMI_LIB_NAMESPACE_END
