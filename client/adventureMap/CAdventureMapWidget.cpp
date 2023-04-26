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
#include "CAdventureMapWidget.h"

#include "CInfoBar.h"
#include "CList.h"
#include "CMinimap.h"
#include "CResDataBar.h"

#include "../gui/CGuiHandler.h"
#include "../mapView/MapView.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../CPlayerInterface.h"
#include "../PlayerLocalState.h"

#include "../../lib/StringConstants.h"
#include "../../lib/filesystem/ResourceID.h"

CAdventureMapWidget::CAdventureMapWidget()
	: state(EGameState::NOT_INITIALIZED)
{
	pos.x = pos.y = 0;
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	REGISTER_BUILDER("adventureInfobar",         &CAdventureMapWidget::buildInfobox         );
	REGISTER_BUILDER("adventureMapImage",        &CAdventureMapWidget::buildMapImage        );
	REGISTER_BUILDER("adventureMapButton",       &CAdventureMapWidget::buildMapButton       );
	REGISTER_BUILDER("adventureMapContainer",    &CAdventureMapWidget::buildMapContainer    );
	REGISTER_BUILDER("adventureMapGameArea",     &CAdventureMapWidget::buildMapGameArea     );
	REGISTER_BUILDER("adventureMapHeroList",     &CAdventureMapWidget::buildMapHeroList     );
	REGISTER_BUILDER("adventureMapIcon",         &CAdventureMapWidget::buildMapIcon         );
	REGISTER_BUILDER("adventureMapTownList",     &CAdventureMapWidget::buildMapTownList     );
	REGISTER_BUILDER("adventureMinimap",         &CAdventureMapWidget::buildMinimap         );
	REGISTER_BUILDER("adventureResourceDateBar", &CAdventureMapWidget::buildResourceDateBar );
	REGISTER_BUILDER("adventureStatusBar",       &CAdventureMapWidget::buildStatusBar       );

	const JsonNode config(ResourceID("config/widgets/adventureMap.json"));

	for(const auto & entry : config["options"]["imagesPlayerColored"].Vector())
	{
		ResourceID resourceName(entry.String(), EResType::IMAGE);
		playerColorerImages.push_back(resourceName.getName());
	}

	build(config);
}

Rect CAdventureMapWidget::readSourceArea(const JsonNode & source, const JsonNode & sourceCommon)
{
	const auto & input = source.isNull() ? sourceCommon : source;

	return readArea(input, Rect(Point(0, 0), Point(800, 600)));
}

Rect CAdventureMapWidget::readTargetArea(const JsonNode & source)
{
	if(subwidgetSizes.empty())
		return readArea(source, pos);
	return readArea(source, subwidgetSizes.back());
}

Rect CAdventureMapWidget::readArea(const JsonNode & source, const Rect & boundingBox)
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

std::shared_ptr<IImage> CAdventureMapWidget::loadImage(const std::string & name)
{
	ResourceID resource(name, EResType::IMAGE);

	if(images.count(resource.getName()) == 0)
		images[resource.getName()] = IImage::createFromFile(resource.getName());
	;

	return images[resource.getName()];
}

