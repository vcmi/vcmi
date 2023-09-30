/*
* ShrineInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ShrineInstanceConstructor.h"

#include "../mapObjects/MiscObjects.h"
#include "../JsonRandom.h"
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

void ShrineInstanceConstructor::initTypeData(const JsonNode & config)
{
	parameters = config;
}

void ShrineInstanceConstructor::randomizeObject(CGShrine * shrine, CRandomGenerator & rng) const
{
	JsonRandom::Variables emptyVariables;

	auto visitTextParameter = parameters["visitText"];

	if (visitTextParameter.isNumber())
		shrine->visitText.appendLocalString(EMetaText::ADVOB_TXT, static_cast<ui32>(visitTextParameter.Float()));
	else
		shrine->visitText.appendRawString(visitTextParameter.String());

	if(shrine->spell == SpellID::NONE) // shrine has no predefined spell
		shrine->spell =JsonRandom::loadSpell(parameters["spell"], rng, emptyVariables);
}

VCMI_LIB_NAMESPACE_END
