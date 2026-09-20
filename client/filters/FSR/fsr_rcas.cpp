/*
 * fsr_rcas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#include "fsr_rcas.h"

#include <vector>

#include "../xBRZ/xbrz_tools.h"

#include <algorithm>
#include <cmath>

using namespace xbrz;

namespace
{
	struct RGB
	{
		float r, g, b;
	};

	RGB minRGB(const RGB & a, const RGB & b) { return { std::min(a.r, b.r), std::min(a.g, b.g), std::min(a.b, b.b) }; }
	RGB maxRGB(const RGB & a, const RGB & b) { return { std::max(a.r, b.r), std::max(a.g, b.g), std::max(a.b, b.b) }; }

	uint32_t sampleClamped(const uint32_t * src, int width, int height, int x, int y)
	{
		x = std::clamp(x, 0, width - 1);
		y = std::clamp(y, 0, height - 1);
		return src[static_cast<size_t>(y) * width + x];
	}

	// Local contrast ratio bounding how far the sharpening lobe may push a channel, so it cannot
	// push a pixel past the min/max of its own neighborhood (i.e. no ringing beyond the source data).
	float lobeFor(float mn, float mx)
	{
		const float hitMin = mn / (4.0f * mx + 1e-6f);
		const float hitMax = (1.0f - mx) / (4.0f * mn - 4.0f - 1e-6f);
		return std::max(-hitMin, hitMax);
	}
}

void fsr::sharpenRCAS(const uint32_t * src, uint32_t * dst, int width, int height, float sharpness,
					   int yFirst, int yLast)
{
	// AMD's own limit constant - the response curve is only well-conditioned within this range
	constexpr float rcasLimit = 0.25f - 1.0f / 16.0f;

	const float con = std::exp2(-sharpness);

	if (yLast < 0 || yLast > height)
		yLast = height;

	for (int y = yFirst; y < yLast; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			const uint32_t centerPixel = sampleClamped(src, width, height, x, y);
			const RGB e = { static_cast<float>(getRed(centerPixel)), static_cast<float>(getGreen(centerPixel)), static_cast<float>(getBlue(centerPixel)) };

			// Fully transparent neighbors typically hold don't-care color data (e.g. outside the
			// original sprite silhouette) - pulling them in would bleed garbage colors across the
			// alpha edge, so such a neighbor is treated as if it repeated the center pixel instead.
			auto sampleNeighbor = [&](int nx, int ny) -> RGB
			{
				const uint32_t pixel = sampleClamped(src, width, height, nx, ny);
				if (getAlpha(pixel) == 0)
					return e;
				return { static_cast<float>(getRed(pixel)), static_cast<float>(getGreen(pixel)), static_cast<float>(getBlue(pixel)) };
			};

			const RGB b = sampleNeighbor(x, y - 1);
			const RGB d = sampleNeighbor(x - 1, y);
			const RGB f = sampleNeighbor(x + 1, y);
			const RGB h = sampleNeighbor(x, y + 1);

			const RGB mn4 = minRGB(minRGB(b, d), minRGB(f, h));
			const RGB mx4 = maxRGB(maxRGB(b, d), maxRGB(f, h));

			const float lobeR = lobeFor(mn4.r / 255.0f, mx4.r / 255.0f);
			const float lobeG = lobeFor(mn4.g / 255.0f, mx4.g / 255.0f);
			const float lobeB = lobeFor(mn4.b / 255.0f, mx4.b / 255.0f);

			float lobe = std::max({ lobeR, lobeG, lobeB });
			lobe = std::max(-rcasLimit, std::min(lobe, 0.0f)) * con;

			auto blend = [lobe](float be, float de, float fe, float he, float ee)
			{
				return (lobe * (be + de + fe + he) + ee) / (4.0f * lobe + 1.0f);
			};

			const float r = std::clamp(blend(b.r, d.r, f.r, h.r, e.r), 0.0f, 255.0f);
			const float g = std::clamp(blend(b.g, d.g, f.g, h.g, e.g), 0.0f, 255.0f);
			const float bl = std::clamp(blend(b.b, d.b, f.b, h.b, e.b), 0.0f, 255.0f);

			dst[static_cast<size_t>(y) * width + x] = makePixel(
				getAlpha(centerPixel),
				static_cast<unsigned char>(r + 0.5f),
				static_cast<unsigned char>(g + 0.5f),
				static_cast<unsigned char>(bl + 0.5f));
		}
	}
}
