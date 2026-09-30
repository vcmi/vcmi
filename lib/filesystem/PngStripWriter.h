/*
 * PngStripWriter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <fstream>

struct z_stream_s;

/// Writes RGBA PNG file a few rows at a time, so that huge images do not have to be kept in memory
class DLL_LINKAGE PngStripWriter
{
	std::ofstream file;
	std::unique_ptr<z_stream_s> stream;
	std::vector<uint8_t> rowBuffer;
	std::vector<uint8_t> outBuffer;
	uint32_t width;
	uint32_t height;
	uint32_t rowsWritten = 0;
	bool finished = false;

	void writeChunk(const char * type, const uint8_t * data, uint32_t size);
	void compress(int flushMode);

public:
	PngStripWriter(const boost::filesystem::path & path, uint32_t width, uint32_t height);
	~PngStripWriter();

	/// Appends rows, each of width * 4 bytes in RGBA order
	void writeRows(const uint8_t * rgba, uint32_t rowCount);

	/// Must be called after the last row
	void finish();
};
