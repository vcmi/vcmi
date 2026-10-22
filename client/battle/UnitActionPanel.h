/*
 * UnitActionPanel.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionEntry.h"

#include "../gui/CIntObject.h"

class CFilledTexture;
class TransparentFilledRectangle;
class CToggleButton;
class CLabel;
class BattleInterface;

class UnitActionPanel : public CIntObject
{
private:
	std::shared_ptr<CFilledTexture> background;
	std::shared_ptr<TransparentFilledRectangle> rect;
	std::vector<std::shared_ptr<CToggleButton>> buttons;

	BattleInterface & owner;

	void addButton(const UnitActionButton & button, const BattleActionEntries & entries);

	void restoreAllActions();
	void setActions(int buttonIndex, const BattleActionEntries & newActions);
public:
	static constexpr int ACTION_SLOTS = 12;

	UnitActionPanel(BattleInterface & owner);

	void setPossibleActions(const BattleActionEntries & actions);

	std::vector<std::tuple<SpellID, bool>> getSpells() const;

	void show(Canvas & to) override;
};
