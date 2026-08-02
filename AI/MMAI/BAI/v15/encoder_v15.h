/*
 * encoder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "schema/v15/types.h"
#include "vstd/CLoggerBase.h"

namespace MMAI::BAI::V15::Encoder
{
inline void Warn(Schema::V15::Encoding e, const std::string_view attrname, int a, int n, int vmax, int v)
{
	// Warn at most once every 600s
	auto now = std::chrono::steady_clock::now();
	static thread_local std::map<std::string_view, std::map<int, std::chrono::steady_clock::time_point>> warns;
	const auto & warned_at = warns[attrname][a];

	if(std::chrono::duration_cast<std::chrono::seconds>(now - warned_at) > std::chrono::seconds(600))
	{
		// This is not critical; the value will be capped to vmax (should not occur often)
		logAi->info("MMAI: Attribute value out of bounds: v=%d (vmax=|%d|, a=%d, e=%d, n=%d, attrname=%s)\n", v, vmax, a, static_cast<int>(e), n, attrname);
		warns[attrname][a] = now;
	}
}

template<typename EncTraits>
int Encode(const std::array<int, EncTraits::attr_count> & attrs, std::span<float> out)
{
	using Encoding = Schema::V15::Encoding;

	int i = 0;

	for(int iraw = 0; iraw < attrs.size(); ++iraw)
	{
		static_assert(EncTraits::encoding.size() == EncTraits::attr_count);
		const auto & [_, e, n, vmax] = EncTraits::encoding[iraw];
		int v = attrs[iraw];

		if(v > vmax || v < -vmax)
		{
			Warn(e, EncTraits::name, iraw, n, vmax, v);
			v = std::clamp<int>(v, -vmax, vmax);
		}

		switch(e)
		{
			case Encoding::LINNORM:
				// XXX: this is a simplified version for 0..1 norm
				out[i] = (static_cast<float>(v) / static_cast<float>(vmax));
				break;
			case Encoding::CATEGORICAL:
				assert(v >= 0 && v < n);
				out[i + v] = 1.0f;
				break;
			case Encoding::RAW:
				out[i] = static_cast<float>(v);
				break;
			default:
				throwf("Unexpected Encoding: {}", EI(e));
		}

		i += n;
	}

	return i;
}
};
