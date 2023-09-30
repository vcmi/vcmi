/*
 * CMapOverview.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CMapOverview.h"

#include "../lobby/SelectionTab.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/TerrainHandler.h"

CMapOverview::CMapOverview(std::string text, ResourcePath resource, ESelectionScreen tabType)
	: CWindowObject(BORDERED | RCLICK_POPUP)
{

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	pos = Rect(0, 0, 400, 300);

	widget = std::make_shared<CMapOverviewWidget>(text, resource, tabType);

	updateShadow();

	/*
	std::vector<std::shared_ptr<IImage>> mapLayerImages;
	if(renderImage)
		mapLayerImages = createMinimaps(ResourcePath(resource.getName(), EResType::MAP), IMAGE_SIZE);

	if(mapLayerImages.size() == 0)
		renderImage = false;

	pos = Rect(0, 0, 3 * BORDER + 2 * IMAGE_SIZE, 2000);

	auto drawLabel = [&]() {
		label = std::make_shared<CTextBox>(text, Rect(BORDER, BORDER, BORDER + 2 * IMAGE_SIZE, 350), 0, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE);
		if(!label->slider)
			label->resize(Point(BORDER + 2 * IMAGE_SIZE, label->label->textSize.y));
	};
	drawLabel();

	int textHeight = std::min(350, label->label->textSize.y);
	pos.h = BORDER + textHeight + BORDER;
	if(renderImage)
		pos.h += IMAGE_SIZE + BORDER;
	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
	updateShadow();

	drawLabel();

	if(renderImage)
	{
		if(mapLayerImages.size() == 1)
			image1 = std::make_shared<CPicture>(mapLayerImages[0], Point(BORDER + (BORDER + IMAGE_SIZE) / 2, textHeight + 2 * BORDER));
		else
		{
			image1 = std::make_shared<CPicture>(mapLayerImages[0], Point(BORDER, textHeight + 2 * BORDER));
			image2 = std::make_shared<CPicture>(mapLayerImages[1], Point(BORDER + IMAGE_SIZE + BORDER, textHeight + 2 * BORDER));
		}
	}
	*/

	center(GH.getCursorPosition()); //center on mouse
#ifdef VCMI_MOBILE
	moveBy({0, -pos.h / 2});
#endif
	fitToScreen(10);
}

Canvas CMapOverview::CMapOverviewWidget::createMinimapForLayer(std::unique_ptr<CMap> & map, int layer) const
{
	Canvas canvas = Canvas(Point(map->width, map->height));

	for (int y = 0; y < map->height; ++y)
		for (int x = 0; x < map->width; ++x)
		{
			TerrainTile & tile = map->getTile(int3(x, y, layer));

			ColorRGBA color = tile.terType->minimapUnblocked;
			if (tile.blocked && (!tile.visitable))
				color = tile.terType->minimapBlocked;

			if(drawPlayerElements)
				// if object at tile is owned - it will be colored as its owner
				for (const CGObjectInstance *obj : tile.blockingObjects)
				{
					PlayerColor player = obj->getOwner();
					if(player == PlayerColor::NEUTRAL)
					{
						color = graphics->neutralColor;
						break;
					}
					if (player.isValidPlayer())
					{
						color = graphics->playerColors[player.getNum()];
						break;
					}
				}

			canvas.drawPoint(Point(x, y), color);
		}
	
	return canvas;
}

std::vector<std::shared_ptr<IImage>> CMapOverview::CMapOverviewWidget::createMinimaps(ResourcePath resource, Point size) const
{
	std::vector<std::shared_ptr<IImage>> ret = std::vector<std::shared_ptr<IImage>>();

	CMapService mapService;
	std::unique_ptr<CMap> map;
	try
	{
		map = mapService.loadMap(resource);
	}
	catch (...)
	{
		return ret;
	}

	for(int i = 0; i < (map->twoLevel ? 2 : 1); i++)
	{
		Canvas canvas = createMinimapForLayer(map, i);
		Canvas canvasScaled = Canvas(size);
		canvasScaled.drawScaled(canvas, Point(0, 0), size);
		std::shared_ptr<IImage> img = GH.renderHandler().createImage(canvasScaled.getInternalSurface());
		
		ret.push_back(img);
	}

	return ret;
}

std::shared_ptr<TransparentFilledRectangle> CMapOverview::CMapOverviewWidget::CMapOverviewWidget::buildDrawTransparentRect(const JsonNode & config) const
{
	logGlobal->debug("Building widget drawTransparentRect");

	auto rect = readRect(config["rect"]);
	auto color = readColor(config["color"]);
	if(!config["colorLine"].isNull())
	{
		auto colorLine = readColor(config["colorLine"]);
		return std::make_shared<TransparentFilledRectangle>(rect, color, colorLine);
	}
	return std::make_shared<TransparentFilledRectangle>(rect, color);
}

std::shared_ptr<CPicture> CMapOverview::CMapOverviewWidget::buildDrawMinimap(const JsonNode & config) const
{
	logGlobal->debug("Building widget drawMinimap");

	auto rect = readRect(config["rect"]);
	auto id = config["id"].Integer();
	const std::vector<std::shared_ptr<IImage>> images = createMinimaps(ResourcePath(resource.getName(), EResType::MAP), Point(rect.w, rect.h));

	return std::make_shared<CPicture>(images[id], Point(rect.x, rect.y));
}

CMapOverview::CMapOverviewWidget::CMapOverviewWidget(std::string text, ResourcePath resource, ESelectionScreen tabType):
	InterfaceObjectConfigurable(), resource(resource)
{
	drawPlayerElements = tabType == ESelectionScreen::newGame;
	renderImage = tabType == ESelectionScreen::newGame && settings["lobby"]["mapPreview"].Bool();

	const JsonNode config(JsonPath::builtin("config/widgets/mapOverview.json"));

	REGISTER_BUILDER("drawTransparentRect", &CMapOverview::CMapOverviewWidget::buildDrawTransparentRect);
	REGISTER_BUILDER("drawMinimap", &CMapOverview::CMapOverviewWidget::buildDrawMinimap);

	build(config);
}