/*
 * PngStripWriter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PngStripWriter.h"

#include <zlib.h>

static constexpr size_t bytesPerPixel = 4;
static constexpr size_t outBufferSize = 64 * 1024;

static void putUInt32(uint8_t * where, uint32_t value)
{
	for(int i = 0; i < 4; ++i)
		where[i] = static_cast<uint8_t>(value >> (24 - 8 * i));
}

PngStripWriter::PngStripWriter(const boost::filesystem::path & path, uint32_t width, uint32_t height)
	: file(path.c_str(), std::ofstream::binary)
	, stream(std::make_unique<z_stream>())
	, rowBuffer(1 + static_cast<size_t>(width) * bytesPerPixel)
	, outBuffer(outBufferSize)
	, width(width)
	, height(height)
{
	if(!file)
		throw std::runtime_error("Unable to open " + path.string() + " for writing");

	if(deflateInit(stream.get(), Z_DEFAULT_COMPRESSION) != Z_OK)
		throw std::runtime_error("Unable to initialize PNG compression");

	static constexpr uint8_t signature[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
	file.write(reinterpret_cast<const char *>(signature), sizeof(signature));

	uint8_t header[13] = {};
	putUInt32(header, width);
	putUInt32(header + 4, height);
	header[8] = 8; // bit depth
	header[9] = 6; // color type: RGBA
	writeChunk("IHDR", header, sizeof(header));

	// filter type of every row, the only one that is used
	rowBuffer[0] = 0;
}

PngStripWriter::~PngStripWriter()
{
	deflateEnd(stream.get());
}

void PngStripWriter::writeChunk(const char * type, const uint8_t * data, uint32_t size)
{
	uint8_t number[4];

	putUInt32(number, size);
	file.write(reinterpret_cast<const char *>(number), 4);
	file.write(type, 4);
	file.write(reinterpret_cast<const char *>(data), size);

	uLong crc = crc32(0, reinterpret_cast<const Bytef *>(type), 4);
	if(size > 0) // crc32 treats null buffer as a reset
		crc = crc32(crc, data, size);
	putUInt32(number, static_cast<uint32_t>(crc));
	file.write(reinterpret_cast<const char *>(number), 4);
}

void PngStripWriter::compress(int flushMode)
{
	int result;

	do
	{
		stream->next_out = outBuffer.data();
		stream->avail_out = static_cast<uInt>(outBuffer.size());

		result = deflate(stream.get(), flushMode);

		if(result != Z_OK && result != Z_STREAM_END && result != Z_BUF_ERROR)
			throw std::runtime_error("PNG compression failed");

		// compressed data is split between as many IDAT chunks as needed
		const auto produced = static_cast<uint32_t>(outBuffer.size() - stream->avail_out);
		if(produced > 0)
			writeChunk("IDAT", outBuffer.data(), produced);
	}
	while(stream->avail_out == 0 || (flushMode == Z_FINISH && result != Z_STREAM_END));
}

void PngStripWriter::writeRows(const uint8_t * rgba, uint32_t rowCount)
{
	if(finished || rowsWritten + rowCount > height)
		throw std::logic_error("Too many rows for PNG");

	const size_t rowSize = static_cast<size_t>(width) * bytesPerPixel;

	for(uint32_t row = 0; row < rowCount; ++row)
	{
		std::copy_n(rgba + row * rowSize, rowSize, rowBuffer.begin() + 1);

		stream->next_in = rowBuffer.data();
		stream->avail_in = static_cast<uInt>(rowBuffer.size());
		compress(Z_NO_FLUSH);
	}
	rowsWritten += rowCount;
}

void PngStripWriter::finish()
{
	if(finished || rowsWritten != height)
		throw std::logic_error("Incomplete PNG image");

	stream->avail_in = 0;
	compress(Z_FINISH);
	writeChunk("IEND", nullptr, 0);
	file.flush();
	finished = true;
}
