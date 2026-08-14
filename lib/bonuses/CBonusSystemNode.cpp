/*
 * CBonusSystemNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CBonusSystemNode.h"
#include "Limiters.h"
#include "Updaters.h"
#include "Propagators.h"

constexpr bool cachingEnabled = true;
static std::atomic<int32_t> globalCounter = 1;

std::shared_ptr<Bonus> CBonusSystemNode::getLocalBonus(const CSelector & selector)
{
	auto ret = bonuses.getFirst(selector);
	if(ret)
		return ret;
	return nullptr;
}

std::shared_ptr<const Bonus> CBonusSystemNode::getFirstBonus(const CSelector & selector) const
{
	for(const auto & bonus : bonuses)
	{
		auto updated = bonus;
		if(!propagationRecords.empty())
			updated = applyPropagationUpdater(updated);
		if(selector(updated.get()))
			return updated;
	}

	TCNodes lparents;
	getDirectParents(lparents);
	for(const CBonusSystemNode *pname : lparents)
	{
		auto ret = pname->getFirstBonus(selector);
		if (ret)
			return ret;
	}

	return nullptr;
}

void CBonusSystemNode::getDirectParents(TCNodes & out) const /*retrieves list of parent nodes (nodes to inherit bonuses from) */
{
	for(const auto * elem : parentsToInherit)
		out.insert(elem);
}

