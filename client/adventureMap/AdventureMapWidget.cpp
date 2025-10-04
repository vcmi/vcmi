/*
 * CAdventureMapWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AdventureMapWidget.h"

#include "AdventureMapShortcuts.h"
#include "CInfoBar.h"
#include "CList.h"
#include "CMinimap.h"
#include "CResDataBar.h"
#include "AdventureState.h"

#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../mapView/MapView.h"
#include "../render/AssetGenerator.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/callback/CCallback.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/filesystem/ResourcePath.h"
#include "../../lib/entities/ResourceTypeHandler.h"

AdventureMapWidget::AdventureMapWidget( std::shared_ptr<AdventureMapShortcuts> shortcuts )
	: shortcuts(shortcuts)
	, mapLevel(0)
{
	pos.x = pos.y = 0;
	pos.w = ENGINE->screenDimensions().x;
	pos.h = ENGINE->screenDimensions().y;

	REGISTER_BUILDER("adventureInfobar",            &AdventureMapWidget::buildInfobox             );
	REGISTER_BUILDER("adventureMapImage",           &AdventureMapWidget::buildMapImage            );
	REGISTER_BUILDER("adventureMapButton",          &AdventureMapWidget::buildMapButton           );
	REGISTER_BUILDER("adventureMapContainer",       &AdventureMapWidget::buildMapContainer        );
	REGISTER_BUILDER("adventureMapGameArea",        &AdventureMapWidget::buildMapGameArea         );
	REGISTER_BUILDER("adventureMapHeroList",        &AdventureMapWidget::buildMapHeroList         );
	REGISTER_BUILDER("adventureMapIcon",            &AdventureMapWidget::buildMapIcon             );
	REGISTER_BUILDER("adventureMapTownList",        &AdventureMapWidget::buildMapTownList         );
	REGISTER_BUILDER("adventureMinimap",            &AdventureMapWidget::buildMinimap             );
	REGISTER_BUILDER("adventureResourceDateBar",    &AdventureMapWidget::buildResourceDateBar     );
	REGISTER_BUILDER("adventureStatusBar",          &AdventureMapWidget::buildStatusBar           );
	REGISTER_BUILDER("adventurePlayerTexture",      &AdventureMapWidget::buildTexturePlayerColored);
	REGISTER_BUILDER("adventureResourceAdditional", &AdventureMapWidget::buildResourceAdditional  );

	for (const auto & entry : shortcuts->getShortcuts())
		addShortcut(entry.shortcut, entry.callback);

	const JsonNode config(JsonPath::builtin("config/widgets/adventureMap.json"));

	for(const auto & entry : config["options"]["imagesPlayerColored"].Vector())
		playerColoredImages.push_back(ImagePath::fromJson(entry));

	build(config);
	addUsedEvents(KEYBOARD);
}

void AdventureMapWidget::onMapViewMoved(const Rect & visibleArea, int newMapLevel)
{
	if(mapLevel == newMapLevel)
		return;

	mapLevel = newMapLevel;
	updateActiveState();
}

Rect AdventureMapWidget::readSourceArea(const JsonNode & source, const JsonNode & sourceCommon)
{
	const auto & input = source.isNull() ? sourceCommon : source;

	return readArea(input, Rect(Point(0, 0), Point(800, 600)));
}

Rect AdventureMapWidget::readTargetArea(const JsonNode & source)
{
	if(subwidgetSizes.empty())
		return readArea(source, pos);
	return readArea(source, subwidgetSizes.back());
}

Rect AdventureMapWidget::readArea(const JsonNode & source, const Rect & boundingBox)
{
	const auto & object = source.Struct();

	if(object.count("left") + object.count("width") + object.count("right") != 2)
		logGlobal->error("Invalid area definition in widget! Unable to load width!");

	if(object.count("top") + object.count("height") + object.count("bottom") != 2)
		logGlobal->error("Invalid area definition in widget! Unable to load height!");

	int left = source["left"].Integer();
	int width = source["width"].Integer();
	int right = source["right"].Integer();

	int top = source["top"].Integer();
	int height = source["height"].Integer();
	int bottom = source["bottom"].Integer();

	Point topLeft(left, top);
	Point dimensions(width, height);

	if(source["left"].isNull())
		topLeft.x = boundingBox.w - right - width;

	if(source["width"].isNull())
		dimensions.x = boundingBox.w - right - left;

	if(source["top"].isNull())
		topLeft.y = boundingBox.h - bottom - height;

	if(source["height"].isNull())
		dimensions.y = boundingBox.h - bottom - top;

	return Rect(topLeft + boundingBox.topLeft(), dimensions);
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildInfobox(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	infoBar = std::make_shared<CInfoBar>(area);
	return infoBar;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapImage(const JsonNode & input)
{
	Rect targetArea = readTargetArea(input["area"]);
	Rect sourceArea = readSourceArea(input["sourceArea"], input["area"]);
	ImagePath path = ImagePath::fromJson(input["image"]);

	if (vstd::contains(playerColoredImages, path))
		return std::make_shared<FilledTexturePlayerIndexed>(path, targetArea, sourceArea);
	else
		return std::make_shared<CFilledTexture>(path, targetArea, sourceArea);
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapButton(const JsonNode & input)
{
	auto position = readTargetArea(input["area"]);
	auto image = AnimationPath::fromJson(input["image"]);
	auto help = readHintText(input["help"]);
	bool playerColored = input["playerColored"].Bool();

	if(!input["generateFromBaseImage"].isNull())
	{
		bool small = input["generateSmall"].Bool();
		auto assetGenerator = ENGINE->renderHandler().getAssetGenerator();
		auto layout = assetGenerator->createAdventureMapButton(ImagePath::fromJson(input["generateFromBaseImage"]), small);
		assetGenerator->addAnimationFile(AnimationPath::builtin("SPRITES/" + input["image"].String()), layout);
		ENGINE->renderHandler().updateGeneratedAssets();
	}

	auto button = std::make_shared<CButton>(position.topLeft(), image, help, 0, EShortcut::NONE, playerColored);

	loadButtonBorderColor(button, input["borderColor"]);
	loadButtonHotkey(button, input["hotkey"]);

	return button;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapContainer(const JsonNode & input)
{
	auto position = readTargetArea(input["area"]);
	std::shared_ptr<CAdventureMapContainerWidget> result;

	if (!input["exists"].isNull())
	{
		if (!input["exists"]["heightMin"].isNull() && input["exists"]["heightMin"].Integer() >= pos.h)
			return nullptr;
		if (!input["exists"]["heightMax"].isNull() && input["exists"]["heightMax"].Integer() < pos.h)
			return nullptr;
		if (!input["exists"]["widthMin"].isNull() && input["exists"]["widthMin"].Integer() >= pos.w)
			return nullptr;
		if (!input["exists"]["widthMax"].isNull() && input["exists"]["widthMax"].Integer() < pos.w)
			return nullptr;
	}

	if (input["overlay"].Bool())
		result = std::make_shared<CAdventureMapOverlayWidget>();
	else
		result = std::make_shared<CAdventureMapContainerWidget>();

	result->disableCondition = input["hideWhen"].String();

	result->moveBy(position.topLeft());
	subwidgetSizes.push_back(position);
	for(const auto & entry : input["items"].Vector())
	{
		auto widget = buildWidget(entry);

		addWidget(entry["name"].String(), widget);
		result->ownedChildren.push_back(widget);

		// FIXME: remove cast and replace it with better check
		if (std::dynamic_pointer_cast<CLabel>(widget) || std::dynamic_pointer_cast<CLabelGroup>(widget))
			result->addChild(widget.get(), true);
		else
			result->addChild(widget.get(), false);
	}
	subwidgetSizes.pop_back();

	return result;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapGameArea(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	mapView = std::make_shared<MapView>(area.topLeft(), area.dimensions());
	return mapView;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapHeroList(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	subwidgetSizes.push_back(area);

	Rect item = readTargetArea(input["item"]);

	Point itemOffset(input["itemsOffset"]["x"].Integer(), input["itemsOffset"]["y"].Integer());
	int itemsCount = input["itemsCount"].Integer();

	auto result = std::make_shared<CHeroList>(itemsCount, area, item.topLeft() - area.topLeft(), itemOffset, GAME->interface()->localState->getWanderingHeroes().size());


	if(!input["scrollUp"].isNull())
		result->setScrollUpButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollUp"])));

	if(!input["scrollDown"].isNull())
		result->setScrollDownButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollDown"])));

	subwidgetSizes.pop_back();

	heroList = result;
	return result;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapIcon(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	size_t index = input["index"].Integer();
	size_t perPlayer = input["perPlayer"].Integer();

	return std::make_shared<CAdventureMapIcon>(area.topLeft(), AnimationPath::fromJson(input["image"]), index, perPlayer);
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMapTownList(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	subwidgetSizes.push_back(area);

	Rect item = readTargetArea(input["item"]);
	Point itemOffset(input["itemsOffset"]["x"].Integer(), input["itemsOffset"]["y"].Integer());
	int itemsCount = input["itemsCount"].Integer();

	auto result = std::make_shared<CTownList>(itemsCount, area, item.topLeft() - area.topLeft(), itemOffset, GAME->interface()->localState->getOwnedTowns().size());


	if(!input["scrollUp"].isNull())
		result->setScrollUpButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollUp"])));

	if(!input["scrollDown"].isNull())
		result->setScrollDownButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollDown"])));

	subwidgetSizes.pop_back();

	townList = result;
	return result;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildMinimap(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	minimap = std::make_shared<CMinimap>(area);
	return minimap;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildResourceDateBar(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	auto image = ImagePath::fromJson(input["image"]);

	auto result = std::make_shared<CResDataBar>(image, area.topLeft());

	for (const auto & i : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		const auto & node = input[i.toResource()->getJsonKey()];

		if(node.isNull())
			continue;

		result->setResourcePosition(i, Point(node["x"].Integer(), node["y"].Integer()));
	}

	result->setDatePosition(Point(input["date"]["x"].Integer(), input["date"]["y"].Integer()));

	return result;
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildStatusBar(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	auto image = ImagePath::fromJson(input["image"]);

	auto background = std::make_shared<CFilledTexture>(image, area);

	return CGStatusBar::create(background);
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildTexturePlayerColored(const JsonNode & input)
{
	logGlobal->debug("Building widget CFilledTexture");
	Rect area = readTargetArea(input["area"]);
	return std::make_shared<FilledTexturePlayerColored>(area);
}

std::shared_ptr<CIntObject> AdventureMapWidget::buildResourceAdditional(const JsonNode & input)
{
	OBJECT_CONSTRUCTION;

	logGlobal->debug("Building widget ResourceAdditional");
	Rect area = readTargetArea(input["area"]);
	auto obj = std::make_shared<CIntObject>();

	int remainingSpace = area.w;
	int resElementSize = 84;
	int fitOffset = 2;
	for(const auto & resource : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		if(resource.getNum() < GameConstants::RESOURCE_QUANTITY)
			continue;

		if(remainingSpace < resElementSize)
			break;

		auto res = std::make_shared<CResDataBar>(ImagePath::builtin("ResBarElement"), area.topRight() + Point(remainingSpace - area.w - resElementSize + fitOffset, 0));
		res->setResourcePosition(resource, Point(35, 3));
		addWidget("", res);
		obj->addChild(res.get());

		auto resIcon = std::make_shared<CAnimImage>(AnimationPath::builtin("SMALRES"), GameResID(resource), 0, res->pos.x + 4, res->pos.y + 2);
		addWidget("", resIcon);
		obj->addChild(resIcon.get());

		remainingSpace -= resElementSize;
	}
	
	area.w = remainingSpace + fitOffset;
	auto texture = std::make_shared<FilledTexturePlayerColored>(area);
	addWidget("", texture);
	obj->addChild(texture.get());

	return obj;
}

std::shared_ptr<CHeroList> AdventureMapWidget::getHeroList()
{
	return heroList;
}

std::shared_ptr<CTownList> AdventureMapWidget::getTownList()
{
	return townList;
}

std::shared_ptr<CMinimap> AdventureMapWidget::getMinimap()
{
	return minimap;
}

std::shared_ptr<MapView> AdventureMapWidget::getMapView()
{
	return mapView;
}

std::shared_ptr<CInfoBar> AdventureMapWidget::getInfoBar()
{
	return infoBar;
}

void AdventureMapWidget::setPlayerColor(const PlayerColor & player)
{
	setPlayerChildren(this, player);
}

void AdventureMapWidget::setPlayerChildren(CIntObject * widget, const PlayerColor & player)
{
	for(auto & entry : widget->children)
	{
		auto container = dynamic_cast<CAdventureMapContainerWidget *>(entry);
		auto icon = dynamic_cast<CAdventureMapIcon *>(entry);
		auto button = dynamic_cast<CButton *>(entry);
		auto resDataBar = dynamic_cast<CResDataBar *>(entry);
		auto textureColored = dynamic_cast<FilledTexturePlayerColored *>(entry);
		auto textureIndexed = dynamic_cast<FilledTexturePlayerIndexed *>(entry);

		if(button)
			button->setPlayerColor(player);

		if(resDataBar)
			resDataBar->setPlayerColor(player);

		if(icon)
			icon->setPlayerColor(player);

		if(container)
			setPlayerChildren(container, player);

		if(textureColored)
			textureColored->setPlayerColor(player);

		if(textureIndexed)
			textureIndexed->setPlayerColor(player);

		if(entry)
			setPlayerChildren(entry, player);
	}

	redraw();
}

CAdventureMapIcon::CAdventureMapIcon(const Point & position, const AnimationPath & animation, size_t index, size_t iconsPerPlayer)
	: index(index)
	, iconsPerPlayer(iconsPerPlayer)
{
	OBJECT_CONSTRUCTION;
	pos += position;
	image = std::make_shared<CAnimImage>(animation, index);
}

void CAdventureMapIcon::setPlayerColor(const PlayerColor & player)
{
	image->setFrame(index + player.getNum() * iconsPerPlayer);
}

void CAdventureMapOverlayWidget::show(Canvas & to)
{
	CIntObject::showAll(to);
}

void AdventureMapWidget::updateActiveStateChildren(CIntObject * widget)
{
	for(auto & entry : widget->children)
	{
		auto container = dynamic_cast<CAdventureMapContainerWidget *>(entry);

		int mapLevels = GAME->interface()->cb->getMapHeader()->mapLevels;

		if (container)
		{
			if (container->disableCondition == "heroAwake")
				container->setEnabled(!shortcuts->optionHeroSleeping());

			if (container->disableCondition == "heroSleeping")
				container->setEnabled(shortcuts->optionHeroSleeping());

			if (container->disableCondition == "heroGround")
				container->setEnabled(!shortcuts->optionHeroBoat(EPathfindingLayer::SAIL) && !shortcuts->optionHeroBoat(EPathfindingLayer::AIR));

			if (container->disableCondition == "heroBoat")
				container->setEnabled(shortcuts->optionHeroBoat(EPathfindingLayer::SAIL));

			if (container->disableCondition == "heroAirship")
				container->setEnabled(shortcuts->optionHeroBoat(EPathfindingLayer::AIR));

			if (container->disableCondition == "mapLayerSurface")
				container->setEnabled(shortcuts->optionMapLevel() == 0);

			if (container->disableCondition == "mapLayerUnderground")
				container->setEnabled(shortcuts->optionMapLevel() == mapLevels - 1);

			if (container->disableCondition == "mapLayerOther")
				container->setEnabled(shortcuts->optionMapLevel() > 0 && shortcuts->optionMapLevel() < mapLevels - 1);

			if (container->disableCondition == "mapViewMode")
				container->setEnabled(shortcuts->optionInWorldView());

			if (container->disableCondition == "worldViewMode")
				container->setEnabled(!shortcuts->optionInWorldView());

			updateActiveStateChildren(container);
		}
	}
}

void AdventureMapWidget::updateActiveState()
{
	updateActiveStateChildren(this);

	for (auto entry: shortcuts->getShortcuts())
		setShortcutBlocked(entry.shortcut, !entry.isEnabled);
}
