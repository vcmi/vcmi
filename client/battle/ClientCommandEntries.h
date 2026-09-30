/*
 * ClientCommandEntries.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionEntry.h"

class BattleInterface;

/// Base of the entries handled by the client alone, without a battle action
class ClientCommandEntry : public IBattleActionEntry
{
protected:
	BattleInterface & owner;

	/// Living unit on the hex, or nullptr
	const CStack * getUnit(const BattleHex & hex) const;

public:
	explicit ClientCommandEntry(BattleInterface & owner);

	BattleHexArray getShadedHexes(const BattleHex & hex) const override;
	SpellID getSpell() const override;
};

/// Opens the window of a living unit
class CreatureInfoEntry final : public ClientCommandEntry
{
public:
	using ClientCommandEntry::ClientCommandEntry;

	int getPriority(const CStack * actor, const CStack * target) const override;
	bool isLegal(const BattleHex & hex) const override;
	BattleActionPreview preview(const BattleHex & hex) const override;
	void realize(const BattleHex & hex) const override;
	std::optional<UnitActionButton> getPanelButton() const override;
};

/// Opens the window of a hero of the battle
class HeroInfoEntry final : public ClientCommandEntry
{
public:
	using ClientCommandEntry::ClientCommandEntry;

	int getPriority(const CStack * actor, const CStack * target) const override;
	bool isLegal(const BattleHex & hex) const override;
	BattleActionPreview preview(const BattleHex & hex) const override;
	void realize(const BattleHex & hex) const override;
	std::optional<UnitActionButton> getPanelButton() const override;
};

/// Makes another unit of the player the active one during tactics
class TacticsUnitSelectionEntry final : public ClientCommandEntry
{
public:
	using ClientCommandEntry::ClientCommandEntry;

	int getPriority(const CStack * actor, const CStack * target) const override;
	bool isLegal(const BattleHex & hex) const override;
	BattleActionPreview preview(const BattleHex & hex) const override;
	void realize(const BattleHex & hex) const override;
	std::optional<UnitActionButton> getPanelButton() const override;
};
