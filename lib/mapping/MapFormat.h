/*
 * MapFormat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class EMapFormat : uint8_t
{
	INVALID = 0,
	//       HEX    DEC
	ROE   = 0x0e, // 14
	AB    = 0x15, // 21
	SOD   = 0x1c, // 28
//	CHR   = 0x1d, // 29 Heroes Chronicles, presumably - identical to SoD, untested
	HOTA  = 0x20, // 32
	WOG   = 0x33, // 51
	VCMI  = 0x64
};

VCMI_LIB_NAMESPACE_END
