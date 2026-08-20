/*
 * CQuestLog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CJournalWindow.h"

#include "../widgets/Images.h"
#include "../adventureMap/CMinimap.h"

#include "../../lib/gameState/QuestInfo.h"

class CCreature;
class CStackInstance;
class CGHeroInstance;
struct QuestInfo;

class CPicture;
class CCreaturePic;
class LRClickableAreaWTextComp;
class CLabel;

class CQuestIcon : public CAnimImage
{
public:
	std::function<void()> callback; //TODO: merge with other similar classes?

	CQuestIcon(const AnimationPath & defname, int index, int x=0, int y=0);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

class CQuestMinimap : public CMinimap
{
	std::vector<std::shared_ptr<CQuestIcon>> icons;
	std::vector<int3> markerTiles; // computed once per selected quest, placed each redraw

	void clickPressed(const Point & cursorPosition) override{}; //minimap ignores clicking on its surface
	void iconClicked();
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override{};

	void placeMarks(); // (re)create icons from markerTiles

public:
	const QuestInfo * currentQuest;

	CQuestMinimap(const Rect & position);
	//should be called to invalidate whole map - different player or level
	void update();
	/// Select the quest whose markers are shown, recomputing the marker tiles.
	void setQuest(const QuestInfo * q);

	void showAll(Canvas & to) override;
};

class CQuestLog : public JournalWindow
{
	const QuestInfo * currentQuest;

	const std::vector<QuestInfo> quests;
	std::shared_ptr<CQuestMinimap> minimap;

	size_t getItemCount() const override;
	std::string getItemText(size_t itemIndex) const override;
	void onItemSelected(size_t itemIndex) override;
	void updateMinimap() override;

public:
	CQuestLog(const std::vector<QuestInfo> & Quests);
};
