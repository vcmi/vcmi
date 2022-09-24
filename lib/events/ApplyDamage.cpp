/*
 * ApplyDamage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include <vcmi/Environment.h>

#include "ApplyDamage.h"

#include "../../lib/NetPacks.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

SubscriptionRegistry<ApplyDamage> * ApplyDamage::getRegistry()
{
	static std::unique_ptr<SubscriptionRegistry<ApplyDamage>> Instance = make_unique<SubscriptionRegistry<ApplyDamage>>();
	return Instance.get();
}

CApplyDamage::CApplyDamage(const Environment * env_, BattleStackAttacked * pack_, std::shared_ptr<battle::Unit> target_)
	: pack(pack_),
	target(target_)
{
	initalDamage = pack->damageAmount;
}

bool CApplyDamage::isEnabled() const
{
	return true;
}

int64_t CApplyDamage::getInitalDamage() const
{
	return initalDamage;
}

int64_t CApplyDamage::getDamage() const
{
	return pack->damageAmount;
}

void CApplyDamage::setDamage(int64_t value)
{
	pack->damageAmount = value;
}

const battle::Unit * CApplyDamage::getTarget() const
{
	return target.get();
}


};

VCMI_LIB_NAMESPACE_END
