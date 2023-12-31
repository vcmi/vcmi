/*
 * CStream.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CStream : private boost::noncopyable
{
public:
	/**
	 * D-tor.
	 */
	virtual ~CStream() = default;

	/**
	 * Seeks to the specified position.
	 *
	 * @param position The position from the beginning.
	 * @return the position actually moved to, -1 on error.
	 */
	virtual si64 seek(si64 position) = 0;

	/**
	 * Gets the current position in the stream.
	 *
	 * @return the position.
	 */
	virtual si64 tell() = 0;

	/**
	 * Relative seeks to the specified position.
	 *
	 * @param delta The count of bytes to seek from current position.
	 * @return the count of bytes skipped actually.
	 */
	virtual si64 skip(si64 delta) = 0;

	/**
	 * Gets the length of the stream.
	 *
	 * @return the length in bytes
	 */
	virtual si64 getSize() = 0;	
};

VCMI_LIB_NAMESPACE_END
