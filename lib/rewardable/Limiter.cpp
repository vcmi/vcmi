/*
 * Limiter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Limiter.h"

#include "../IGameCallback.h"
#include "../CPlayerState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../networkPacks/Component.h"
#include "../serializer/JsonSerializeFormat.h"
#include "../constants/StringConstants.h"
#include "../CHeroHandler.h"
#include "../CSkillHandler.h"
#include "../ArtifactUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

Rewardable::Limiter::Limiter()
	: dayOfWeek(0)
	, daysPassed(0)
	, heroExperience(0)
	, heroLevel(-1)
	, manaPercentage(0)
	, manaPoints(0)
	, canLearnSkills(false)
	, primary(GameConstants::PRIMARY_SKILLS, 0)
{
}

Rewardable::Limiter::~Limiter() = default;

bool operator==(const Rewardable::Limiter & l, const Rewardable::Limiter & r)
{
	return l.dayOfWeek == r.dayOfWeek
	&& l.daysPassed == r.daysPassed
	&& l.heroLevel == r.heroLevel
	&& l.heroExperience == r.heroExperience
	&& l.manaPoints == r.manaPoints
	&& l.manaPercentage == r.manaPercentage
	&& l.secondary == r.secondary
	&& l.canLearnSkills == r.canLearnSkills
	&& l.creatures == r.creatures
	&& l.spells == r.spells
	&& l.artifacts == r.artifacts
	&& l.players == r.players
	&& l.heroes == r.heroes
	&& l.heroClasses == r.heroClasses
	&& l.resources == r.resources
	&& l.primary == r.primary
	&& l.noneOf == r.noneOf
	&& l.allOf == r.allOf
	&& l.anyOf == r.anyOf;
}

bool operator!=(const Rewardable::Limiter & l, const Rewardable::Limiter & r)
{
	return !(l == r);
}

bool Rewardable::Limiter::heroAllowed(const CGHeroInstance * hero) const
{
	if(dayOfWeek != 0)
	{
		if (hero->cb->getDate(Date::DAY_OF_WEEK) != dayOfWeek)
			return false;
	}

	if(daysPassed != 0)
	{
		if (hero->cb->getDate(Date::DAY) < daysPassed)
			return false;
	}

	for(const auto & reqStack : creatures)
	{
		size_t count = 0;
		for(const auto & slot : hero->Slots())
		{
			const CStackInstance * heroStack = slot.second;
			if (heroStack->type == reqStack.type)
				count += heroStack->count;
		}
		if (count < reqStack.count) //not enough creatures of this kind
			return false;
	}

	if(!hero->cb->getPlayerState(hero->tempOwner)->resources.canAfford(resources))
		return false;

	if(heroLevel > static_cast<si32>(hero->level))
		return false;

	if(static_cast<TExpType>(heroExperience) > hero->exp)
		return false;

	if(manaPoints > hero->mana)
		return false;

	if (canLearnSkills && !hero->canLearnSkill())
		return false;

	if(manaPercentage > 100 * hero->mana / hero->manaLimit())
		return false;

	for(size_t i=0; i<primary.size(); i++)
	{
		if(primary[i] > hero->getPrimSkillLevel(static_cast<PrimarySkill>(i)))
			return false;
	}

	for(const auto & skill : secondary)
	{
		if (skill.second > hero->getSecSkillLevel(skill.first))
			return false;
	}

	for(const auto & spell : spells)
	{
		if (!hero->spellbookContainsSpell(spell))
			return false;
	}

	for(const auto & spell : canLearnSpells)
	{
		if (!hero->canLearnSpell(spell.toEntity(VLC), true))
			return false;
	}

	{
		std::unordered_map<ArtifactID, unsigned int, ArtifactID::hash> artifactsRequirements; // artifact ID -> required count
		for(const auto & art : artifacts)
			++artifactsRequirements[art];
		
		size_t reqSlots = 0;
		for(const auto & elem : artifactsRequirements)
		{
			// check required amount of artifacts
			if(hero->getArtPosCount(elem.first, false, true, true) < elem.second)
				return false;
			if(!hero->hasArt(elem.first))
				reqSlots += hero->getAssemblyByConstituent(elem.first)->getPartsInfo().size() - 2;
		}
		if(!ArtifactUtils::isBackpackFreeSlots(hero, reqSlots))
			return false;
	}
	
	if(!players.empty() && !vstd::contains(players, hero->getOwner()))
		return false;
	
	if(!heroes.empty() && !vstd::contains(heroes, hero->type->getId()))
		return false;
	
	if(!heroClasses.empty() && !vstd::contains(heroClasses, hero->type->heroClass->getId()))
		return false;
		
	
	for(const auto & sublimiter : noneOf)
	{
		if (sublimiter->heroAllowed(hero))
			return false;
	}

	for(const auto & sublimiter : allOf)
	{
		if (!sublimiter->heroAllowed(hero))
			return false;
	}

	if(anyOf.empty())
		return true;

	for(const auto & sublimiter : anyOf)
	{
		if (sublimiter->heroAllowed(hero))
			return true;
	}
	return false;
}

void Rewardable::Limiter::loadComponents(std::vector<Component> & comps,
								 const CGHeroInstance * h) const
{
	if (heroExperience)
		comps.emplace_back(ComponentType::EXPERIENCE, static_cast<si32>(h ? h->calculateXp(heroExperience) : heroExperience));

	if (heroLevel > 0)
		comps.emplace_back(ComponentType::EXPERIENCE, heroLevel);

	if (manaPoints || manaPercentage > 0)
	{
		int absoluteMana = (h && h->manaLimit()) ? (manaPercentage * h->mana / h->manaLimit() / 100) : 0;
		comps.emplace_back(ComponentType::MANA, absoluteMana + manaPoints);
	}

	for (size_t i=0; i<primary.size(); i++)
	{
		if (primary[i] != 0)
			comps.emplace_back(ComponentType::PRIM_SKILL, PrimarySkill(i), primary[i]);
	}

	for(const auto & entry : secondary)
		comps.emplace_back(ComponentType::SEC_SKILL, entry.first, entry.second);

	for(const auto & entry : artifacts)
		comps.emplace_back(ComponentType::ARTIFACT, entry);

	for(const auto & entry : spells)
		comps.emplace_back(ComponentType::SPELL, entry);

	for(const auto & entry : creatures)
		comps.emplace_back(ComponentType::CREATURE, entry.type->getId(), entry.count);
	
	for(const auto & entry : players)
		comps.emplace_back(ComponentType::FLAG, entry);
	
	for(const auto & entry : heroes)
		comps.emplace_back(ComponentType::HERO_PORTRAIT, entry);
	
	for (size_t i=0; i<resources.size(); i++)
	{
		if (resources[i] !=0)
			comps.emplace_back(ComponentType::RESOURCE, GameResID(i), resources[i]);
	}
}

void Rewardable::Limiter::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("dayOfWeek", dayOfWeek);
	handler.serializeInt("daysPassed", daysPassed);
	resources.serializeJson(handler, "resources");
	handler.serializeInt("manaPercentage", manaPercentage);
	handler.serializeInt("heroExperience", heroExperience);
	handler.serializeInt("heroLevel", heroLevel);
	handler.serializeIdArray("heroes", heroes);
	handler.serializeIdArray("heroClasses", heroClasses);
	handler.serializeIdArray("colors", players);
	handler.serializeInt("manaPoints", manaPoints);
	handler.serializeIdArray("artifacts", artifacts);
	handler.enterArray("creatures").serializeStruct(creatures);
	handler.enterArray("primary").serializeArray(primary);
	{
		auto a = handler.enterArray("secondary");
		std::vector<std::pair<SecondarySkill, si32>> fieldValue(secondary.begin(), secondary.end());
		a.serializeStruct<std::pair<SecondarySkill, si32>>(fieldValue, [](JsonSerializeFormat & h, std::pair<SecondarySkill, si32> & e)
		{
			h.serializeId("skill", e.first, SecondarySkill(SecondarySkill::NONE));
			h.serializeId("level", e.second, 0, [](const std::string & i){return vstd::find_pos(NSecondarySkill::levels, i);}, [](si32 i){return NSecondarySkill::levels.at(i);});
		});
		a.syncSize(fieldValue);
		secondary = std::map<SecondarySkill, si32>(fieldValue.begin(), fieldValue.end());
	}
	//sublimiters
	auto serializeSublimitersList = [&handler](const std::string & field, LimitersList & container)
	{
		auto a = handler.enterArray(field);
		a.syncSize(container);
		for(int i = 0; i < container.size(); ++i)
		{
			if(!handler.saving)
				container[i] = std::make_shared<Rewardable::Limiter>();
			auto e = a.enterStruct(i);
			container[i]->serializeJson(handler);
		}
	};
	serializeSublimitersList("allOf", allOf);
	serializeSublimitersList("anyOf", anyOf);
	serializeSublimitersList("noneOf", noneOf);
}

VCMI_LIB_NAMESPACE_END