std::shared_ptr<CAnimation> CAdventureMapWidget::loadAnimation(const std::string & name)
{
	ResourceID resource(name, EResType::ANIMATION);

	if(animations.count(resource.getName()) == 0)
		animations[resource.getName()] = std::make_shared<CAnimation>(resource.getName());

	return animations[resource.getName()];
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildInfobox(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	infoBar = std::make_shared<CInfoBar>(area);
	return infoBar;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapImage(const JsonNode & input)
{
	Rect targetArea = readTargetArea(input["area"]);
	Rect sourceArea = readSourceArea(input["sourceArea"], input["area"]);
	std::string image = input["image"].String();

	return std::make_shared<CFilledTexture>(loadImage(image), targetArea, sourceArea);
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapButton(const JsonNode & input)
{
	auto position = readTargetArea(input["area"]);
	auto image = input["image"].String();
	auto help = readHintText(input["help"]);
	return std::make_shared<CButton>(position.topLeft(), image, help);
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapContainer(const JsonNode & input)
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

	result->moveBy(position.topLeft());
	subwidgetSizes.push_back(position);
	for(const auto & entry : input["items"].Vector())
	{
		result->ownedChildren.push_back(buildWidget(entry));
		result->addChild(result->ownedChildren.back().get(), false);
	}
	subwidgetSizes.pop_back();

	return result;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapGameArea(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	mapView = std::make_shared<MapView>(area.topLeft(), area.dimensions());
	return mapView;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapHeroList(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	subwidgetSizes.push_back(area);

	Rect item = readTargetArea(input["item"]);

	Point itemOffset(input["itemsOffset"]["x"].Integer(), input["itemsOffset"]["y"].Integer());
	int itemsCount = input["itemsCount"].Integer();

	auto result = std::make_shared<CHeroList>(itemsCount, item.topLeft(), itemOffset, LOCPLINT->localState->getWanderingHeroes().size());


	if(!input["scrollUp"].isNull())
		result->setScrollUpButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollUp"])));

	if(!input["scrollDown"].isNull())
		result->setScrollDownButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollDown"])));

	subwidgetSizes.pop_back();

	heroList = result;
	return result;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapIcon(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	size_t index = input["index"].Integer();
	size_t perPlayer = input["perPlayer"].Integer();
	std::string image = input["image"].String();

	return std::make_shared<CAdventureMapIcon>(area.topLeft(), loadAnimation(image), index, perPlayer);
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMapTownList(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	subwidgetSizes.push_back(area);

	Rect item = readTargetArea(input["item"]);
	Point itemOffset(input["itemsOffset"]["x"].Integer(), input["itemsOffset"]["y"].Integer());
	int itemsCount = input["itemsCount"].Integer();

	auto result = std::make_shared<CTownList>(itemsCount, item.topLeft(), itemOffset, LOCPLINT->localState->getOwnedTowns().size());

	if(!input["scrollUp"].isNull())
		result->setScrollUpButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollUp"])));

	if(!input["scrollDown"].isNull())
		result->setScrollDownButton(std::dynamic_pointer_cast<CButton>(buildMapButton(input["scrollDown"])));

	subwidgetSizes.pop_back();

	townList = result;
	return result;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildMinimap(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	minimap = std::make_shared<CMinimap>(area);
	return minimap;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildResourceDateBar(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	std::string image = input["image"].String();

	auto result = std::make_shared<CResDataBar>(image, area.topLeft());

	for(auto i = 0; i < GameConstants::RESOURCE_QUANTITY; i++)
	{
		const auto & node = input[GameConstants::RESOURCE_NAMES[i]];

		if(node.isNull())
			continue;

		result->setResourcePosition(GameResID(i), Point(node["x"].Integer(), node["y"].Integer()));
	}

	result->setDatePosition(Point(input["date"]["x"].Integer(), input["date"]["y"].Integer()));

	return result;
}

std::shared_ptr<CIntObject> CAdventureMapWidget::buildStatusBar(const JsonNode & input)
{
	Rect area = readTargetArea(input["area"]);
	std::string image = input["image"].String();

	auto background = std::make_shared<CFilledTexture>(image, area);

	return CGStatusBar::create(background);
}

std::shared_ptr<CHeroList> CAdventureMapWidget::getHeroList()
{
	return heroList;
}

std::shared_ptr<CTownList> CAdventureMapWidget::getTownList()
{
	return townList;
}

std::shared_ptr<CMinimap> CAdventureMapWidget::getMinimap()
{
	return minimap;
}

std::shared_ptr<MapView> CAdventureMapWidget::getMapView()
{
	return mapView;
}

std::shared_ptr<CInfoBar> CAdventureMapWidget::getInfoBar()
{
	return infoBar;
}

void CAdventureMapWidget::setPlayer(const PlayerColor & player)
{
	setPlayerChildren(this, player);
}

void CAdventureMapWidget::setPlayerChildren(CIntObject * widget, const PlayerColor & player)
{
	for(auto & entry : widget->children)
	{
		auto container = dynamic_cast<CAdventureMapContainerWidget *>(entry);
		auto icon = dynamic_cast<CAdventureMapIcon *>(entry);
		auto button = dynamic_cast<CButton *>(entry);

		if(button)
			button->setPlayerColor(player);

		if(icon)
			icon->setPlayer(player);

		if(container)
			setPlayerChildren(container, player);
	}

	for(const auto & entry : playerColorerImages)
	{
		if(images.count(entry))
			images[entry]->playerColored(player);
	}

	redraw();
}

void CAdventureMapWidget::setState(EGameState newState)
{
	state = newState;

	if(newState == EGameState::WORLD_VIEW)
		widget<CIntObject>("worldViewContainer")->enable();
	else
		widget<CIntObject>("worldViewContainer")->disable();
}

EGameState CAdventureMapWidget::getState()
{
	return state;
}

void CAdventureMapWidget::setOptionHasQuests(bool on)
{

}

void CAdventureMapWidget::setOptionHasUnderground(bool on)
{

}

void CAdventureMapWidget::setOptionUndergroundLevel(bool on)
{

}

void CAdventureMapWidget::setOptionHeroSleeping(bool on)
{

}

void CAdventureMapWidget::setOptionHeroSelected(bool on)
{

}

void CAdventureMapWidget::setOptionHeroCanMove(bool on)
{

}

void CAdventureMapWidget::setOptionHasNextHero(bool on)
{

}

CAdventureMapIcon::CAdventureMapIcon(const Point & position, std::shared_ptr<CAnimation> animation, size_t index, size_t iconsPerPlayer)
	: index(index)
	, iconsPerPlayer(iconsPerPlayer)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos += position;
	image = std::make_shared<CAnimImage>(animation, index);
}

void CAdventureMapIcon::setPlayer(const PlayerColor & player)
{
	image->setFrame(index + player.getNum() * iconsPerPlayer);
}

void CAdventureMapOverlayWidget::show(SDL_Surface * to)
{
	CIntObject::showAll(to);
}
