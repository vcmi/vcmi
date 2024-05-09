/*
 * BinaryDeserializer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BinaryDeserializer.h"
#include "../registerTypes/RegisterTypes.h"

VCMI_LIB_NAMESPACE_BEGIN

BinaryDeserializer::BinaryDeserializer(IBinaryReader * r): CLoaderBase(r)
{
	saving = false;
	version = Version::NONE;
	smartPointerSerialization = true;
	reverseEndianness = false;

	registerTypes(*this);
}

VCMI_LIB_NAMESPACE_END