void CBonusSystemNode::getAllBonusesRec(BonusList &out) const
{
	BonusList beforeUpdate;

	for(const auto * parent : parentsToInherit)
		parent->getAllBonusesRec(beforeUpdate);

	bonuses.getAllBonuses(beforeUpdate);

	if(propagationRecords.empty())
	{
		for(const auto & b : beforeUpdate)
		{
			auto updated = b->updater ? getUpdatedBonus(b, b->updater) : b;
			out.push_back(updated);
		}
	}
	else
	{
		for(const auto & b : beforeUpdate)
		{
			auto propagated = applyPropagationUpdater(b);
			auto updated = propagated->updater ? getUpdatedBonus(propagated, propagated->updater) : propagated;
			out.push_back(updated);
		}
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonuses(const CSelector &selector, const std::string &cachingStr) const
{
	if (cachingEnabled)
	{
		// If a bonus system request comes with a caching string then look up in the map if there are any
		// pre-calculated bonus results. Limiters can't be cached so they have to be calculated.
		if (cachedLast == nodeChanged && !cachingStr.empty())
		{
			RequestsMap::const_accessor accessor;

			//Cached list contains bonuses for our query with applied limiters
			if (cachedRequests.find(accessor, cachingStr) && accessor->second.first == cachedLast)
				return accessor->second.second;
		}

		//We still don't have the bonuses (didn't returned them from cache)
		//Perform bonus selection
		auto ret = std::make_shared<BonusList>();

		if (cachedLast == nodeChanged)
		{
			// Cached bonuses are up-to-date - use shared/read access and compute results
			std::shared_lock lock(sync);
			cachedBonuses.getBonuses(*ret, selector);
		}
		else
		{
			// If the bonus system tree changes(state of a single node or the relations to each other) then
			// cache all bonus objects. Selector objects doesn't matter.
			std::lock_guard lock(sync);
			if (cachedLast == nodeChanged)
			{
				// While our thread was waiting, another one have updated bonus tree. Use cached bonuses.
				cachedBonuses.getBonuses(*ret, selector);
			}
			else
			{
				// Cached bonuses may be outdated - regenerate them
				BonusList allBonuses;

				cachedBonuses.clear();

				getAllBonusesRec(allBonuses);
				limitBonuses(allBonuses, cachedBonuses);
				cachedBonuses.stackBonuses();
				cachedLast = nodeChanged;
				cachedBonuses.getBonuses(*ret, selector);
			}
		}

		// Save the results in the cache
		if (!cachingStr.empty())
		{
			RequestsMap::accessor accessor;
			if (cachedRequests.find(accessor, cachingStr))
			{
				accessor->second.second = ret;
				accessor->second.first = cachedLast;
			}
			else
				cachedRequests.emplace(cachingStr, std::pair<int32_t, TBonusListPtr>{ cachedLast, ret });
		}

		return ret;
	}
	else
	{
		return getAllBonusesWithoutCaching(selector);
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonusesWithoutCaching(const CSelector &selector) const
{
	auto ret = std::make_shared<BonusList>();

	// Get bonus results without caching enabled.
	BonusList beforeLimiting;
	BonusList afterLimiting;
	getAllBonusesRec(beforeLimiting);
	limitBonuses(beforeLimiting, afterLimiting);
	afterLimiting.getBonuses(*ret, selector);
	ret->stackBonuses();
	return ret;
}

std::shared_ptr<Bonus> CBonusSystemNode::getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const
{
	assert(updater);
	return updater->createUpdatedBonus(b, * this);
}

CBonusSystemNode::CBonusSystemNode(BonusNodeType NodeType, bool isHypotetic):
	nodeType(NodeType),
	cachedLast(0),
	nodeChanged(0),
	isHypotheticNode(isHypotetic)
{
}

CBonusSystemNode::CBonusSystemNode(BonusNodeType NodeType):
	CBonusSystemNode(NodeType, false)
{}

CBonusSystemNode::~CBonusSystemNode()
{
	detachFromAll();

	if(!children.empty())
	{
		while(!children.empty())
			children.front()->detachFrom(*this);
	}

	clearPropagationRecords();
	while(!propagationDependents.empty())
		propagationDependents.begin()->first->removePropagationRecordsForContext(*this);
}

void CBonusSystemNode::attachTo(CBonusSystemNode & parent)
{
	assert(!vstd::contains(parentsToPropagate, &parent));
	parentsToPropagate.push_back(&parent);

	attachToSource(parent);

	if(!isHypothetic())
	{
		if(!parent.actsAsBonusSourceOnly())
			newRedDescendant(parent);

		assert(!vstd::contains(parent.children, this));
		parent.children.push_back(this);
	}

	// The new branch was invalidated by attachToSource. Adding it does not
	// change the parent or existing branches; propagation invalidates every
	// node whose bonuses actually change.
}

void CBonusSystemNode::attachToSource(const CBonusSystemNode & parent)
{
	assert(!vstd::contains(parentsToInherit, &parent));
	parentsToInherit.push_back(&parent);

	++globalCounter;

	if(!isHypothetic())
	{
		if(parent.actsAsBonusSourceOnly())
			parent.newRedDescendant(*this);
	}

	invalidateChildrenNodes(globalCounter);
}

void CBonusSystemNode::detachFrom(CBonusSystemNode & parent)
{
	assert(vstd::contains(parentsToPropagate, &parent));

	if(!isHypothetic())
	{
		if(!parent.actsAsBonusSourceOnly())
			removedRedDescendant(parent);
	}

	detachFromSource(parent);

	if (vstd::contains(parentsToPropagate, &parent))
	{
		parentsToPropagate -= &parent;
	}
	else
	{
		logBonus->error("Error on Detach. Node %s (nodeType=%d) has not parent %s (nodeType=%d)",
			nodeShortInfo(), static_cast<int>(nodeType), parent.nodeShortInfo(), static_cast<int>(parent.nodeType));
	}

	if(!isHypothetic())
	{
		if(vstd::contains(parent.children, this))
			parent.children -= this;
		else
		{
			logBonus->error("Error on Detach. Node %s (nodeType=%d) is not a child of %s (nodeType=%d)",
				nodeShortInfo(), static_cast<int>(nodeType), parent.nodeShortInfo(), static_cast<int>(parent.nodeType));
		}
	}

	parent.nodeHasChanged();
}


void CBonusSystemNode::detachFromSource(const CBonusSystemNode & parent)
{
	assert(vstd::contains(parentsToInherit, &parent));

	++globalCounter;

	if(!isHypothetic())
	{
		if(parent.actsAsBonusSourceOnly())
			parent.removedRedDescendant(*this);
	}

	if (vstd::contains(parentsToInherit, &parent))
	{
		parentsToInherit -= &parent;
	}
	else
	{
		logBonus->error("Error on Detach. Node %s (nodeType=%d) has not parent %s (nodeType=%d)",
			nodeShortInfo(), static_cast<int>(nodeType), parent.nodeShortInfo(), static_cast<int>(parent.nodeType));
	}

	invalidateChildrenNodes(globalCounter);
}

void CBonusSystemNode::removeBonusesRecursive(const CSelector & s)
{
	removeBonuses(s);
	for(CBonusSystemNode * child : children)
		child->removeBonusesRecursive(s);
}

void CBonusSystemNode::reduceBonusDurations(const CSelector &s)
{
	BonusList bl;
	exportedBonuses.getBonuses(bl, s);
	for(const auto & b : bl)
	{
		b->turnsRemain--;
		if(b->turnsRemain <= 0)
			removeBonus(b);
	}

	for(CBonusSystemNode *child : children)
		child->reduceBonusDurations(s);
}

void CBonusSystemNode::addNewBonus(const std::shared_ptr<Bonus>& b)
{
	//turnsRemain shouldn't be zero for following durations
	if(Bonus::NTurns(b.get()) || Bonus::NDays(b.get()) || Bonus::OneWeek(b.get()))
	{
		assert(b->turnsRemain);
	}

	assert(!vstd::contains(exportedBonuses, b));
	exportedBonuses.push_back(b);
	exportBonus(b);
}

void CBonusSystemNode::accumulateBonus(const std::shared_ptr<Bonus>& b)
{
	auto bonus = exportedBonuses.getFirst(Selector::typeSubtypeValueType(b->type, b->subtype, b->valType)); //only local bonuses are interesting
	if(bonus)
	{
		bonus->val += b->val;
		nodeHasChanged();
	}
	else
		addNewBonus(std::make_shared<Bonus>(*b)); //duplicate needed, original may get destroyed
}

void CBonusSystemNode::removeBonus(const std::shared_ptr<Bonus>& b)
{
	// Callers may pass a reference to an element of exportedBonuses.
	// Keep the object alive while removing that element and its propagated instances.
	auto bonus = b;
	exportedBonuses -= bonus;
	if(bonus->propagator)
	{
		++globalCounter;
		const CBonusSystemNode * updaterContext = bonus->propagationUpdater && !actsAsBonusSourceOnly() ? this : nullptr;
		unpropagateBonus(bonus, updaterContext);
		invalidateChildrenNodes(globalCounter);
	}
	else
	{
		bonuses -= bonus;
		nodeHasChanged();
	}
}

void CBonusSystemNode::removeBonuses(const CSelector & selector)
{
	BonusList toRemove;
	exportedBonuses.getBonuses(toRemove, selector);
	for(const auto & bonus : toRemove)
		removeBonus(bonus);
}

bool CBonusSystemNode::actsAsBonusSourceOnly() const
{
	switch(nodeType)
	{
	case BonusNodeType::CREATURE:
	case BonusNodeType::ARTIFACT:
	case BonusNodeType::ARTIFACT_INSTANCE:
	case BonusNodeType::BOAT:
		return true;
	default:
		return false;
	}
}

void CBonusSystemNode::propagateBonus(
	const std::shared_ptr<Bonus> & b,
	const CBonusSystemNode & source,
	bool trackUpdater)
{
	if(b->propagator->shouldBeAttached(this))
	{
		if(b->propagationUpdater)
		{
			if(trackUpdater)
				addPropagationRecord(b, source);
			else
				bonuses.push_back(source.getUpdatedBonus(b, b->propagationUpdater));
		}
		else
			bonuses.push_back(b);

		logBonus->trace("#$# %s #propagated to# %s", b->Description(nullptr), nodeName());
		invalidateChildrenNodes(globalCounter);
	}

	TNodes lchildren;
	getRedChildren(lchildren);
	for(CBonusSystemNode *pname : lchildren)
		pname->propagateBonus(b, source, trackUpdater);
}

void CBonusSystemNode::unpropagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode * updaterContext)
{
	if(b->propagator->shouldBeAttached(this))
	{
		if(b->propagationUpdater)
		{
			if(updaterContext)
			{
				if(!removePropagationRecord(b, *updaterContext))
					logBonus->warn("Attempt to remove updated bonus (type=%d), which is not propagated to %s",
						static_cast<int>(b->type), nodeName());
			}
			else
			{
				bonuses.remove_if([this, b](const Bonus * bonus)
				{
					return !propagationRecords.contains(bonus)
						&& bonus->propagationUpdater
						&& bonus->propagationUpdater == b->propagationUpdater;
				});
			}
		}
		else
		{
			if (bonuses -= b)
				logBonus->trace("#$# %s #is no longer propagated to# %s", b->Description(nullptr), nodeName());
			else
				logBonus->warn("Attempt to remove #$# %s, which is not propagated to %s", b->Description(nullptr), nodeName());
		}

		invalidateChildrenNodes(globalCounter);
	}

	TNodes lchildren;
	getRedChildren(lchildren);
	for(CBonusSystemNode *pname : lchildren)
		pname->unpropagateBonus(b, updaterContext);
}

void CBonusSystemNode::addPropagationRecord(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & updaterContext)
{
	auto materialized = std::make_shared<Bonus>(*b);
	bonuses.push_back(materialized);
	propagationRecords.emplace(materialized.get(), PropagationRecord{materialized, b, &updaterContext});
	updaterContext.addPropagationDependent(*this);
}

bool CBonusSystemNode::removePropagationRecord(
	const std::shared_ptr<Bonus> & b,
	const CBonusSystemNode & updaterContext)
{
	for(auto it = propagationRecords.begin(); it != propagationRecords.end(); ++it)
	{
		if(it->second.original == b && it->second.updaterContext == &updaterContext)
		{
			bonuses -= it->second.materialized;
			updaterContext.removePropagationDependent(*this);
			propagationRecords.erase(it);
			return true;
		}
	}
	return false;
}

std::shared_ptr<Bonus> CBonusSystemNode::applyPropagationUpdater(const std::shared_ptr<Bonus> & b) const
{
	auto record = propagationRecords.find(b.get());
	if(record == propagationRecords.end())
		return b;

	return record->second.updaterContext->getUpdatedBonus(
		record->second.original,
		record->second.original->propagationUpdater);
}

void CBonusSystemNode::addPropagationDependent(CBonusSystemNode & dependent) const
{
	propagationDependents[&dependent]++;
}

void CBonusSystemNode::removePropagationDependent(CBonusSystemNode & dependent) const
{
	auto it = propagationDependents.find(&dependent);
	assert(it != propagationDependents.end());
	if(--it->second == 0)
		propagationDependents.erase(it);
}

void CBonusSystemNode::clearPropagationRecords()
{
	for(const auto & entry : propagationRecords)
	{
		bonuses -= entry.second.materialized;
		entry.second.updaterContext->removePropagationDependent(*this);
	}
	propagationRecords.clear();
}

void CBonusSystemNode::removePropagationRecordsForContext(const CBonusSystemNode & updaterContext)
{
	bool removed = false;
	for(auto it = propagationRecords.begin(); it != propagationRecords.end();)
	{
		if(it->second.updaterContext == &updaterContext)
		{
			bonuses -= it->second.materialized;
			updaterContext.removePropagationDependent(*this);
			it = propagationRecords.erase(it);
			removed = true;
		}
		else
			++it;
	}

	if(!removed)
	{
		assert(removed);
		updaterContext.propagationDependents.erase(this);
		return;
	}

	invalidateChildrenNodes(++globalCounter);
}

void CBonusSystemNode::detachFromAll()
{
	while(!parentsToPropagate.empty())
		detachFrom(*parentsToPropagate.front());

	while(!parentsToInherit.empty())
		detachFromSource(*parentsToInherit.front());
}

bool CBonusSystemNode::isIndependentNode() const
{
	return parentsToInherit.empty() && parentsToPropagate.empty() && children.empty();
}

std::string CBonusSystemNode::nodeName() const
{
	return std::string("Bonus system node of type ") + typeid(*this).name();
}

std::string CBonusSystemNode::nodeShortInfo() const
{
	std::ostringstream str;
	str << "'" << typeid(* this).name() << "'";
	return str.str();
}

void CBonusSystemNode::getRedParents(TCNodes & out) const
{
	TCNodes lparents;
	getDirectParents(lparents);
	for(const CBonusSystemNode *pname : lparents)
	{
		if(pname->actsAsBonusSourceOnly())
		{
			out.insert(pname);
		}
	}

	if(!actsAsBonusSourceOnly())
	{
		for(const CBonusSystemNode *child : children)
		{
			out.insert(child);
		}
	}
}

void CBonusSystemNode::getRedChildren(TNodes &out)
{
	for(CBonusSystemNode *pname : parentsToPropagate)
	{
		if(!pname->actsAsBonusSourceOnly())
		{
			out.insert(pname);
		}
	}

	if(actsAsBonusSourceOnly())
	{
		for(CBonusSystemNode *child : children)
		{
			out.insert(child);
		}
	}
}

void CBonusSystemNode::newRedDescendant(CBonusSystemNode & descendant) const
{
	for(const auto & b : exportedBonuses)
	{
		if(b->propagator)
			descendant.propagateBonus(b, *this, b->propagationUpdater && !actsAsBonusSourceOnly());
	}
	TCNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(const auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
		{
			if(b->propagator)
			{
				// Source-only nodes keep the context supplied by the topology event.
				// Runtime nodes retain their own context so later changes invalidate the target.
				const bool trackUpdater = b->propagationUpdater && !parent->actsAsBonusSourceOnly();
				const CBonusSystemNode & source = trackUpdater ? *parent : *this;
				descendant.propagateBonus(b, source, trackUpdater);
			}
		}
	}
}

void CBonusSystemNode::removedRedDescendant(CBonusSystemNode & descendant) const
{
	++globalCounter;
	for(const auto & b : exportedBonuses)
		if(b->propagator)
		{
			const CBonusSystemNode * updaterContext = b->propagationUpdater && !actsAsBonusSourceOnly() ? this : nullptr;
			descendant.unpropagateBonus(b, updaterContext);
		}

	TCNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
			if(b->propagator)
			{
				const CBonusSystemNode * updaterContext = b->propagationUpdater && !parent->actsAsBonusSourceOnly() ? parent : nullptr;
				descendant.unpropagateBonus(b, updaterContext);
			}
	}
}

void CBonusSystemNode::getRedAncestors(TCNodes &out) const
{
	getRedParents(out);

	TCNodes redParents;
	getRedParents(redParents);

	for(const CBonusSystemNode * parent : redParents)
		parent->getRedAncestors(out);
}

void CBonusSystemNode::exportBonus(const std::shared_ptr<Bonus> & b)
{
	if(b->propagator)
	{
		++globalCounter;
		propagateBonus(b, *this, b->propagationUpdater && !actsAsBonusSourceOnly());
		invalidateChildrenNodes(globalCounter);
	}
	else
	{
		bonuses.push_back(b);
		nodeHasChanged();
	}
}

void CBonusSystemNode::exportBonuses()
{
	for(const auto & b : exportedBonuses)
		exportBonus(b);
}

BonusNodeType CBonusSystemNode::getNodeType() const
{
	return nodeType;
}

const TCNodesVector& CBonusSystemNode::getParentNodes() const
{
	return parentsToInherit;
}

BonusList & CBonusSystemNode::getExportedBonusList()
{
	return exportedBonuses;
}

const BonusList & CBonusSystemNode::getExportedBonusList() const
{
	return exportedBonuses;
}

void CBonusSystemNode::limitBonuses(const BonusList &allBonuses, BonusList &out) const
{
	assert(&allBonuses != &out); //todo should it work in-place?

	BonusList undecided = allBonuses;
	BonusList & accepted = out;

	while(true)
	{
		int undecidedCount = static_cast<int>(undecided.size());
		for(int i = 0; i < undecided.size(); i++)
		{
			auto b = undecided[i];
			BonusLimitationContext context = {*b, *this, out, undecided};
			auto decision = b->limiter ? b->limiter->limit(context) : ILimiter::EDecision::ACCEPT; //bonuses without limiters will be accepted by default
			if(decision == ILimiter::EDecision::DISCARD || decision == ILimiter::EDecision::NOT_APPLICABLE)
			{
				undecided.erase(i);
				i--; continue;
			}
			else if(decision == ILimiter::EDecision::ACCEPT)
			{
				accepted.push_back(b);
				undecided.erase(i);
				i--; continue;
			}
			else
				assert(decision == ILimiter::EDecision::NOT_SURE);
		}

		if(undecided.size() == undecidedCount) //we haven't moved a single bonus -> limiters reached a stable state
			return;
	}
}

void CBonusSystemNode::nodeHasChanged()
{
	invalidateChildrenNodes(++globalCounter);
}

void CBonusSystemNode::invalidateChildrenNodes(int32_t changeCounter)
{
	if (nodeChanged == changeCounter)
		return;

	nodeChanged = changeCounter;

	for(CBonusSystemNode * child : children)
		child->invalidateChildrenNodes(changeCounter);

	for(const auto & dependent : propagationDependents)
		dependent.first->invalidateChildrenNodes(changeCounter);
}

int32_t CBonusSystemNode::getTreeVersion() const
{
	return nodeChanged;
}
