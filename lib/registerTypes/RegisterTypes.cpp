/*
 * RegisterTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#define INSTANTIATE_REGISTER_TYPES_HERE
#include "RegisterTypes.h"

#include "../mapping/CMapInfo.h"
#include "../StartInfo.h"
#include "../CGameState.h"
#include "../mapObjects/CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../spells/CSpellHandler.h"
#include "../mapping/CCampaignHandler.h"

#include "../serializer/BinaryDeserializer.h"
#include "../serializer/BinarySerializer.h"
#include "../serializer/CTypeList.h"

VCMI_LIB_NAMESPACE_BEGIN

// For reference: peak memory usage by gcc during compilation of register type templates
// registerTypesMapObjects:  1.9 Gb
// registerTypes2:  2.2 Gb
//     registerTypesClientPacks1 1.6 Gb
//     registerTypesClientPacks2 1.6 Gb
// registerTypesServerPacks:  1.3 Gb
// registerTypes4:  1.3 Gb


#define DEFINE_EXTERNAL_METHOD(METHODNAME) \
extern template DLL_LINKAGE void METHODNAME<BinaryDeserializer>(BinaryDeserializer & s); \
extern template DLL_LINKAGE void METHODNAME<BinarySerializer>(BinarySerializer & s); \
extern template DLL_LINKAGE void METHODNAME<CTypeList>(CTypeList & s); \

//DEFINE_EXTERNAL_METHOD(registerTypesMapObjects)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects1)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects2)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks1)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks2)
DEFINE_EXTERNAL_METHOD(registerTypesServerPacks)
DEFINE_EXTERNAL_METHOD(registerTypesLobbyPacks)

template void registerTypes<BinaryDeserializer>(BinaryDeserializer & s);
template void registerTypes<BinarySerializer>(BinarySerializer & s);
template void registerTypes<CTypeList>(CTypeList & s);

VCMI_LIB_NAMESPACE_END
