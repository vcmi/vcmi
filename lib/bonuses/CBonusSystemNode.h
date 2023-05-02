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

VCMI_LIB_NAMESPACE_BEGIN

using TNodes = std::set<CBonusSystemNode *>;
using TCNodes = std::set<const CBonusSystemNode *>;
using TNodesVector = std::vector<CBonusSystemNode *>;

class DLL_LINKAGE CBonusSystemNode : public virtual IBonusBearer, public boost::noncopyable
{
public:
	enum ENodeTypes
	{
		NONE = -1, 
		UNKNOWN, STACK_INSTANCE, STACK_BATTLE, SPECIALTY, ARTIFACT, CREATURE, ARTIFACT_INSTANCE, HERO, PLAYER, TEAM,
		TOWN_AND_VISITOR, BATTLE, COMMANDER, GLOBAL_EFFECTS, ALL_CREATURES, TOWN
	};
private:
	BonusList bonuses; //wielded bonuses (local or up-propagated here)
	BonusList exportedBonuses; //bonuses coming from this node (wielded or propagated away)

	TNodesVector parents; //parents -> we inherit bonuses from them, we may attach our bonuses to them
	TNodesVector children;

	ENodeTypes nodeType;
	std::string description;
	bool isHypotheticNode;

	static const bool cachingEnabled;
	mutable BonusList cachedBonuses;
	mutable int64_t cachedLast;
	static std::atomic<int64_t> treeChanged;

	// Setting a value to cachingStr before getting any bonuses caches the result for later requests.
	// This string needs to be unique, that's why it has to be setted in the following manner:
	// [property key]_[value] => only for selector
	mutable std::map<std::string, TBonusListPtr > cachedRequests;
	mutable boost::mutex sync;

	void getAllBonusesRec(BonusList &out, const CSelector & selector) const;
	TConstBonusListPtr getAllBonusesWithoutCaching(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr) const;
	std::shared_ptr<Bonus> getUpdatedBonus(const std::shared_ptr<Bonus> & b, const TUpdaterPtr & updater) const;

	void getRedParents(TNodes &out);  //retrieves list of red parent nodes (nodes bonuses propagate from)
	void getRedAncestors(TNodes &out);
	void getRedChildren(TNodes &out);

	void getAllParents(TCNodes & out) const;

	void newChildAttached(CBonusSystemNode & child);
	void childDetached(CBonusSystemNode & child);
	void propagateBonus(const std::shared_ptr<Bonus> & b, const CBonusSystemNode & source);
	void unpropagateBonus(const std::shared_ptr<Bonus> & b);
	bool actsAsBonusSourceOnly() const;

	void newRedDescendant(CBonusSystemNode & descendant); //propagation needed
	void removedRedDescendant(CBonusSystemNode & descendant); //de-propagation needed

	std::string nodeShortInfo() const;

	void exportBonus(const std::shared_ptr<Bonus> & b);

public:
	explicit CBonusSystemNode(bool isHypotetic = false);
	explicit CBonusSystemNode(ENodeTypes NodeType);
	CBonusSystemNode(CBonusSystemNode && other) noexcept;
	virtual ~CBonusSystemNode();

	void limitBonuses(const BonusList &allBonuses, BonusList &out) const; //out will bo populed with bonuses that are not limited here
	TBonusListPtr limitBonuses(const BonusList &allBonuses) const; //same as above, returns out by val for convienence
	TConstBonusListPtr getAllBonuses(const CSelector &selector, const CSelector &limit, const CBonusSystemNode *root = nullptr, const std::string &cachingStr = "") const override;
	void getParents(TCNodes &out) const;  //retrieves list of parent nodes (nodes to inherit bonuses from),
	std::shared_ptr<const Bonus> getBonusLocalFirst(const CSelector & selector) const;

	//non-const interface
	void getParents(TNodes &out);  //retrieves list of parent nodes (nodes to inherit bonuses from)
	static PlayerColor retrieveNodeOwner(const CBonusSystemNode * node);
	std::shared_ptr<Bonus> getBonusLocalFirst(const CSelector & selector);

	void attachTo(CBonusSystemNode & parent);
	void detachFrom(CBonusSystemNode & parent);
	void detachFromAll();
	virtual void addNewBonus(const std::shared_ptr<Bonus>& b);
	void accumulateBonus(const std::shared_ptr<Bonus>& b); //add value of bonus with same type/subtype or create new

	void removeBonus(const std::shared_ptr<Bonus>& b);
	void removeBonuses(const CSelector & selector);
	void removeBonusesRecursive(const CSelector & s);

	bool isIndependentNode() const; //node is independent when it has no parents nor children
	///updates count of remaining turns and removes outdated bonuses by selector
	void reduceBonusDurations(const CSelector &s);
	virtual std::string bonusToString(const std::shared_ptr<Bonus>& bonus, bool description) const {return "";}; //description or bonus name
	virtual std::string nodeName() const;
	bool isHypothetic() const { return isHypotheticNode; }

	void deserializationFix();
	void exportBonuses();

	const BonusList &getBonusList() const;
	BonusList & getExportedBonusList();
	const BonusList & getExportedBonusList() const;
	CBonusSystemNode::ENodeTypes getNodeType() const;
	void setNodeType(CBonusSystemNode::ENodeTypes type);
	const TNodesVector & getParentNodes() const;
	const TNodesVector & getChildrenNodes() const;
	const std::string & getDescription() const;
	void setDescription(const std::string & description);

	static void treeHasChanged();

	int64_t getTreeVersion() const override;

	virtual PlayerColor getOwner() const
	{
		return PlayerColor::NEUTRAL;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
//		h & bonuses;
		h & nodeType;
		h & exportedBonuses;
		h & description;
		BONUS_TREE_DESERIALIZATION_FIX
		//h & parents & children;
	}

	friend class CBonusProxy;
};

VCMI_LIB_NAMESPACE_END