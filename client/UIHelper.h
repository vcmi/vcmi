/*
 * UIHelper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "StdInc.h"

#include "../lib/CSoundBase.h"
#include "../lib/texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

struct MoveArtifactInfo;
struct Component;
class CArtifactSet;
class CGHeroInstance;
class CStackBasicDescriptor;

VCMI_LIB_NAMESPACE_END

namespace UIHelper
{
    std::vector<Component> getArtifactsComponents(const CArtifactSet & artSet, const std::vector<MoveArtifactInfo> & movedPack);
    std::vector<Component> getSpellsComponents(const std::set<SpellID> & spells);
    soundBase::soundID getNecromancyInfoWindowSound();
    std::string getNecromancyInfoWindowText(const CStackBasicDescriptor & stack);
    std::string getArtifactsInfoWindowText();
    std::string getEagleEyeInfoWindowText(const CGHeroInstance & hero, const std::set<SpellID> & spells);
}
