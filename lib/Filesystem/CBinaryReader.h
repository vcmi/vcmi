
/*
 * CBinaryReader.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class CInputStream;

/**
 * Reads primitive binary values from a underlying stream.
 *
 * The integers which are read are supposed to be little-endian values permanently. They will be
 * converted to big-endian values on big-endian machines.
 */
class DLL_LINKAGE CBinaryReader : public boost::noncopyable
{
	/**
	 * Reads any integer. Advances the read pointer by its size.
	 *
	 * @return read integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	template <typename CData>
	CData readInteger();

public:
	/**
	 * Default c-tor.
	 */
	CBinaryReader();

	/**
	 * C-tor.
	 *
	 * @param stream The base stream object which serves as the reading input.
	 */
	CBinaryReader(CInputStream & stream);

	/**
	 * Gets the underlying stream.
	 *
	 * @return the base stream.
	 */
	CInputStream * getStream();

	/**
	 * Sets the underlying stream.
	 *
	 * @param stream The base stream to set
	 */
	void setStream(CInputStream & stream);

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	si64 read(ui8 * data, si64 size);

	/**
	 * Reads a unsigned 8 bit integer. Advances the read pointer by one byte.
	 *
	 * @return a unsigned 8 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	ui8 readUInt8();

	/**
	 * Reads a signed 8 bit integer. Advances the read pointer by one byte.
	 *
	 * @return a signed 8 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	si8 readInt8();

	/**
	 * Reads a unsigned 16 bit integer. Advances the read pointer by two bytes.
	 *
	 * @return a unsigned 16 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	ui16 readUInt16();

	/**
	 * Reads a signed 16 bit integer. Advances the read pointer by two bytes.
	 *
	 * @return a signed 16 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	si16 readInt16();

	/**
	 * Reads a unsigned 32 bit integer. Advances the read pointer by four bytes.
	 *
	 * @return a unsigned 32 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	ui32 readUInt32();

	/**
	 * Reads a signed 32 bit integer. Advances the read pointer by four bytes.
	 *
	 * @return a signed 32 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	si32 readInt32();

	/**
	 * Reads a unsigned 64 bit integer. Advances the read pointer by eight bytes.
	 *
	 * @return a unsigned 64 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	ui64 readUInt64();

	/**
	 * Reads a signed 64 bit integer. Advances the read pointer by eight bytes.
	 *
	 * @return a signed 64 bit integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	si64 readInt64();

private:
	/**
	 * Gets a end of stream exception message.
	 *
	 * @param bytesToRead The number of bytes which should be read.
	 * @return the exception message text
	 */
	std::string getEndOfStreamExceptionMsg(long bytesToRead) const;

	/** The underlying base stream */
	CInputStream * stream;
};
