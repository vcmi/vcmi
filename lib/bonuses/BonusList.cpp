/*
 * BonusList.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CBonusSystemNode.h"

VCMI_LIB_NAMESPACE_BEGIN

BonusList::BonusList(bool BelongsToTree) : belongsToTree(BelongsToTree)
{
}

BonusList::BonusList(const BonusList & bonusList): belongsToTree(false)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
}

BonusList::BonusList(BonusList && other) noexcept: belongsToTree(false)
{
	std::swap(belongsToTree, other.belongsToTree);
	std::swap(bonuses, other.bonuses);
}

BonusList& BonusList::operator=(const BonusList &bonusList)
{
	bonuses.resize(bonusList.size());
	std::copy(bonusList.begin(), bonusList.end(), bonuses.begin());
	belongsToTree = false;
	return *this;
}

void BonusList::changed() const
{
    if(belongsToTree)
		CBonusSystemNode::treeHasChanged();
}

void BonusList::stackBonuses()
{
	boost::sort(bonuses, [](const std::shared_ptr<Bonus> & b1, const std::shared_ptr<Bonus> & b2) -> bool
	{
		if(b1 == b2)
			return false;
#define COMPARE_ATT(ATT) if(b1->ATT != b2->ATT) return b1->ATT < b2->ATT
		COMPARE_ATT(stacking);
		COMPARE_ATT(type);
		COMPARE_ATT(subtype);
		COMPARE_ATT(valType);
#undef COMPARE_ATT
		return b1->val > b2->val;
	});
	// remove non-stacking
	size_t next = 1;
	while(next < bonuses.size())
	{
		bool remove = false;
		std::shared_ptr<Bonus> last = bonuses[next-1];
		std::shared_ptr<Bonus> current = bonuses[next];

		if(current->stacking.empty())
			remove = current == last;
		else if(current->stacking == "ALWAYS")
			remove = false;
		else
			remove = current->stacking == last->stacking
				&& current->type == last->type
				&& current->subtype == last->subtype
				&& current->valType == last->valType;

		if(remove)
			bonuses.erase(bonuses.begin() + next);
		else
			next++;
	}
}

int BonusList::totalValue() const
{
	struct BonusCollection
	{
		int base = 0;
		int percentToBase = 0;
		int percentToAll = 0;
		int additive = 0;
		int percentToSource = 0;
		int indepMin = std::numeric_limits<int>::max();
		int indepMax = std::numeric_limits<int>::min();
	};

	auto percent = [](int64_t base, int64_t percent) -> int {
		return static_cast<int>(std::clamp<int64_t>((base * (100 + percent)) / 100, std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
	};
	std::array <BonusCollection, Bonus::BonusSource::NUM_BONUS_SOURCE> sources = {};
	BonusCollection any;
	bool hasIndepMax = false;
	bool hasIndepMin = false;

	for(const auto & b : bonuses)
	{
		switch(b->valType)
		{
		case Bonus::BASE_NUMBER:
			sources[b->source].base += b->val;
			break;
		case Bonus::PERCENT_TO_ALL:
			sources[b->source].percentToAll += b->val;
			break;
		case Bonus::PERCENT_TO_BASE:
			sources[b->source].percentToBase += b->val;
			break;
		case Bonus::PERCENT_TO_SOURCE:
			sources[b->source].percentToSource += b->val;
			break;
		case Bonus::PERCENT_TO_TARGET_TYPE:
			sources[b->targetSourceType].percentToSource += b->val;
			break;
		case Bonus::ADDITIVE_VALUE:
			sources[b->source].additive += b->val;
			break;
		case Bonus::INDEPENDENT_MAX:
			hasIndepMax = true;
			vstd::amax(sources[b->source].indepMax, b->val);
			break;
		case Bonus::INDEPENDENT_MIN:
			hasIndepMin = true;
			vstd::amin(sources[b->source].indepMin, b->val);
			break;
		}
	}
	for(const auto & src : sources)
	{
		any.base += percent(src.base, src.percentToSource);
		any.percentToBase += percent(src.percentToBase, src.percentToSource);
		any.percentToAll += percent(src.percentToAll, src.percentToSource);
		any.additive += percent(src.additive, src.percentToSource);
		if(hasIndepMin)
			vstd::amin(any.indepMin, percent(src.indepMin, src.percentToSource));
		if(hasIndepMax)
			vstd::amax(any.indepMax, percent(src.indepMax, src.percentToSource));
	}
	any.base = percent(any.base, any.percentToBase);
	any.base += any.additive;
	auto valFirst = percent(any.base ,any.percentToAll);

	if(hasIndepMin && hasIndepMax && any.indepMin < any.indepMax)
		any.indepMax = any.indepMin;

	const int notIndepBonuses = static_cast<int>(std::count_if(bonuses.cbegin(), bonuses.cend(), [](const std::shared_ptr<Bonus>& b)
	{
		return b->valType != Bonus::INDEPENDENT_MAX && b->valType != Bonus::INDEPENDENT_MIN;
	}));

	if(notIndepBonuses)
		return std::clamp(valFirst, any.indepMax, any.indepMin);

	return hasIndepMin ? any.indepMin : hasIndepMax ? any.indepMax : 0;
}

std::shared_ptr<Bonus> BonusList::getFirst(const CSelector &select)
{
	for (auto & b : bonuses)
	{
		if(select(b.get()))
			return b;
	}
	return nullptr;
}

std::shared_ptr<const Bonus> BonusList::getFirst(const CSelector &selector) const
{
	for(const auto & b : bonuses)
	{
		if(selector(b.get()))
			return b;
	}
	return nullptr;
}

void BonusList::getBonuses(BonusList & out, const CSelector &selector, const CSelector &limit) const
{
	out.reserve(bonuses.size());
	for(const auto & b : bonuses)
	{
		//add matching bonuses that matches limit predicate or have NO_LIMIT if no given predicate
		auto noFightLimit = b->effectRange == Bonus::NO_LIMIT;
		if(selector(b.get()) && ((!limit && noFightLimit) || ((bool)limit && limit(b.get()))))
			out.push_back(b);
	}
}

void BonusList::getAllBonuses(BonusList &out) const
{
	for(const auto & b : bonuses)
		out.push_back(b);
}

int BonusList::valOfBonuses(const CSelector &select) const
{
	BonusList ret;
	CSelector limit = nullptr;
	getBonuses(ret, select, limit);
	return ret.totalValue();
}

JsonNode BonusList::toJsonNode() const
{
	JsonNode node(JsonNode::JsonType::DATA_VECTOR);
	for(const std::shared_ptr<Bonus> & b : bonuses)
		node.Vector().push_back(b->toJsonNode());
	return node;
}

void BonusList::push_back(const std::shared_ptr<Bonus> & x)
{
	bonuses.push_back(x);
	changed();
}

BonusList::TInternalContainer::iterator BonusList::erase(const int position)
{
	changed();
	return bonuses.erase(bonuses.begin() + position);
}

void BonusList::clear()
{
	bonuses.clear();
	changed();
}

std::vector<BonusList *>::size_type BonusList::operator-=(const std::shared_ptr<Bonus> & i)
{
	auto itr = std::find(bonuses.begin(), bonuses.end(), i);
	if(itr == bonuses.end())
		return false;
	bonuses.erase(itr);
	changed();
	return true;
}

void BonusList::resize(BonusList::TInternalContainer::size_type sz, const std::shared_ptr<Bonus> & c)
{
	bonuses.resize(sz, c);
	changed();
}

void BonusList::reserve(TInternalContainer::size_type sz)
{
	bonuses.reserve(sz);
}

void BonusList::insert(BonusList::TInternalContainer::iterator position, BonusList::TInternalContainer::size_type n, const std::shared_ptr<Bonus> & x)
{
	bonuses.insert(position, n, x);
	changed();
}

DLL_LINKAGE std::ostream & operator<<(std::ostream &out, const BonusList &bonusList)
{
	for (ui32 i = 0; i < bonusList.size(); i++)
	{
		const auto & b = bonusList[i];
		out << "Bonus " << i << "\n" << *b << std::endl;
	}
	return out;
}

VCMI_LIB_NAMESPACE_END