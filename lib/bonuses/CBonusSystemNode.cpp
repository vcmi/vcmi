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

VCMI_LIB_NAMESPACE_BEGIN

std::atomic<int64_t> CBonusSystemNode::treeChanged(1);
constexpr bool CBonusSystemNode::cachingEnabled = true;

std::shared_ptr<Bonus> CBonusSystemNode::getLocalBonus(const CSelector & selector)
{
	auto ret = bonuses.getFirst(selector);
	if(ret)
		return ret;
	return nullptr;
}

std::shared_ptr<const Bonus> CBonusSystemNode::getFirstBonus(const CSelector & selector) const
{
	auto ret = bonuses.getFirst(selector);
	if(ret)
		return ret;

	TCNodes lparents;
	getParents(lparents);
	for(const CBonusSystemNode *pname : lparents)
	{
		ret = pname->getFirstBonus(selector);
		if (ret)
			return ret;
	}

	return nullptr;
}

void CBonusSystemNode::getParents(TCNodes & out) const /*retrieves list of parent nodes (nodes to inherit bonuses from) */
{
	for(const auto * elem : parentsToInherit)
		out.insert(elem);
}

void CBonusSystemNode::getAllParents(TCNodes & out) const //retrieves list of parent nodes (nodes to inherit bonuses from)
{
	for(auto * parent : parentsToInherit)
	{
		out.insert(parent);
		parent->getAllParents(out);
	}
}

void CBonusSystemNode::getAllBonusesRec(BonusList &out, const CSelector & selector) const
{
	//out has been reserved sufficient capacity at getAllBonuses() call

	BonusList beforeUpdate;
	TCNodes lparents;
	getAllParents(lparents);

	if(!lparents.empty())
	{
		//estimate on how many bonuses are missing yet - must be positive
		beforeUpdate.reserve(std::max(out.capacity() - out.size(), bonuses.size()));
	}
	else
	{
		beforeUpdate.reserve(bonuses.size()); //at most all local bonuses
	}

	for(const auto * parent : lparents)
	{
		parent->getAllBonusesRec(beforeUpdate, selector);
	}
	bonuses.getAllBonuses(beforeUpdate);

	for(const auto & b : beforeUpdate)
	{
		//We should not run updaters on non-selected bonuses
		auto updated = selector(b.get()) && b->updater
			? getUpdatedBonus(b, b->updater)
			: b;

		//do not add bonus with updater
		bool bonusExists = false;
		for(const auto & bonus : out)
		{
			if (bonus == updated)
				bonusExists = true;
			if (bonus->updater && bonus->updater == updated->updater)
				bonusExists = true;
		}

		if (!bonusExists)
			out.push_back(updated);
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root, const std::string &cachingStr) const
{
	bool limitOnUs = (!root || root == this); //caching won't work when we want to limit bonuses against an external node
	if (CBonusSystemNode::cachingEnabled && limitOnUs)
	{
		// Exclusive access for one thread
		boost::lock_guard<boost::mutex> lock(sync);

		// If the bonus system tree changes(state of a single node or the relations to each other) then
		// cache all bonus objects. Selector objects doesn't matter.
		if (cachedLast != treeChanged)
		{
			BonusList allBonuses;
			allBonuses.reserve(cachedBonuses.capacity()); //we assume we'll get about the same number of bonuses

			cachedBonuses.clear();
			cachedRequests.clear();

			getAllBonusesRec(allBonuses, Selector::all);
			limitBonuses(allBonuses, cachedBonuses);
			cachedBonuses.stackBonuses();

			cachedLast = treeChanged;
		}

		// If a bonus system request comes with a caching string then look up in the map if there are any
		// pre-calculated bonus results. Limiters can't be cached so they have to be calculated.
		if(!cachingStr.empty())
		{
			auto it = cachedRequests.find(cachingStr);
			if(it != cachedRequests.end())
			{
				//Cached list contains bonuses for our query with applied limiters
				return it->second;
			}
		}

		//We still don't have the bonuses (didn't returned them from cache)
		//Perform bonus selection
		auto ret = std::make_shared<BonusList>();
		cachedBonuses.getBonuses(*ret, selector, limit);

		// Save the results in the cache
		if(!cachingStr.empty())
			cachedRequests[cachingStr] = ret;

		return ret;
	}
	else
	{
		return getAllBonusesWithoutCaching(selector, limit, root);
	}
}

TConstBonusListPtr CBonusSystemNode::getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root) const
{
	auto ret = std::make_shared<BonusList>();

	// Get bonus results without caching enabled.
	BonusList beforeLimiting;
	BonusList afterLimiting;
	getAllBonusesRec(beforeLimiting, selector);

	if(!root || root == this)
	{
		limitBonuses(beforeLimiting, afterLimiting);
	}
	else if(root)
	{
		//We want to limit our query against an external node. We get all its bonuses,
		// add the ones we're considering and see if they're cut out by limiters
		BonusList rootBonuses;
		BonusList limitedRootBonuses;
		getAllBonusesRec(rootBonuses, selector);

		for(const auto & b : beforeLimiting)
			rootBonuses.push_back(b);

		root->limitBonuses(rootBonuses, limitedRootBonuses);

		for(const auto & b : beforeLimiting)
			if(vstd::contains(limitedRootBonuses, b))
				afterLimiting.push_back(b);

	}
	afterLimiting.getBonuses(*ret, selector, limit);
	ret->stackBonuses();
	return ret;
}

