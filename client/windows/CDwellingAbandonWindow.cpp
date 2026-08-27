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

CDwellingAbandonWindow::CInstanceItem::CInstanceItem(CDwellingAbandonWindow * parent, size_t index, const Rect & position, const std::string & itemText)
	: CIntObject(LCLICK, position.topLeft())
	, parent(parent)
	, index(index)
{
	OBJECT_CONSTRUCTION;

	pos.w = position.w;
	pos.h = position.h;

	text = std::make_shared<CMultiLineLabel>(Rect(4, 0, position.w - 4, position.h), FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, itemText);
}

void CDwellingAbandonWindow::CInstanceItem::clickPressed(const Point & cursorPosition)
{
	parent->selectItem(index);
}

CDwellingAbandonWindow::CDwellingAbandonWindow(const std::vector<const CGObjectInstance *> & Instances)
	: CWindowObject(BORDERED | PLAYER_COLORED)
	, instances(Instances)
	, selected(-1)
{
	OBJECT_CONSTRUCTION;

	pos.w = 540;
	pos.h = 440;
	center();

	background = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));

	std::string headerText = LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.title");
	boost::algorithm::replace_first(headerText, "%s", instances.empty() ? "" : instances.front()->getObjectName());
	title = std::make_shared<CLabel>(pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, headerText);

	const int listPanelHeight = LIST_VISIBLE_COUNT * LIST_ITEM_HEIGHT + 1;
	const int listSliderLength = LIST_VISIBLE_COUNT * LIST_ITEM_HEIGHT;
	const int listPanelWidth = LIST_ITEM_WIDTH + 3;

	listBackground = std::make_shared<TransparentFilledRectangle>(Rect(LIST_X, LIST_Y, listPanelWidth, listPanelHeight), ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64));

	listSlider = std::make_shared<CSlider>(Point(LIST_X + listPanelWidth - 1, LIST_ITEM_Y - 1), listSliderLength,
		std::bind(&CDwellingAbandonWindow::sliderMoved, this, _1), LIST_VISIBLE_COUNT, static_cast<int>(instances.size()), 0, Orientation::VERTICAL, CSlider::BROWN);
	listSlider->setPanningStep(LIST_ITEM_HEIGHT);
	if (instances.size() <= static_cast<size_t>(LIST_VISIBLE_COUNT))
	{
		listSlider->block(true);
		listSlider->scrollToMin();
	}

	rebuildItems();

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
}

void CDwellingAbandonWindow::recreateItemList(int firstVisible)
{
	for (size_t i = 0; i < items.size(); ++i)
	{
		items[i]->moveTo(Point(pos.x + LIST_ITEM_X, pos.y + LIST_ITEM_Y + (static_cast<int>(i) - firstVisible) * LIST_ITEM_HEIGHT));
		if (static_cast<int>(i) >= firstVisible && static_cast<int>(i) < firstVisible + LIST_VISIBLE_COUNT)
			items[i]->enable();
		else
			items[i]->disable();
	}
	redraw();
}

void CDwellingAbandonWindow::sliderMoved(int newPos)
{
	recreateItemList(newPos);
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

void CDwellingAbandonWindow::rebuildItems()
{
	OBJECT_CONSTRUCTION;

	items.clear();
	for (size_t i = 0; i < instances.size(); ++i)
	{
		const CGObjectInstance * obj = instances[i];
		int3 tile = obj->visitablePos();
		std::string itemText = obj->getObjectName() + " (" + std::to_string(tile.x) + ", " + std::to_string(tile.y) + ")";
		items.push_back(std::make_shared<CInstanceItem>(this, i, Rect(LIST_ITEM_X, LIST_ITEM_Y, LIST_ITEM_WIDTH, LIST_ITEM_HEIGHT - 2), itemText));
	}

	listSlider->setAmount(static_cast<int>(instances.size()));
	if (instances.size() <= static_cast<size_t>(LIST_VISIBLE_COUNT))
	{
		listSlider->block(true);
		listSlider->scrollToMin();
	}
	else
		listSlider->block(false);

	recreateItemList(0);
}

void CDwellingAbandonWindow::abandon()
{
	if (selected < 0 || selected >= static_cast<int>(instances.size()))
		return;

	const CGObjectInstance * obj = instances[selected];

	GAME->interface()->showYesNoDialog(
		LIBRARY->generaltexth->translate("vcmi.kingdomOverview.abandonDwelling.confirm"),
		[obj]()
		{
			// Only sends the request - the list/minimap update once the server confirms
			// the change, via onOwnershipChanged() (see CPlayerInterface::objectPropertyChanged).
			GAME->interface()->cb->abandonObjectOwnership(obj);
		},
		nullptr
	);
}

void CDwellingAbandonWindow::onOwnershipChanged(const ObjectInstanceID & id)
{
	auto it = std::find_if(instances.begin(), instances.end(), [&id](const CGObjectInstance * obj)
	{
		return obj->id == id;
	});

	if (it == instances.end())
		return;

	instances.erase(it);

	if (instances.empty())
	{
		close();
		return;
	}

	selected = -1;
	abandonButton->block(true);
	minimap->setTile(int3(-1, -1, -1));

	rebuildItems();
	selectItem(0);

	redraw();
}

void CDwellingAbandonWindow::showAll(Canvas & to)
{
	CWindowObject::showAll(to);

	to.drawBorder(Rect::createAround(minimap->pos, 1), Colors::METALLIC_GOLD);

	if (creatureIcon)
		to.drawBorder(Rect::createAround(creatureIcon->pos, 1), Colors::METALLIC_GOLD);

	if (selected < 0 || selected >= static_cast<int>(items.size()))
		return;

	if (items[selected]->isDisabled())
		return;

	Rect selection = Rect::createAround(items[selected]->pos, 1);
	to.drawBorder(selection, Colors::METALLIC_GOLD);
}
