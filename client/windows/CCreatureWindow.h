/*
 * CCreatureWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/HeroBonus.h"
#include "../widgets/MiscWidgets.h"
#include "CWindowObject.h"

struct StackWindowInfo;
class CCommanderInstance;
class CStackInstance;
class CStack;
struct UpgradeInfo;
class CTabbedInt;
class CButton;

class CCommanderSkillIcon : public LRClickableAreaWText //TODO: maybe bring commander skill button initialization logic inside?
{
	CIntObject * object; // passive object that will be used to determine clickable area
public:
	CCommanderSkillIcon(CIntObject * object, std::function<void()> callback);

	std::function<void()> callback;

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;

	void setObject(CIntObject * object);
};

class CStackWindow : public CWindowObject
{
	struct BonusInfo
	{
		std::string name;
		std::string description;
		std::string imagePath;
	};

	class CWindowSection : public CIntObject
	{
		CStackWindow *parent;

		void createBackground(std::string path);
		void createBonusItem(size_t index, Point position);

		void printStatString(int index, std::string name, std::string value);
		void printStatRange(int index, std::string name, int min, int max);
		void printStatBase(int index, std::string name, int base, int current);
		void printStat(int index, std::string name, int value);
	public:
		void createStackInfo(bool showExp, bool showArt);
		void createActiveSpells();
		void createCommanderSection();
		void createCommander();
		void createCommanderAbilities();
		void createBonuses(boost::optional<size_t> size = boost::optional<size_t>());
		void createBonusEntry(size_t index);
		void createButtonPanel();

		CWindowSection(CStackWindow * parent);
	};

	std::unique_ptr<CAnimImage> stackArtifactIcon;
	std::unique_ptr<LRClickableAreaWTextComp> stackArtifactHelp;
	std::unique_ptr<CButton> stackArtifactButton;
	CAnimImage *expRankIcon;
	LRClickableAreaWText *expArea;
	CLabel *expLabel;

	std::unique_ptr<StackWindowInfo> info;
	std::vector<BonusInfo> activeBonuses;
	size_t activeTab;
	CTabbedInt *commanderTab;

	std::map<int, CButton *> switchButtons;

	void setSelection(si32 newSkill, CCommanderSkillIcon * newIcon);
	CCommanderSkillIcon * selectedIcon;
	si32 selectedSkill;

	CIntObject * createBonusEntry(size_t index);
	CIntObject * switchTab(size_t index);

	void removeStackArtifact(ArtifactPosition pos);

	void initSections();
	void initBonusesList();

	void init();

	std::string generateStackExpDescription();

	CIntObject * createSkillEntry(int index);

public:
	// for battles
	CStackWindow(const CStack * stack, bool popup);

	// for non-existing stacks, e.g. recruit screen
	CStackWindow(const CCreature * creature, bool popup);

	// for normal stacks in armies
	CStackWindow(const CStackInstance * stack, bool popup);
	CStackWindow(const CStackInstance * stack, std::function<void()> dismiss, const UpgradeInfo & info, std::function<void(CreatureID)> callback);

	// for commanders & commander level-up dialog
	CStackWindow(const CCommanderInstance * commander, bool popup);
	CStackWindow(const CCommanderInstance * commander, std::vector<ui32> &skills, std::function<void(ui32)> callback);

	~CStackWindow();
};
