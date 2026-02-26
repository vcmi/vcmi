/*
 * CBonusSystemNode.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BonusList.h"
#include "IBonusBearer.h"

#include "../serializer/Serializeable.h"

#include <tbb/concurrent_hash_map.h>

VCMI_LIB_NAMESPACE_BEGIN

using TNodes = std::set<CBonusSystemNode *>;
using TCNodes = std::set<const CBonusSystemNode *>;
using TNodesVector = std::vector<CBonusSystemNode *>;
using TCNodesVector = std::vector<const CBonusSystemNode *>;

class DLL_LINKAGE CBonusSystemNode : public virtual IBonusBearer, public virtual Serializeable, public boost::noncopyable
{
public:
	struct HashStringCompare {
		static size_t hash(const std::string& data)
		{
			std::hash<std::string> hasher;
			return hasher(data);
		}
		static bool equal(const std::string& x, const std::string& y)
		{
			return x == y;
		}
	};

private:
	/// List of bonuses that affect this node, whether local, or propagated to this node
	BonusList bonuses;

	/// List of bonuses that ar ecoming from this node.
	/// Also includes nodes that are propagated away from this node, and might not affect this node itself
	BonusList exportedBonuses;

	TCNodesVector parentsToInherit; // we inherit bonuses from them
	TNodesVector parentsToPropagate; // we may attach our bonuses to them
	TNodesVector children;

	BonusNodeType nodeType;
	bool isHypotheticNode;

	mutable BonusList cachedBonuses;
	mutable int32_t cachedLast;
	std::atomic<int32_t> nodeChanged;

	void invalidateChildrenNodes(int32_t changeCounter);

	// Setting a value to cachingStr before getting any bonuses caches the result for later requests.
	// This string needs to be unique, that's why it has to be set in the following manner:
	// [property key]_[value] => only for selector
	using RequestsMap = tbb::concurrent_hash_map<std::string, std::pair<int32_t, TBonusListPtr>, HashStringCompare>;
	mutable RequestsMap cachedRequests;
	mutable std::shared_mutex sync;

	void getAllBonusesRec(BonusList &out) const;
	TConstBonusListPtr getAllBonusesWithoutCaching(const CSelector &selector) const;
	std::shared_ptr<Bonus> getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const;
	void limitBonuses(const BonusList &allBonuses, BonusList &out) const; //out will bo populed with bonuses that are not limited here

	void getRedParents(TCNodes &out) const;  //retrieves list of red parent nodes (nodes bonuses propagate from)
	void getRedAncestors(TCNodes &out) const;
	void getRedChildren(TNodes &out);

	void propagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & source);
	void unpropagateBonus(const std::shared_ptr<Bonus> & b);
	bool actsAsBonusSourceOnly() const;

	void newRedDescendant(CBonusSystemNode & descendant) const; //propagation needed
	void removedRedDescendant(CBonusSystemNode & descendant) const; //de-propagation needed

	std::string nodeShortInfo() const;

	void exportBonus(const std::shared_ptr<Bonus> & b);

protected:
	bool isIndependentNode() const; //node is independent when it has no parents nor children
	void exportBonuses();

public:
	explicit CBonusSystemNode(BonusNodeType nodeType, bool isHypotetic);
	explicit CBonusSystemNode(BonusNodeType nodeType);
	virtual ~CBonusSystemNode();

	TConstBonusListPtr getAllBonuses(const CSelector &selector, const std::string &cachingStr = "") const override;
	void getDirectParents(TCNodes &out) const;  //retrieves list of parent nodes (nodes to inherit bonuses from),

	/// Returns first bonus matching selector
	std::shared_ptr<const Bonus> getFirstBonus(const CSelector & selector) const;

	/// Provides write access to first bonus from this node that matches selector
	std::shared_ptr<Bonus> getLocalBonus(const CSelector & selector);

	void attachTo(CBonusSystemNode & parent);
	void attachToSource(const CBonusSystemNode & parent);
	void detachFrom(CBonusSystemNode & parent);
	void detachFromSource(const CBonusSystemNode & parent);
	void detachFromAll();
	virtual void addNewBonus(const std::shared_ptr<Bonus>& b);
	void accumulateBonus(const std::shared_ptr<Bonus>& b); //add value of bonus with same type/subtype or create new

	void removeBonus(const std::shared_ptr<Bonus>& b);
	void removeBonuses(const CSelector & selector);
	void removeBonusesRecursive(const CSelector & s);

	///updates count of remaining turns and removes outdated bonuses by selector
	void reduceBonusDurations(const CSelector &s);
	virtual std::string bonusToString(const std::shared_ptr<Bonus>& bonus) const {return "";}; //description or bonus name
	virtual std::string nodeName() const;
	bool isHypothetic() const { return isHypotheticNode; }

	BonusList & getExportedBonusList();
	const BonusList & getExportedBonusList() const;
	BonusNodeType getNodeType() const;
	const TCNodesVector & getParentNodes() const;

	void nodeHasChanged();

	int32_t getTreeVersion() const override;

	virtual PlayerColor getOwner() const
	{
		return PlayerColor::NEUTRAL;
	}

	template <typename Handler> void serialize(Handler &h)
	{
		h & nodeType;
		h & exportedBonuses;

		if(!h.saving && h.loadingGamestate)
			exportBonuses();
	}

	friend class CBonusProxy;
};

VCMI_LIB_NAMESPACE_END
