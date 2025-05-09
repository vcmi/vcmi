/*
 * Reward.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Reward.h"

#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../constants/StringConstants.h"
#include "../CSkillHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

void Rewardable::RewardRevealTiles::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeBool("hide", hide);
	handler.serializeInt("scoreSurface", scoreSurface);
	handler.serializeInt("scoreSubterra", scoreSubterra);
	handler.serializeInt("scoreWater", scoreWater);
	handler.serializeInt("scoreRock", scoreRock);
	handler.serializeInt("radius", radius);
}

Rewardable::Reward::Reward()
	: heroExperience(0)
	, heroLevel(0)
	, manaDiff(0)
	, manaPercentage(-1)
	, manaOverflowFactor(0)
	, movePoints(0)
	, movePercentage(-1)
	, primary(4, 0)
	, removeObject(false)
	, spellCast(SpellID::NONE, MasteryLevel::NONE)
{
}

Rewardable::Reward::~Reward() = default;

si32 Rewardable::Reward::calculateManaPoints(const CGHeroInstance * hero) const
{
	si32 manaScaled = hero->mana;
	if (manaPercentage >= 0)
		manaScaled = hero->manaLimit() * manaPercentage / 100;

	si32 manaMissing   = std::max(0, hero->manaLimit() - manaScaled);
	si32 manaGranted   = std::min(manaMissing, manaDiff);
	si32 manaOverflow  = manaDiff - manaGranted;
	si32 manaOverLimit = manaOverflow * manaOverflowFactor / 100;
	si32 manaOutput    = manaScaled + manaGranted + manaOverLimit;

	return manaOutput;
}

Component Rewardable::Reward::getDisplayedComponent(const CGHeroInstance * h) const
{
	std::vector<Component> comps;
	loadComponents(comps, h);

	if (!comps.empty())
		return comps.front();

	// Rewardable requested component that represent such rewards, to be used as button in UI selection dialog, e.g. Chest with its experience / money pick
	// However reward is either completely empty OR has no rewards that target hero can receive OR these rewards have no visible component (e.g. movement)
	// Such cases are unreachable in H3, however can be reached by mods
	logMod->warn("Failed to find displayed component for reward!");
	return Component(ComponentType::NONE, 0);
}

void Rewardable::Reward::loadComponents(std::vector<Component> & comps, const CGHeroInstance * h) const
{
	for (auto comp : extraComponents)
		comps.push_back(comp);
	
	for (auto & bonus : heroBonuses)
	{
		if (bonus.type == BonusType::MORALE)
			comps.emplace_back(ComponentType::MORALE, bonus.val);
		if (bonus.type == BonusType::LUCK)
			comps.emplace_back(ComponentType::LUCK, bonus.val);
	}
	
	if (heroExperience)
		comps.emplace_back(ComponentType::EXPERIENCE, static_cast<si32>(h ? h->calculateXp(heroExperience) : heroExperience));

	if (heroLevel)
		comps.emplace_back(ComponentType::LEVEL, heroLevel);

	if (manaDiff || manaPercentage >= 0)
		comps.emplace_back(ComponentType::MANA, h ? (calculateManaPoints(h) - h->mana) : manaDiff);

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] != 0)
			comps.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill(i), primary[i]);
	}

	for(const auto & entry : secondary)
	{
		auto skillID = entry.first;
		int levelsGained = entry.second;
		int currentLevel = h ? h->getSecSkillLevel(skillID) : 0;
		int finalLevel = std::min(static_cast<int>(MasteryLevel::EXPERT), currentLevel + levelsGained);
		comps.emplace_back(ComponentType::SEC_SKILL, entry.first, finalLevel);
	}

	for(const auto & entry : grantedArtifacts)
		comps.emplace_back(ComponentType::ARTIFACT, entry);

	for(const auto & entry : takenArtifacts)
		comps.emplace_back(ComponentType::ARTIFACT, entry);

	for(const auto & entry : takenArtifactSlots)
	{
		if (h)
		{
			const auto & slotContent = h->getSlot(entry);
			if (slotContent->artifactID.hasValue())
				comps.emplace_back(ComponentType::ARTIFACT, slotContent->getArt()->getTypeId());
		}
	}

	for(const SpellID & spell : grantedScrolls)
		comps.emplace_back(ComponentType::SPELL, spell);

	for(const SpellID & spell : takenScrolls)
		comps.emplace_back(ComponentType::SPELL, spell);

	for(const auto & entry : spells)
	{
		bool learnable = !h || h->canLearnSpell(entry.toEntity(LIBRARY), true);
		comps.emplace_back(ComponentType::SPELL, entry, learnable ?	0 : -1);
	}

	for(const auto & entry : creatures)
		comps.emplace_back(ComponentType::CREATURE, entry.getId(), entry.getCount());

	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.emplace_back(ComponentType::RESOURCE, GameResID(i), resources[i]);
	}
}

void Rewardable::Reward::serializeJson(JsonSerializeFormat & handler)
{
	resources.serializeJson(handler, "resources");
	handler.serializeBool("removeObject", removeObject);
	handler.serializeInt("manaPercentage", manaPercentage);
	handler.serializeInt("movePercentage", movePercentage);
	handler.serializeInt("heroExperience", heroExperience);
	handler.serializeInt("heroLevel", heroLevel);
	handler.serializeInt("manaDiff", manaDiff);
	handler.serializeInt("manaOverflowFactor", manaOverflowFactor);
	handler.serializeInt("movePoints", movePoints);
	handler.serializeIdArray("artifacts", grantedArtifacts);
	handler.serializeIdArray("takenArtifacts", takenArtifacts);
	handler.serializeIdArray("takenArtifactSlots", takenArtifactSlots);
	handler.serializeIdArray("scrolls", grantedScrolls);
	handler.serializeIdArray("takenScrolls", takenScrolls);
	handler.serializeIdArray("spells", spells);
	handler.enterArray("creatures").serializeStruct(creatures);
	handler.enterArray("primary").serializeArray(primary);
	{
		auto a = handler.enterArray("secondary");
		std::vector<std::pair<SecondarySkill, si32>> fieldValue(secondary.begin(), secondary.end());
		a.serializeStruct<std::pair<SecondarySkill, si32>>(fieldValue, [](JsonSerializeFormat & h, std::pair<SecondarySkill, si32> & e)
		{
			h.serializeId("skill", e.first);
			h.serializeId("level", e.second, 0, [](const std::string & i){return vstd::find_pos(NSecondarySkill::levels, i);}, [](si32 i){return NSecondarySkill::levels.at(i);});
		});
		a.syncSize(fieldValue);
		secondary = std::map<SecondarySkill, si32>(fieldValue.begin(), fieldValue.end());
	}
	
	{
		auto a = handler.enterArray("creaturesChange");
		std::vector<std::pair<CreatureID, CreatureID>> fieldValue(creaturesChange.begin(), creaturesChange.end());
		a.serializeStruct<std::pair<CreatureID, CreatureID>>(fieldValue, [](JsonSerializeFormat & h, std::pair<CreatureID, CreatureID> & e)
		{
			h.serializeId("creature", e.first, CreatureID{});
			h.serializeId("amount", e.second, CreatureID{});
		});
		creaturesChange = std::map<CreatureID, CreatureID>(fieldValue.begin(), fieldValue.end());
	}
	
	{
		auto a = handler.enterStruct("spellCast");
		a->serializeId("spell", spellCast.first, SpellID{});
		a->serializeInt("level", spellCast.second);
	}
}

VCMI_LIB_NAMESPACE_END
