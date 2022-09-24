/*
 * CMemoryBuffer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CInputOutputStream.h"

VCMI_LIB_NAMESPACE_BEGIN

/**
 * A class which provides IO memory buffer.
 */
 
class DLL_LINKAGE CMemoryBuffer : public CInputOutputStream
{
public:
	typedef std::vector<ui8> TBuffer;

	/**
	 * C-tor.
	 *
	 */
	CMemoryBuffer();
	
	/**
	 * Write n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to write.
	 * @return the number of bytes written actually.
	 */
	si64 write(const ui8 * data, si64 size) override;	

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	si64 read(ui8 * data, si64 size) override;

	/**
	 * Seeks the internal read pointer to the specified position.
	 *
	 * @param position The read position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	si64 seek(si64 position) override;

	/**
	 * Gets the current read position in the stream.
	 *
	 * @return the read position.
	 */
	si64 tell() override;

	/**
	 * Skips delta numbers of bytes.
	 *
	 * @param delta The count of bytes to skip.
	 * @return the count of bytes skipped actually.
	 */
	si64 skip(si64 delta) override;

	/**
	 * Gets the length in bytes of the stream.
	 *
	 * @return the length in bytes of the stream.
	 */
	si64 getSize() override;
	
	const TBuffer & getBuffer(){return buffer;}

private:
	/** Actual data. */
	TBuffer buffer;
	
	/** Current reading position of the stream. */
	si64 position;
};


VCMI_LIB_NAMESPACE_END
