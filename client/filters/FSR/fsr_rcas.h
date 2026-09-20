/*
 * fsr_rcas.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */

#pragma once

#include <cstdint>

/// Scalar CPU port of AMD FidelityFX RCAS (Robust Contrast Adaptive Sharpening), following the
/// public FSR 1.0 reference algorithm (MIT-licensed by AMD, see
/// https://github.com/GPUOpen-Effects/FidelityFX-FSR). Unlike FSR's EASU upscale pass, RCAS reads
/// only a fixed 5-pixel neighborhood and keeps no history between pixels or frames, so - just like
/// xBRZ - it ports to plain per-pixel CPU code without needing a GPU at all.
namespace fsr
{
	/// Sharpens a single ARGB8888 image; src and dst must not overlap (dst is written a pixel at a
	/// time, but each pixel's result depends on its neighbors in src). Pixels use the same packed
	/// 0xAARRGGBB layout as xbrz::makePixel() / getAlpha() / etc.
	///
	/// sharpness follows AMD's own convention: 0 is the strongest setting, and every +1 halves the
	/// effect. Values around 0.2-0.5 match what FSR-based tools typically ship as their default.
	///
	/// Every output row only reads its own neighborhood from src, so - unlike xbrz::scale() -
	/// [yFirst, yLast) slices may be processed by different threads with no extra row padding.
	void sharpenRCAS(const uint32_t * src, uint32_t * dst, int width, int height, float sharpness,
					  int yFirst = 0, int yLast = -1);
}
