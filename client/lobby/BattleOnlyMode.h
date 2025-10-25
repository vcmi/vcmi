/*
 * BattleOnlyMode.h, part of VCMI engine
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
class BattleOnlyModeWindow;
class CAnimImage;
class GraphicalPrimitiveCanvas;
class CTextInput;
class TransparentFilledRectangle;

class BattleOnlyMode
{
public:
	static void openBattleWindow();
};

class BattleOnlyModeHeroSelector : public CIntObject
{
private:
	BattleOnlyModeWindow& parent;

	std::shared_ptr<CPicture> backgroundImage;
	std::shared_ptr<CPicture> heroImage;
	std::shared_ptr<CLabel> heroLabel;
	std::vector<std::shared_ptr<CPicture>> creatureImage;

	int id;
public:
	std::vector<std::shared_ptr<CAnimImage>> primSkills;
	std::vector<std::shared_ptr<GraphicalPrimitiveCanvas>> primSkillsBorder;
	std::vector<std::shared_ptr<CTextInput>> primSkillsInput;

	std::vector<std::shared_ptr<CTextInput>> selectedArmyInput;

	void setHeroIcon();
	void setCreatureIcons();
	BattleOnlyModeHeroSelector(int id, BattleOnlyModeWindow& parent, Point position);
};

class BattleOnlyModeWindow : public CWindowObject
{
	friend class BattleOnlyModeHeroSelector;
private:
	std::shared_ptr<BattleOnlyModeStartInfo> startInfo;
	std::unique_ptr<CMap> map;
	std::shared_ptr<EditorCallback> cb;

	std::shared_ptr<FilledTexturePlayerColored> backgroundTexture;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonAbort;
	std::shared_ptr<CLabel> title;

	std::shared_ptr<CButton> battlefieldSelector;
	std::shared_ptr<CButton> buttonReset;
	std::shared_ptr<BattleOnlyModeHeroSelector> heroSelector1;
	std::shared_ptr<BattleOnlyModeHeroSelector> heroSelector2;

	void init();
	void onChange();
	void update();
	void setTerrainButtonText();
	void setOkButtonEnabled();
	void startBattle();
public:
	BattleOnlyModeWindow();
	void applyStartInfo(std::shared_ptr<BattleOnlyModeStartInfo> si);
};
