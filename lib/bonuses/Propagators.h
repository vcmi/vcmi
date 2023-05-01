/*
 * Propagators.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"
#include "CBonusSystemNode.h"

VCMI_LIB_NAMESPACE_BEGIN

extern DLL_LINKAGE const std::map<std::string, TPropagatorPtr> bonusPropagatorMap;

class DLL_LINKAGE IPropagator
{
public:
	virtual ~IPropagator() = default;
	virtual bool shouldBeAttached(CBonusSystemNode *dest);
	virtual CBonusSystemNode::ENodeTypes getPropagatorType() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

class DLL_LINKAGE CPropagatorNodeType : public IPropagator
{
	CBonusSystemNode::ENodeTypes nodeType;

public:
	CPropagatorNodeType(CBonusSystemNode::ENodeTypes NodeType = CBonusSystemNode::ENodeTypes::UNKNOWN);
	bool shouldBeAttached(CBonusSystemNode *dest) override;
	CBonusSystemNode::ENodeTypes getPropagatorType() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & nodeType;
	}
};

VCMI_LIB_NAMESPACE_END