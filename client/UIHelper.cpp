/*
 * UIHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "UIHelper.h"

#include "widgets/CComponent.h"

#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/networkPacks/ArtifactLocation.h"
#include "../lib/CRandomGenerator.h"

std::vector<Component> UIHelper::getArtifactsComponents(const CArtifactSet & artSet, const std::vector<MoveArtifactInfo> & movedPack)
{
	std::vector<Component> components;
	for(const auto & artMoveInfo : movedPack)
	{
		const auto art = artSet.getArt(artMoveInfo.dstPos);
		assert(art);

		if(art->isScroll())
			components.emplace_back(ComponentType::SPELL_SCROLL, art->getScrollSpellID());
		else
			components.emplace_back(ComponentType::ARTIFACT, art->getTypeId());
	}
	return components;
}

std::vector<Component> UIHelper::getSpellsComponents(const std::set<SpellID> & spells)
{
	std::vector<Component> components;
	for(const auto & spell : spells)
		components.emplace_back(ComponentType::SPELL, spell);
	return components;
}

soundBase::soundID UIHelper::getNecromancyInfoWindowSound()
{
	return soundBase::soundID(soundBase::pickup01 + CRandomGenerator::getDefault().nextInt(6));
}

std::string UIHelper::getNecromancyInfoWindowText(const CStackBasicDescriptor & stack)
{
	MetaString text;
	if(stack.getCount() > 1) // Practicing the dark arts of necromancy, ... (plural)
	{
		text.appendLocalString(EMetaText::GENERAL_TXT, 145);
		text.replaceNumber(stack.getCount());
	}
	else // Practicing the dark arts of necromancy, ... (singular)
	{
		text.appendLocalString(EMetaText::GENERAL_TXT, 146);
	}
	text.replaceName(stack);
	return text.toString();
}

std::string UIHelper::getArtifactsInfoWindowText()
{
	MetaString text;
	text.appendLocalString(EMetaText::GENERAL_TXT, 30);
	return text.toString();
}

std::string UIHelper::getEagleEyeInfoWindowText(const CGHeroInstance & hero, const std::set<SpellID> & spells)
{
	MetaString text;
	text.appendLocalString(EMetaText::GENERAL_TXT, 221); // Through eagle-eyed observation, %s is able to learn %s
	text.replaceRawString(hero.getNameTranslated());

	auto curSpell = spells.begin();
	text.replaceName(*curSpell++);
	for(int i = 1; i < spells.size(); i++, curSpell++)
	{
		if(i + 1 == spells.size())
			text.appendLocalString(EMetaText::GENERAL_TXT, 141); // " and "
		else
			text.appendRawString(", ");
		text.appendName(*curSpell);
	}
	text.appendRawString(".");
	return text.toString();
}
