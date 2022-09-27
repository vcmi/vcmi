/*
 * COutputStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CStream.h"

VCMI_LIB_NAMESPACE_BEGIN

/**
 * Abstract class which provides method definitions for writing into a stream.
 */
class DLL_LINKAGE COutputStream : public virtual CStream
{
public:
	/**
	 * D-tor.
	 */
	virtual ~COutputStream() {}

	/**
	 * Write n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to write.
	 * @return the number of bytes written actually.
	 */
	virtual si64 write(const ui8 * data, si64 size) = 0;
};

VCMI_LIB_NAMESPACE_END
