/*
 * TypesClientPacks2.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "RegisterTypes.h"

#include "../StartInfo.h"
#include "../CStack.h"
#include "../battle/BattleInfo.h"
#include "../mapObjects/CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../VCMI_Lib.h"
#include "../CArtHandler.h"
#include "../CHeroHandler.h"
#include "../spells/CSpellHandler.h"
#include "../CTownHandler.h"
#include "../NetPacks.h"

#include "../serializer/BinaryDeserializer.h"
#include "../serializer/BinarySerializer.h"
#include "../serializer/CTypeList.h"

VCMI_LIB_NAMESPACE_BEGIN


template void registerTypesClientPacks2<BinaryDeserializer>(BinaryDeserializer & s);
template void registerTypesClientPacks2<BinarySerializer>(BinarySerializer & s);
template void registerTypesClientPacks2<CTypeList>(CTypeList & s);



VCMI_LIB_NAMESPACE_END
