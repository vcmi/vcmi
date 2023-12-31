/*
 * BonusCaster.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ProxyCaster.h"

VCMI_LIB_NAMESPACE_BEGIN

struct Bonus;

namespace spells
{

class DLL_LINKAGE BonusCaster : public ProxyCaster
{
public:
	BonusCaster(const Caster * actualCaster_, std::shared_ptr<Bonus> bonus_);
	virtual ~BonusCaster();

	void getCasterName(MetaString & text) const override;
	void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int spellCost) const override;

private:
	std::shared_ptr<Bonus> bonus;
};

} // namespace spells


VCMI_LIB_NAMESPACE_END
