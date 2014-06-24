#include "StdInc.h"
#define INSTANTIATE_REGISTER_TYPES_HERE
#include "RegisterTypes.h"

#include "../mapping/CMapInfo.h"
#include "../StartInfo.h"
#include "../BattleState.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CModHandler.h"
#include "../mapObjects/CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../VCMI_Lib.h"
#include "../CArtHandler.h"
#include "../CHeroHandler.h"
#include "../CSpellHandler.h"
#include "../CTownHandler.h"
#include "../mapping/CCampaignHandler.h"
#include "../NetPacks.h"
#include "../mapObjects/CObjectClassesHandler.h"

// For reference: peak memory usage by gcc during compilation of register type templates
// registerTypesMapObjects:  1.9 Gb
// registerTypes2:  2.2 Gb
//     registerTypesClientPacks1 1.6 Gb
//     registerTypesClientPacks2 1.6 Gb
// registerTypesServerPacks:  1.3 Gb
// registerTypes4:  1.3 Gb

#define DEFINE_EXTERNAL_METHOD(METHODNAME) \
extern template DLL_LINKAGE void METHODNAME<CISer<CConnection>>(CISer<CConnection>& s); \
extern template DLL_LINKAGE void METHODNAME<COSer<CConnection>>(COSer<CConnection>& s); \
extern template DLL_LINKAGE void METHODNAME<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s); \
extern template DLL_LINKAGE void METHODNAME<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s); \
extern template DLL_LINKAGE void METHODNAME<CSaveFile>(CSaveFile & s); \
extern template DLL_LINKAGE void METHODNAME<CLoadFile>(CLoadFile & s); \
extern template DLL_LINKAGE void METHODNAME<CTypeList>(CTypeList & s); \
extern template DLL_LINKAGE void METHODNAME<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);

//DEFINE_EXTERNAL_METHOD(registerTypesMapObjects)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects1)
DEFINE_EXTERNAL_METHOD(registerTypesMapObjects2)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks1)
DEFINE_EXTERNAL_METHOD(registerTypesClientPacks2)
DEFINE_EXTERNAL_METHOD(registerTypesServerPacks)
DEFINE_EXTERNAL_METHOD(registerTypesPregamePacks)

template void registerTypes<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypes<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypes<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypes<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypes<CSaveFile>(CSaveFile & s);
template void registerTypes<CLoadFile>(CLoadFile & s);
template void registerTypes<CTypeList>(CTypeList & s);
template void registerTypes<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);
