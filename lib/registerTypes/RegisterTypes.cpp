#include "StdInc.h"
#define INSTANTIATE_REGISTER_TYPES_HERE
#include "RegisterTypes.h"

#include "../mapping/CMapInfo.h"
#include "../StartInfo.h"
#include "../BattleState.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../spells/CSpellHandler.h"
#include "../mapping/CCampaignHandler.h"

// For reference: peak memory usage by gcc during compilation of register type templates
// registerTypesMapObjects:  1.9 Gb
// registerTypes2:  2.2 Gb
//     registerTypesClientPacks1 1.6 Gb
//     registerTypesClientPacks2 1.6 Gb
// registerTypesServerPacks:  1.3 Gb
// registerTypes4:  1.3 Gb


#define DEFINE_EXTERNAL_METHOD(METHODNAME) \
extern template DLL_LINKAGE void METHODNAME<CISer>(CISer & s); \
extern template DLL_LINKAGE void METHODNAME<COSer>(COSer & s); \
extern template DLL_LINKAGE void METHODNAME<CTypeList>(CTypeList & s); \

//DEFINE_EXTERNAL_METHOD(registerTypesMapObjects)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects1)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects2)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks1)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks2)
DEFINE_EXTERNAL_METHOD(registerTypesServerPacks)
DEFINE_EXTERNAL_METHOD(registerTypesPregamePacks)

template void registerTypes<CISer>(CISer & s);
template void registerTypes<COSer>(COSer & s);
template void registerTypes<CTypeList>(CTypeList & s);
