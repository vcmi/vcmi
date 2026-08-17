/*
 * CScenarioEventJournal.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"

#include "../adventureMap/CMinimap.h"
#include "../widgets/Images.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"

#include "../../lib/gameState/ScenarioEventJournalEntry.h"

class CButton;
class CToggleGroup;
class CComponentBox;
class CSlider;

class CScenarioEventJournalLabel : public LRClickableAreaWText, public CMultiLineLabel
{
public:
	std::function<void()> callback;

	CScenarioEventJournalLabel(const Rect & position, const std::string & text);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

class CScenarioEventJournalMinimap : public CMinimap
{
	int3 location = int3(-1, -1, -1);
	std::shared_ptr<CPicture> marker;

	void markerClicked() const;

	void clickPressed(const Point & cursorPosition) override {};
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override {};

public:
	explicit CScenarioEventJournalMinimap(const Rect & position);

	void setLocation(const int3 & newLocation);
	void showAll(Canvas & to) override;
};

class CScenarioEventJournal : public CWindowObject
{
	static constexpr int VISIBLE_ENTRY_COUNT = 6;
	static constexpr int DESCRIPTION_TOP = 64;
	static constexpr int DESCRIPTION_HEIGHT = 309;

	int selectedLabel = -1;
	const std::vector<ScenarioEventJournalEntry> entries;
	std::vector<std::shared_ptr<CScenarioEventJournalLabel>> labels;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<CComponentBox> componentsBox;
	std::shared_ptr<CScenarioEventJournalMinimap> minimap;
	std::shared_ptr<CSlider> slider;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CToggleGroup> journalTabs;

	void selectEntry(size_t entryIndex, int labelIndex);
	void recreateEntryList(int firstVisible);
	void sliderMoved(int newPosition);

public:
	explicit CScenarioEventJournal(const std::vector<ScenarioEventJournalEntry> & journalEntries);

	void showAll(Canvas & to) override;
};
