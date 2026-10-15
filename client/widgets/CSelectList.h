/*
 * CSelectList.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CSlider;
class IImage;

/// Scrollable list of text rows. Rows can be selected or checked, have an icon and an indentation level
class CSelectList : public CIntObject
{
public:
	enum class Mode
	{
		SINGLE,      ///< click selects the row
		CHECK,       ///< click toggles the check mark of the row
		CHECK_SELECT ///< click on the check mark toggles it, click elsewhere selects the row
	};

	enum class Branch
	{
		NONE,
		COLLAPSED,
		EXPANDED
	};

	struct Row
	{
		std::string text;
		std::string detail; ///< secondary text aligned to the right
		std::shared_ptr<IImage> icon;
		int indent = 0;
		Branch branch = Branch::NONE;
		bool checked = false;
		bool checked2 = false; ///< second check mark, see setRightChecks()
		bool enabled = true;
		bool separator = false;
	};

private:
	Mode mode;
	int rowHeight;
	int rightChecks = 0;
	int firstRow = 0;
	int selectedRow = -1;
	int hoveredRow = -1;
	int panningAccumulated = 0;
	std::vector<Row> rows;
	std::shared_ptr<CSlider> slider;

	int visibleRows() const;
	int rowAt(const Point & position) const;
	Rect contentRect() const;
	Rect checkArea(int index) const;
	Rect rightCheckArea(int index, int column) const;
	int rightCheckAt(int index, const Point & position) const;
	void updateSlider();
	void onSliderMoved(int value);

public:
	/// Called when a row gets selected by a click
	std::function<void(int)> onSelect;
	/// Called on double click on a row
	std::function<void(int)> onActivate;
	/// Called on right click or long press on a row
	std::function<void(int)> onPopup;
	/// Called when a row gets checked or unchecked by a click, in CHECK mode
	std::function<void(int, bool)> onCheck;
	/// Called when the expand / collapse marker of a row is clicked
	std::function<void(int)> onBranch;
	/// Called when the second check mark of a row gets toggled
	std::function<void(int, bool)> onCheck2;

	CSelectList(const Rect & area, Mode mode, int rowHeight = 22);
	~CSelectList() override;

	void setRows(std::vector<Row> newRows, bool keepScroll = false);
	const std::vector<Row> & getRows() const;
	size_t size() const;

	void setSelected(int index);
	int getSelected() const;

	void setChecked(int index, bool on);
	bool isChecked(int index) const;
	std::vector<int> getCheckedIndexes() const;
	void setAllChecked(bool on);

	/// Shows one or two check marks at the right edge of every row instead of the one on the left.
	/// The first one is Row::checked and reported by onCheck, the second is Row::checked2 and reported by onCheck2
	void setRightChecks(int count);

	/// Scrolls the list so that the row is visible
	void scrollTo(int index);
	void scrollBy(int rowsCount);

	void clickPressed(const Point & cursorPosition) override;
	void clickDouble(const Point & cursorPosition) override;
	void showPopupWindow(const Point & cursorPosition) override;
	void wheelScrolled(int distance) override;
	void mouseMoved(const Point & cursorPosition, const Point & lastUpdateDistance) override;
	void hover(bool on) override;
	void gesture(bool on, const Point & initialPosition, const Point & finalPosition) override;
	void gesturePanning(const Point & initialPosition, const Point & currentPosition, const Point & lastUpdateDistance) override;
	void showAll(Canvas & to) override;
};