std::shared_ptr<Bonus> CBonusSystemNode::getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const
{
	assert(updater);
	return updater->createUpdatedBonus(b, * this);
}

CBonusSystemNode::CBonusSystemNode(bool isHypotetic):
	bonuses(true),
	exportedBonuses(true),
	nodeType(UNKNOWN),
	cachedLast(0),
	isHypotheticNode(isHypotetic)
{
}

CBonusSystemNode::CBonusSystemNode(ENodeTypes NodeType):
	bonuses(true),
	exportedBonuses(true),
	nodeType(NodeType),
	cachedLast(0),
	isHypotheticNode(false)
{
}

CBonusSystemNode::~CBonusSystemNode()
{
	detachFromAll();

	if(!children.empty())
	{
		while(!children.empty())
			children.front()->detachFrom(*this);
	}
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

		parent.newChildAttached(*this);
	}

	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::attachToSource(const CBonusSystemNode & parent)
{
	assert(!vstd::contains(parentsToInherit, &parent));
	parentsToInherit.push_back(&parent);

	if(!isHypothetic())
	{
		if(parent.actsAsBonusSourceOnly())
			parent.newRedDescendant(*this);
	}

	CBonusSystemNode::treeHasChanged();
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
		logBonus->error("Error on Detach. Node %s (nodeType=%d) has not parent %s (nodeType=%d)"
			, nodeShortInfo(), nodeType, parent.nodeShortInfo(), parent.nodeType);
	}

	if(!isHypothetic())
	{
		parent.childDetached(*this);
	}
	CBonusSystemNode::treeHasChanged();
}


void CBonusSystemNode::detachFromSource(const CBonusSystemNode & parent)
{
	assert(vstd::contains(parentsToInherit, &parent));

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
		logBonus->error("Error on Detach. Node %s (nodeType=%d) has not parent %s (nodeType=%d)"
			, nodeShortInfo(), nodeType, parent.nodeShortInfo(), parent.nodeType);
	}

	CBonusSystemNode::treeHasChanged();
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
	exportedBonuses.getBonuses(bl, s, Selector::all);
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
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::accumulateBonus(const std::shared_ptr<Bonus>& b)
{
	auto bonus = exportedBonuses.getFirst(Selector::typeSubtypeValueType(b->type, b->subtype, b->valType)); //only local bonuses are interesting
	if(bonus)
		bonus->val += b->val;
	else
		addNewBonus(std::make_shared<Bonus>(*b)); //duplicate needed, original may get destroyed
}

