/*
 * ApplyDamage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/ApplyDamage.h>

namespace events
{

class DLL_LINKAGE CApplyDamage : public ApplyDamage
{
public:
	CApplyDamage(const Environment * env_, BattleStackAttacked * pack_, std::shared_ptr<battle::Unit> target_);

	void execute() override;

	int64_t getInitalDamage() const override;
	int64_t getDamage() const override;
	void setDamage(int64_t value) override;
	const battle::Unit * getTarget() const override;
private:
	int64_t initalDamage;

	const Environment * env;
	BattleStackAttacked * pack;
	std::shared_ptr<battle::Unit> target;
};

}



