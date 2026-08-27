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
#include "../adventureMap/CMinimap.h"
#include "../../lib/int3.h"

class CGObjectInstance;
class CListBox;
class CLabel;
class CMultiLineLabel;
class CButton;
class CAnimImage;
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
	class CInstanceItem : public CIntObject
	{
		CDwellingAbandonWindow * parent;
		std::shared_ptr<CLabel> text;

	public:
		const size_t index;

		CInstanceItem(CDwellingAbandonWindow * parent, size_t index);

		void clickPressed(const Point & cursorPosition) override;
	};

	std::vector<const CGObjectInstance *> instances;
	std::function<void()> onChanged;

	std::shared_ptr<FilledTexturePlayerColored> background;
	std::shared_ptr<TransparentFilledRectangle> listBackground;
	std::shared_ptr<TransparentFilledRectangle> infoBackground;
	std::shared_ptr<CLabel> title;
	std::shared_ptr<CListBox> list;
	std::shared_ptr<CDwellingAbandonMinimap> minimap;
	std::shared_ptr<CAnimImage> creatureIcon;
	std::shared_ptr<CMultiLineLabel> infoText;
	std::shared_ptr<CButton> abandonButton;
	std::shared_ptr<CButton> closeButton;

	int selected;

	std::shared_ptr<CIntObject> createItem(size_t index);
	void selectItem(size_t index);
	void updateInfo(const CGObjectInstance * obj);
	void abandon();

public:
	CDwellingAbandonWindow(const std::vector<const CGObjectInstance *> & Instances, const std::function<void()> & OnChanged);

	void showAll(Canvas & to) override;
};
