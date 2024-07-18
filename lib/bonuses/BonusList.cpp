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
#include "../json/JsonNode.h"

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
	if (bonuses.empty())
		return 0;

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

	auto applyPercentage = [](int base, int percent) -> int {
		return (static_cast<int64_t>(base) * (100 + percent)) / 100;
	};

	BonusCollection accumulated;
	bool hasIndepMax = false;
	bool hasIndepMin = false;

	std::array<int, vstd::to_underlying(BonusSource::NUM_BONUS_SOURCE)> percentToSource = {};

	for(const auto & b : bonuses)
	{
		switch(b->valType)
		{
		case BonusValueType::PERCENT_TO_SOURCE:
			percentToSource[vstd::to_underlying(b->source)] += b->val;
		break;
		case BonusValueType::PERCENT_TO_TARGET_TYPE:
			percentToSource[vstd::to_underlying(b->targetSourceType)] += b->val;
			break;
		}
	}

	for(const auto & b : bonuses)
	{
		int sourceIndex = vstd::to_underlying(b->source);
		int valModified	= applyPercentage(b->val, percentToSource[sourceIndex]);

		switch(b->valType)
		{
		case BonusValueType::BASE_NUMBER:
			accumulated.base += valModified;
			break;
		case BonusValueType::PERCENT_TO_ALL:
			accumulated.percentToAll += valModified;
			break;
		case BonusValueType::PERCENT_TO_BASE:
			accumulated.percentToBase += valModified;
			break;
		case BonusValueType::ADDITIVE_VALUE:
			accumulated.additive += valModified;
			break;
		case BonusValueType::INDEPENDENT_MAX:
			hasIndepMax = true;
			vstd::amax(accumulated.indepMax, valModified);
			break;
		case BonusValueType::INDEPENDENT_MIN:
			hasIndepMin = true;
			vstd::amin(accumulated.indepMin, valModified);
			break;
		}
	}

	accumulated.base = applyPercentage(accumulated.base, accumulated.percentToBase);
	accumulated.base += accumulated.additive;
	auto valFirst = applyPercentage(accumulated.base ,accumulated.percentToAll);

	if(hasIndepMin && hasIndepMax && accumulated.indepMin < accumulated.indepMax)
		accumulated.indepMax = accumulated.indepMin;

	const int notIndepBonuses = static_cast<int>(std::count_if(bonuses.cbegin(), bonuses.cend(), [](const std::shared_ptr<Bonus>& b)
	{
		return b->valType != BonusValueType::INDEPENDENT_MAX && b->valType != BonusValueType::INDEPENDENT_MIN;
	}));

	if(notIndepBonuses)
		return std::clamp(valFirst, accumulated.indepMax, accumulated.indepMin);

	if (hasIndepMin)
		return accumulated.indepMin;

	if (hasIndepMax)
		return accumulated.indepMax;

	return 0;
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
		if(selector(b.get()) && (!limit || ((bool)limit && limit(b.get()))))
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
	JsonNode node;
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
