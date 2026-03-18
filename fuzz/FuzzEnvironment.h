/*
 * FuzzEnvironment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace fuzzing
{
class ByteReader
{
public:
	ByteReader(const uint8_t * data, size_t size);

	[[nodiscard]] bool empty() const;
	[[nodiscard]] size_t remaining() const;

	uint8_t readByte();
	bool readBool();
	uint32_t readU32();
	int readInt();
	int readInRange(int minValue, int maxValue);

	template<typename T, size_t N>
	T pick(const std::array<T, N> & choices)
	{
		static_assert(N > 0, "pick() requires a non-empty choices array");
		return choices[readU32() % N];
	}

private:
	const uint8_t * data;
	size_t size;
	size_t offset;
};

void initializeEngine();
}
