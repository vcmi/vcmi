/*
 * ObstacleCasterProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ProxyCaster.h"
#include "../lib/NetPacksBase.h"
#include "../battle/BattleHex.h"
#include "../battle/CObstacleInstance.h"

VCMI_LIB_NAMESPACE_BEGIN
namespace spells
{

class DLL_LINKAGE ObstacleCasterProxy : public ProxyCaster
{
public:
	ObstacleCasterProxy(PlayerColor owner_, const Caster * hero_, const SpellCreatedObstacle * obs_);

	int32_t getCasterUnitId() const override;
	int32_t getSpellSchoolLevel(const Spell * spell, int32_t * outSelectedSchool = nullptr) const override;
	int32_t getEffectLevel(const Spell * spell) const override;
	int64_t getSpellBonus(const Spell * spell, int64_t base, const battle::Unit * affectedStack) const override;
	int64_t getSpecificSpellBonus(const Spell * spell, int64_t base) const override;
	int32_t getEffectPower(const Spell * spell) const override;
	int32_t getEnchantPower(const Spell * spell) const override;
	int64_t getEffectValue(const Spell * spell) const override;
	PlayerColor getCasterOwner() const override;
	void getCasterName(MetaString & text) const override;
	void getCastDescription(const Spell * spell, const std::vector<const battle::Unit *> & attacked, MetaString & text) const override;
	void spendMana(ServerCallback * server, const int spellCost) const override;

private:
	const PlayerColor owner;
	const SpellCreatedObstacle & obs;
};

}//
VCMI_LIB_NAMESPACE_END