void CBonusSystemNode::removeBonus(const std::shared_ptr<Bonus>& b)
{
	exportedBonuses -= b;
	if(b->propagator)
		unpropagateBonus(b);
	else
		bonuses -= b;
	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::removeBonuses(const CSelector & selector)
{
	BonusList toRemove;
	exportedBonuses.getBonuses(toRemove, selector, Selector::all);
	for(const auto & bonus : toRemove)
		removeBonus(bonus);
}

bool CBonusSystemNode::actsAsBonusSourceOnly() const
{
	switch(nodeType)
	{
	case CREATURE:
	case ARTIFACT:
	case ARTIFACT_INSTANCE:
		return true;
	default:
		return false;
	}
}

void CBonusSystemNode::propagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & source)
{
	if(b->propagator->shouldBeAttached(this))
	{
		auto propagated = b->propagationUpdater 
			? source.getUpdatedBonus(b, b->propagationUpdater)
			: b;
		bonuses.push_back(propagated);
		logBonus->trace("#$# %s #propagated to# %s",  propagated->Description(), nodeName());
	}

	TNodes lchildren;
	getRedChildren(lchildren);
	for(CBonusSystemNode *pname : lchildren)
		pname->propagateBonus(b, source);
}

void CBonusSystemNode::unpropagateBonus(const std::shared_ptr<Bonus> & b)
{
	if(b->propagator->shouldBeAttached(this))
	{
		bonuses -= b;
		logBonus->trace("#$# %s #is no longer propagated to# %s",  b->Description(), nodeName());
	}

	TNodes lchildren;
	getRedChildren(lchildren);
	for(CBonusSystemNode *pname : lchildren)
		pname->unpropagateBonus(b);
}

void CBonusSystemNode::newChildAttached(CBonusSystemNode & child)
{
	assert(!vstd::contains(children, &child));
	children.push_back(&child);
}

void CBonusSystemNode::childDetached(CBonusSystemNode & child)
{
	if(vstd::contains(children, &child))
		children -= &child;
	else
	{
		logBonus->error("Error on Detach. Node %s (nodeType=%d) is not a child of %s (nodeType=%d)"
			, child.nodeShortInfo(), child.nodeType, nodeShortInfo(), nodeType);
	}
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

void CBonusSystemNode::deserializationFix()
{
	exportBonuses();
}

void CBonusSystemNode::getRedParents(TCNodes & out) const
{
	TCNodes lparents;
	getParents(lparents);
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
			descendant.propagateBonus(b, *this);
	}
	TCNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(const auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
		{
			if(b->propagator)
				descendant.propagateBonus(b, *this);
		}
	}
}

void CBonusSystemNode::removedRedDescendant(CBonusSystemNode & descendant) const
{
	for(const auto & b : exportedBonuses)
		if(b->propagator)
			descendant.unpropagateBonus(b);

	TCNodes redParents;
	getRedAncestors(redParents); //get all red parents recursively

	for(auto * parent : redParents)
	{
		for(const auto & b : parent->exportedBonuses)
			if(b->propagator)
				descendant.unpropagateBonus(b);
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
		propagateBonus(b, *this);
	else
		bonuses.push_back(b);

	CBonusSystemNode::treeHasChanged();
}

void CBonusSystemNode::exportBonuses()
{
	for(const auto & b : exportedBonuses)
		exportBonus(b);
}

CBonusSystemNode::ENodeTypes CBonusSystemNode::getNodeType() const
{
	return nodeType;
}

const TCNodesVector& CBonusSystemNode::getParentNodes() const
{
	return parentsToInherit;
}

void CBonusSystemNode::setNodeType(CBonusSystemNode::ENodeTypes type)
{
	nodeType = type;
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
			if(decision == ILimiter::EDecision::DISCARD)
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

TBonusListPtr CBonusSystemNode::limitBonuses(const BonusList &allBonuses) const
{
	auto ret = std::make_shared<BonusList>();
	limitBonuses(allBonuses, *ret);
	return ret;
}

void CBonusSystemNode::treeHasChanged()
{
	treeChanged++;
}

int64_t CBonusSystemNode::getTreeVersion() const
{
	return treeChanged;
}

VCMI_LIB_NAMESPACE_END
