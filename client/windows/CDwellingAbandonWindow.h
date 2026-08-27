/*
 * CDwellingAbandonWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CWindowObject.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../adventureMap/CMinimap.h"
#include "../../lib/int3.h"

class CGObjectInstance;
class CButton;
class CAnimImage;
class CSlider;
class FilledTexturePlayerColored;
class TransparentFilledRectangle;

/// Single map marker shown on CDwellingAbandonMinimap, centers the main map when clicked
class CDwellingAbandonMarker : public CAnimImage
{
public:
	std::function<void()> callback;

	CDwellingAbandonMarker(int x, int y);

	void clickPressed(const Point & cursorPosition) override;
	void showAll(Canvas & to) override;
};

/// Minimap showing the location of a single owned object, reused per selected list entry
class CDwellingAbandonMinimap : public CMinimap
{
	std::shared_ptr<CDwellingAbandonMarker> icon;
	int3 markerTile;

	void clickPressed(const Point & cursorPosition) override {}
	void mouseDragged(const Point & cursorPosition, const Point & lastUpdateDistance) override {}

	void placeMark();

public:
	explicit CDwellingAbandonMinimap(const Rect & position);

	void setTile(const int3 & tile);

	void showAll(Canvas & to) override;
};

/// Lets the player pick one of several same-type owned dwellings (or other IOwnableObject
/// instances) and release it back to neutral ownership
class CDwellingAbandonWindow : public CWindowObject
{
	/// A single clickable row in the instance list, positioned directly by recreateItemList().
	/// The row's own pos spans the full width (used for the selection border and click area);
	/// the text label inside it is inset a few pixels so it doesn't touch the row's edge.
	class CInstanceItem : public CIntObject
	{
		CDwellingAbandonWindow * parent;
		std::shared_ptr<CMultiLineLabel> text;

	public:
		const size_t index;

		CInstanceItem(CDwellingAbandonWindow * parent, size_t index, const Rect & position, const std::string & text);

		void clickPressed(const Point & cursorPosition) override;
	};

	static constexpr int LIST_VISIBLE_COUNT = 6;
	static constexpr int LIST_ITEM_HEIGHT = 24;
	static constexpr int LIST_X = 10;
	static constexpr int LIST_Y = 46;
	static constexpr int LIST_ITEM_X = LIST_X + 1;
	static constexpr int LIST_ITEM_Y = LIST_Y + 1;
	static constexpr int LIST_ITEM_WIDTH = 195;

	std::vector<const CGObjectInstance *> instances;
	std::function<void()> onChanged;

	std::shared_ptr<FilledTexturePlayerColored> background;
	std::shared_ptr<TransparentFilledRectangle> listBackground;
	std::shared_ptr<TransparentFilledRectangle> infoBackground;
	std::shared_ptr<CLabel> title;
	std::vector<std::shared_ptr<CInstanceItem>> items;
	std::shared_ptr<CSlider> listSlider;
	std::shared_ptr<CDwellingAbandonMinimap> minimap;
	std::shared_ptr<CAnimImage> creatureIcon;
	std::shared_ptr<CMultiLineLabel> infoText;
	std::shared_ptr<CButton> abandonButton;
	std::shared_ptr<CButton> closeButton;

	int selected;

	void recreateItemList(int firstVisible);
	void sliderMoved(int newPos);
	void selectItem(size_t index);
	void updateInfo(const CGObjectInstance * obj);
	void abandon();

public:
	CDwellingAbandonWindow(const std::vector<const CGObjectInstance *> & Instances, const std::function<void()> & OnChanged);

	void showAll(Canvas & to) override;
};
