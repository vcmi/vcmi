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

#include "../../lib/bonuses/HeroBonus.h"
#include "../widgets/MiscWidgets.h"
#include "CWindowObject.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCommanderInstance;
class CStackInstance;
class CStack;
struct UpgradeInfo;

VCMI_LIB_NAMESPACE_END

class UnitView;
class CTabbedInt;
class CButton;
class CMultiLineLabel;
class CListBox;
class CCommanderArtPlace;

class CCommanderSkillIcon : public LRClickableAreaWText //TODO: maybe bring commander skill button initialization logic inside?
{
	std::shared_ptr<CIntObject> object; // passive object that will be used to determine clickable area
public:
	CCommanderSkillIcon(std::shared_ptr<CIntObject> object_, std::function<void()> callback);

	std::function<void()> callback;

	void clickLeft(tribool down, bool previousState) override;
	void clickRight(tribool down, bool previousState) override;

	void setObject(std::shared_ptr<CIntObject> object);
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
	private:
		std::shared_ptr<CPicture> background;
	protected:
		CStackWindow * parent;
	public:
		CWindowSection(CStackWindow * parent, std::string backgroundPath, int yOffset);
	};

	class ActiveSpellsSection : public CWindowSection
	{
		std::vector<std::shared_ptr<CAnimImage>> spellIcons;
		std::vector<std::shared_ptr<LRClickableAreaWText>> clickableAreas;
	public:
		ActiveSpellsSection(CStackWindow * owner, int yOffset);
	};

	class BonusLineSection : public CWindowSection
	{
		std::array<std::shared_ptr<CPicture>, 2> icon;
		std::array<std::shared_ptr<CLabel>, 2> name;
		std::array<std::shared_ptr<CMultiLineLabel>, 2> description;
	public:
		BonusLineSection(CStackWindow * owner, size_t lineIndex);
	};

	class BonusesSection : public CWindowSection
	{
		std::shared_ptr<CListBox> lines;
	public:
		BonusesSection(CStackWindow * owner, int yOffset, std::optional<size_t> preferredSize = std::optional<size_t>());
	};

	class ButtonsSection : public CWindowSection
	{
		std::shared_ptr<CButton> dismiss;
		std::array<std::shared_ptr<CButton>, 3> upgrade;// no more than 3 buttons - space limit
		std::shared_ptr<CButton> exit;
	public:
		ButtonsSection(CStackWindow * owner, int yOffset);
	};

	class CommanderMainSection : public CWindowSection
	{
		std::vector<std::shared_ptr<CCommanderSkillIcon>> skillIcons;
		std::vector<std::shared_ptr<CCommanderArtPlace>> artifacts;

		std::shared_ptr<CPicture> abilitiesBackground;
		std::shared_ptr<CListBox> abilities;

		std::shared_ptr<CButton> leftBtn;
		std::shared_ptr<CButton> rightBtn;
	public:
		CommanderMainSection(CStackWindow * owner, int yOffset);
	};

	class MainSection : public CWindowSection
	{
		enum class EStat : size_t
		{
			ATTACK,
			DEFENCE,
			SHOTS,
			DAMAGE,
			HEALTH,
			HEALTH_LEFT,
			SPEED,
			MANA,
			AFTER_LAST
		};

		std::shared_ptr<CCreaturePic> animation;
		std::shared_ptr<CLabel> name;
		std::shared_ptr<CPicture> icons;
		std::shared_ptr<MoraleLuckBox> morale;
		std::shared_ptr<MoraleLuckBox> luck;

		std::vector<std::shared_ptr<CLabel>> stats;

		std::shared_ptr<CAnimImage> expRankIcon;
		std::shared_ptr<LRClickableAreaWText> expArea;
		std::shared_ptr<CLabel> expLabel;

		void addStatLabel(EStat index, int64_t value1, int64_t value2);
		void addStatLabel(EStat index, int64_t value);

		static std::string getBackgroundName(bool showExp, bool showArt);

		std::array<std::string, 8> statNames;
		std::array<std::string, 8> statFormats;
	public:
		MainSection(CStackWindow * owner, int yOffset, bool showExp, bool showArt);
	};

	std::shared_ptr<CAnimImage> stackArtifactIcon;
	std::shared_ptr<LRClickableAreaWTextComp> stackArtifactHelp;
	std::shared_ptr<CButton> stackArtifactButton;


	std::shared_ptr<UnitView> info;
	std::vector<BonusInfo> activeBonuses;
	size_t activeTab;
	std::shared_ptr<CTabbedInt> commanderTab;

	std::map<size_t, std::shared_ptr<CButton>> switchButtons;

	std::shared_ptr<CWindowSection> mainSection;
	std::shared_ptr<CWindowSection> activeSpellsSection;
	std::shared_ptr<CWindowSection> commanderMainSection;
	std::shared_ptr<CWindowSection> commanderBonusesSection;
	std::shared_ptr<CWindowSection> bonusesSection;
	std::shared_ptr<CWindowSection> buttonsSection;

	std::shared_ptr<CCommanderSkillIcon> selectedIcon;
	si32 selectedSkill;

	void setSelection(si32 newSkill, std::shared_ptr<CCommanderSkillIcon> newIcon);
	std::shared_ptr<CIntObject> switchTab(size_t index);

	void removeStackArtifact(ArtifactPosition pos);

	void initSections();
	void initBonusesList();

	void init();

	std::string generateStackExpDescription();

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
