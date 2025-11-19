/*
 * BattleOnlyModeTab.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapping/CMap.h"


VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CCreatureSet;
class CMap;
class EditorCallback;
class BattleOnlyModeStartInfo;
VCMI_LIB_NAMESPACE_END

class FilledTexturePlayerColored;
class CButton;
class CPicture;
class CLabel;
class CMultiLineLabel;
class BattleOnlyModeTab;
class CAnimImage;
class GraphicalPrimitiveCanvas;
class CTextInput;
class TransparentFilledRectangle;
class CToggleButton;

class BattleOnlyModeHeroSelector : public CIntObject
{
private:
	BattleOnlyModeTab& parent;

	std::shared_ptr<CPicture> backgroundImage;
	std::shared_ptr<CPicture> heroImage;
	std::shared_ptr<CLabel> heroLabel;
	std::vector<std::shared_ptr<CPicture>> creatureImage;
	std::vector<std::shared_ptr<CPicture>> secSkillImage;
	std::vector<std::shared_ptr<CPicture>> artifactImage;

	std::vector<std::shared_ptr<CPicture>> addIcon;

	void selectHero();
	void selectCreature(int slot);
	void selectSecSkill(int slot);
	void selectArtifact(int slot, ArtifactID artifactId);

	int id;
public:
	std::vector<std::shared_ptr<CAnimImage>> primSkills;
	std::vector<std::shared_ptr<GraphicalPrimitiveCanvas>> primSkillsBorder;
	std::vector<std::shared_ptr<CTextInput>> primSkillsInput;

	std::vector<std::shared_ptr<CTextInput>> selectedArmyInput;
	std::vector<std::shared_ptr<CTextInput>> selectedSecSkillInput;

	std::shared_ptr<CToggleButton> spellBook;
	std::shared_ptr<CToggleButton> warMachines;

	void setHeroIcon();
	void setCreatureIcons();
	void setSecSkillIcons();
	void setArtifactIcons();
	void manageSpells();
	BattleOnlyModeHeroSelector(int id, BattleOnlyModeTab& parent, Point position);
};

class BattleOnlyModeTab : public CIntObject
{
	friend class BattleOnlyModeHeroSelector;
private:
	std::shared_ptr<BattleOnlyModeStartInfo> startInfo;
	std::unique_ptr<CMap> map;
	std::shared_ptr<EditorCallback> cb;

	std::shared_ptr<CPicture> backgroundImage;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CMultiLineLabel> subTitle;

	std::shared_ptr<CButton> battlefieldSelector;
	std::shared_ptr<CButton> buttonReset;
	std::shared_ptr<BattleOnlyModeHeroSelector> heroSelector1;
	std::shared_ptr<BattleOnlyModeHeroSelector> heroSelector2;

	ColorRGBA disabledColor;
	ColorRGBA boxColor;
	ColorRGBA disabledBoxColor;

	void init();
	void onChange();
	void update();
	void setTerrainButtonText();
	void selectTerrain();
	void reset();
public:
	BattleOnlyModeTab();
	void applyStartInfo(std::shared_ptr<BattleOnlyModeStartInfo> si);
	void startBattle();
	void setStartButtonEnabled();
};
