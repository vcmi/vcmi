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

#include "CJournalWindow.h"

#include "../adventureMap/CMinimap.h"
#include "../widgets/Images.h"

#include "../../lib/gameState/ScenarioEventJournalEntry.h"

class ScenarioEventJournalMinimap : public CMinimap
{
	int3 location = int3(-1, -1, -1);
	std::shared_ptr<CPicture> marker;

	void markerClicked() const;

	void clickPressed(const Point & cursorPosition) override {};
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override {};

public:
	explicit ScenarioEventJournalMinimap(const Rect & position);

	void setLocation(const int3 & newLocation);
	void showAll(Canvas & to) override;
};

class ScenarioEventJournal : public JournalWindow
{
	const std::vector<ScenarioEventJournalEntry> entries;
	std::shared_ptr<ScenarioEventJournalMinimap> minimap;

	size_t getItemCount() const override;
	std::string getItemText(size_t itemIndex) const override;
	void onItemSelected(size_t itemIndex) override;
	void updateMinimap() override;

public:
	explicit ScenarioEventJournal(const std::vector<ScenarioEventJournalEntry> & journalEntries);
};
