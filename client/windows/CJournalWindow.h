/*
 * CJournalWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"

#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"

class CButton;
class CComponent;
class CComponentBox;
class CSlider;
class CToggleGroup;

enum class EJournalMode : uint8_t
{
	QUESTS,
	EVENTS
};

class JournalLabel : public LRClickableAreaWText, public CMultiLineLabel
{
public:
	std::function<void()> callback;

	JournalLabel(const Rect & position, const std::string & text);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

/// Shared journal window chrome and list controller. Derived classes provide the
/// quest/event model and update their minimap when an item is selected.
class JournalWindow : public CWindowObject
{
	static constexpr int VISIBLE_ITEM_COUNT = 6;
	static constexpr int DESCRIPTION_TOP = 64;
	static constexpr int DESCRIPTION_HEIGHT = 309;

	EJournalMode mode;
	int selectedLabel = -1;
	std::vector<std::shared_ptr<JournalLabel>> labels;
	std::shared_ptr<CSlider> slider;
	std::shared_ptr<CButton> ok;
	std::shared_ptr<CToggleGroup> journalTabs;
	std::shared_ptr<CTextBox> description;
	std::shared_ptr<CComponentBox> componentsBox;

	void selectItem(size_t itemIndex);
	void recreateItemList(int firstVisible);
	void sliderMoved(int newPosition);

protected:
	explicit JournalWindow(EJournalMode mode);

	void initializeItems();
	void setContent(const std::string & text, std::vector<std::shared_ptr<CComponent>> components, int componentAreaHeight = 130);

	virtual size_t getItemCount() const = 0;
	virtual std::string getItemText(size_t itemIndex) const = 0;
	virtual void onItemSelected(size_t itemIndex) = 0;
	virtual void updateMinimap() = 0;

public:
	void showAll(Canvas & to) override;
};
