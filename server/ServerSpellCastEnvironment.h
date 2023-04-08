/*
 * ServerSpellCastEnvironment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../lib/spells/ISpellMechanics.h"

class CGameHandler;

class ServerSpellCastEnvironment : public SpellCastEnvironment
{
public:
	ServerSpellCastEnvironment(CGameHandler * gh);
	~ServerSpellCastEnvironment() = default;

	void complain(const std::string & problem) override;
	bool describeChanges() const override;

	vstd::RNG * getRNG() override;

	void apply(CPackForClient * pack) override;

	void apply(BattleLogMessage * pack) override;
	void apply(BattleStackMoved * pack) override;
	void apply(BattleUnitsChanged * pack) override;
	void apply(SetStackEffect * pack) override;
	void apply(StacksInjured * pack) override;
	void apply(BattleObstaclesChanged * pack) override;
	void apply(CatapultAttack * pack) override;

	const CMap * getMap() const override;
	const CGameInfoCallback * getCb() const override;
	bool moveHero(ObjectInstanceID hid, int3 dst, bool teleporting) override;
	void genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode &)> callback) override;
private:
	CGameHandler * gh;
};