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

VCMI_LIB_NAMESPACE_BEGIN

class CInputStream;

/**
 * Reads primitive binary values from a underlying stream.
 *
 * The integers which are read are supposed to be little-endian values permanently. They will be
 * converted to big-endian values on big-endian machines.
 */
class DLL_LINKAGE CBinaryReader : public boost::noncopyable
{
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
    CBinaryReader(CInputStream * stream);

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
    void setStream(CInputStream * stream);

	/**
	 * Reads n bytes from the stream into the data buffer.
	 *
	 * @param data A pointer to the destination data array.
	 * @param size The number of bytes to read.
	 * @return the number of bytes read actually.
	 */
	si64 read(ui8 * data, si64 size);

	/**
	 * Reads integer of various size. Advances the read pointer.
	 *
	 * @return a read integer.
	 *
	 * @throws std::runtime_error if the end of the stream was reached unexpectedly
	 */
	ui8 readUInt8();
	si8 readInt8();
	ui16 readUInt16();
	si16 readInt16();
	ui32 readUInt32();
	si32 readInt32();
	ui64 readUInt64();
	si64 readInt64();

	std::string readString();

	inline bool readBool()
	{
		return readUInt8() != 0;
	}

	void skip(int count);
private:
    /**
     * Reads any integer. Advances the read pointer by its size.
     *
     * @return read integer.
     *
     * @throws std::runtime_error if the end of the stream was reached unexpectedly
     */
    template <typename CData>
    CData readInteger();

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

VCMI_LIB_NAMESPACE_END
