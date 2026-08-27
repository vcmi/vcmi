/*
 * CDwellingAbandonWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CDwellingAbandonWindow.h"

#include "../CPlayerInterface.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../adventureMap/AdventureMapInterface.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/mapObjects/CGDwelling.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/texts/CGeneralTextHandler.h"

CDwellingAbandonMarker::CDwellingAbandonMarker(int x, int y)
	: CAnimImage(AnimationPath::builtin("VwSymbol.def"), 3, 0, x, y)
{
	addUsedEvents(LCLICK);
}

void CDwellingAbandonMarker::clickPressed(const Point & cursorPosition)
{
	callback();
}

void CDwellingAbandonMarker::showAll(Canvas & to)
{
	CanvasClipRectGuard guard(to, parent->pos);
	CAnimImage::showAll(to);
}

CDwellingAbandonMinimap::CDwellingAbandonMinimap(const Rect & position)
	: CMinimap(position)
	, markerTile(-1, -1, -1)
{
}

void CDwellingAbandonMinimap::setTile(const int3 & tile)
{
	markerTile = tile;
	CMinimap::update();
	placeMark();
}

void CDwellingAbandonMinimap::placeMark()
{
	OBJECT_CONSTRUCTION;
	icon.reset();

	if (markerTile.x < 0)
		return;

	onMapViewMoved(Rect(), markerTile.z);

	Point offset = tileToPixels(markerTile);
	icon = std::make_shared<CDwellingAbandonMarker>(offset.x, offset.y);
	icon->moveBy(Point(-icon->pos.w / 2, -icon->pos.h / 2));

	int3 tile = markerTile;
	icon->callback = [tile]()
	{
		adventureInt->centerOnTile(tile);
	};
}

void CDwellingAbandonMinimap::showAll(Canvas & to)
{
	CIntObject::showAll(to);
}

CDwellingAbandonWindow::CInstanceItem::CInstanceItem(CDwellingAbandonWindow * parent, size_t index)
	: CIntObject(LCLICK)
	, parent(parent)
	, index(index)
{
	OBJECT_CONSTRUCTION;

	const CGObjectInstance * obj = parent->instances[index];
	int3 tile = obj->visitablePos();

	pos.w = 190;
	pos.h = 20;

	text = std::make_shared<CLabel>(4, pos.h / 2, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE,
		obj->getObjectName() + " (" + std::to_string(tile.x) + ", " + std::to_string(tile.y) + ")");
}

void CDwellingAbandonWindow::CInstanceItem::clickPressed(const Point & cursorPosition)
{
	parent->selectItem(index);
}

CDwellingAbandonWindow::CDwellingAbandonWindow(const std::vector<const CGObjectInstance *> & Instances, const std::function<void()> & OnChanged)
	: CWindowObject(BORDERED | PLAYER_COLORED)
	, instances(Instances)
	, onChanged(OnChanged)
	, selected(-1)
{
	OBJECT_CONSTRUCTION;

	pos.w = 540;
	pos.h = 440;

	background = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));

	std::string headerText = LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.title");
	boost::algorithm::replace_first(headerText, "%s", instances.empty() ? "" : instances.front()->getObjectName());
	title = std::make_shared<CLabel>(pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, headerText);

	const int visibleItems = 6;
	const int listHeight = visibleItems * 24;

	listBackground = std::make_shared<TransparentFilledRectangle>(Rect(10, 46, 200, listHeight + 10), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));

	list = std::make_shared<CListBox>(std::bind(&CDwellingAbandonWindow::createItem, this, _1),
		Point(14, 50), Point(0, 24), visibleItems, instances.size(), 0, 1, Rect(214, 50, listHeight, listHeight));

	if (auto slider = list->getSlider())
	{
		if (instances.size() > static_cast<size_t>(visibleItems))
			slider->block(false);
		else
		{
			slider->block(true);
			slider->scrollToMin();
		}
	}

	minimap = std::make_shared<CDwellingAbandonMinimap>(Rect(295, 50, 180, 180));

	infoBackground = std::make_shared<TransparentFilledRectangle>(Rect(246, 246, 278, 120), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));

	infoText = std::make_shared<CMultiLineLabel>(Rect(356, 256, 158, 100), FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);

	abandonButton = std::make_shared<CButton>(Point(325, 376), AnimationPath::builtin("ICANCEL.DEF"),
		CButton::tooltipLocalized("vcmi.kingdomOverview.abandonDwelling"), std::bind(&CDwellingAbandonWindow::abandon, this));
	abandonButton->block(true);

	closeButton = std::make_shared<CButton>(Point(395, 376), AnimationPath::builtin("IOKAY.DEF"), CButton::tooltip(),
		std::bind(&CDwellingAbandonWindow::close, this), EShortcut::GLOBAL_RETURN);

	if (!instances.empty())
		selectItem(0);

	center();
}

std::shared_ptr<CIntObject> CDwellingAbandonWindow::createItem(size_t index)
{
	if (index < instances.size())
		return std::make_shared<CInstanceItem>(this, index);
	return std::shared_ptr<CIntObject>();
}

void CDwellingAbandonWindow::selectItem(size_t index)
{
	if (index >= instances.size())
		return;

	selected = static_cast<int>(index);
	minimap->setTile(instances[index]->visitablePos());
	updateInfo(instances[index]);
	abandonButton->block(false);
	redraw();
}

void CDwellingAbandonWindow::updateInfo(const CGObjectInstance * obj)
{
	OBJECT_CONSTRUCTION;
	creatureIcon.reset();

	std::string text;

	if (const auto * dwelling = dynamic_cast<const CGDwelling *>(obj))
	{
		if (!dwelling->creatures.empty() && !dwelling->creatures.front().second.empty())
		{
			const CCreature * creature = dwelling->creatures.front().second.front().toCreature();

			creatureIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("TWCRPORT"), creature->getIconIndex(), 0, 256, 256);

			text += creature->getNamePluralTranslated() + "\n\n";
			text += LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.available")
				+ ": " + std::to_string(dwelling->creatures.front().first) + "\n";
			text += LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.growth")
				+ ": " + std::to_string(creature->getGrowth());
		}
	}

	infoText->setText(text);
}

void CDwellingAbandonWindow::abandon()
{
	if (selected < 0 || selected >= static_cast<int>(instances.size()))
		return;

	const CGObjectInstance * obj = instances[selected];

	GAME->interface()->showYesNoDialog(
		LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.confirm"),
		[this, obj]()
		{
			GAME->interface()->cb->abandonObjectOwnership(obj);

			instances.erase(std::remove(instances.begin(), instances.end(), obj), instances.end());

			if (onChanged)
				onChanged();

			if (instances.empty())
			{
				close();
				return;
			}

			selected = -1;
			abandonButton->block(true);
			minimap->setTile(int3(-1, -1, -1));
			list->resize(instances.size());
			selectItem(0);
		},
		nullptr
	);
}

void CDwellingAbandonWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);

	to.drawBorder(Rect::createAround(minimap->pos, 1), Colors::METALLIC_GOLD);

	if (creatureIcon)
		to.drawBorder(Rect::createAround(creatureIcon->pos, 1), Colors::METALLIC_GOLD);

	if (selected < 0)
		return;

	auto item = list->getItem(static_cast<size_t>(selected));
	if (!item)
		return;

	Rect selection = Rect::createAround(item->pos, 1);
	to.drawBorder(selection, Colors::METALLIC_GOLD);
}
