/*
 * CMemorySerializer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinarySerializer.h"
#include "BinaryDeserializer.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Serializer that stores objects in the dynamic buffer. Allows performing deep object copies.
class DLL_LINKAGE CMemorySerializer
	: public IBinaryReader, public IBinaryWriter
{
	std::vector<ui8> buffer;

	size_t readPos; //index of the next byte to be read
public:
	BinaryDeserializer iser;
	BinarySerializer oser;

	int read(void * data, unsigned size) override; //throws!
	int write(const void * data, unsigned size) override;

	CMemorySerializer();

	template <typename T>
	static std::unique_ptr<T> deepCopy(const T &data)
	{
		CMemorySerializer mem;
		mem.oser & &data;

		std::unique_ptr<T> ret;
		mem.iser & ret;
		return ret;
	}
};

VCMI_LIB_NAMESPACE_END
