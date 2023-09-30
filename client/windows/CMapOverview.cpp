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

#include <vstd/DateUtils.h>

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
#include "../../lib/filesystem/Filesystem.h"

CMapOverview::CMapOverview(std::string mapName, std::string fileName, std::string date, ResourcePath resource, ESelectionScreen tabType)
	: CWindowObject(BORDERED | RCLICK_POPUP), resource(resource), mapName(mapName), fileName(fileName), date(date), tabType(tabType)
{

	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	const JsonNode config(JsonPath::builtin("config/widgets/mapOverview.json"));
	pos = Rect(0, 0, config["items"][0]["rect"]["w"].Integer(), config["items"][0]["rect"]["h"].Integer());

	widget = std::make_shared<CMapOverviewWidget>(*this);

	updateShadow();

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

	if(!renderImage)
		return nullptr;

	const std::vector<std::shared_ptr<IImage>> images = createMinimaps(ResourcePath(parent.resource.getName(), EResType::MAP), Point(rect.w, rect.h));

	if(id >= images.size())
		return nullptr;

	return std::make_shared<CPicture>(images[id], Point(rect.x, rect.y));
}

std::shared_ptr<CTextBox> CMapOverview::CMapOverviewWidget::buildDrawPath(const JsonNode & config) const
{
	logGlobal->debug("Building widget drawPath");

	auto rect = readRect(config["rect"]);
	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);

	return std::make_shared<CTextBox>(parent.fileName, rect, 0, font, alignment, color);
}

std::shared_ptr<CLabel> CMapOverview::CMapOverviewWidget::buildDrawString(const JsonNode & config) const
{
	logGlobal->debug("Building widget drawString");

	auto font = readFont(config["font"]);
	auto alignment = readTextAlignment(config["alignment"]);
	auto color = readColor(config["color"]);
	std::string text = "";
	if("mapname" == config["text"].String())
		text = parent.mapName;
	if("date" == config["text"].String())
	{
		if(parent.date.empty())
		{
			std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(ResourcePath(parent.resource.getName(), EResType::MAP)));
			text = vstd::getFormattedDateTime(time);
		}
		else
			text = parent.date;
	}
	auto position = readPosition(config["position"]);
	return std::make_shared<CLabel>(position.x, position.y, font, alignment, color, text);
}

CMapOverview::CMapOverviewWidget::CMapOverviewWidget(CMapOverview& parent):
	InterfaceObjectConfigurable(), parent(parent)
{
	drawPlayerElements = parent.tabType == ESelectionScreen::newGame;
	renderImage = parent.tabType == ESelectionScreen::newGame && settings["lobby"]["mapPreview"].Bool();

	const JsonNode config(JsonPath::builtin("config/widgets/mapOverview.json"));

	REGISTER_BUILDER("drawTransparentRect", &CMapOverview::CMapOverviewWidget::buildDrawTransparentRect);
	REGISTER_BUILDER("drawMinimap", &CMapOverview::CMapOverviewWidget::buildDrawMinimap);
	REGISTER_BUILDER("drawPath", &CMapOverview::CMapOverviewWidget::buildDrawPath);
	REGISTER_BUILDER("drawString", &CMapOverview::CMapOverviewWidget::buildDrawString);

	drawPlayerElements = parent.tabType == ESelectionScreen::newGame;
	renderImage = parent.tabType == ESelectionScreen::newGame && settings["lobby"]["mapPreview"].Bool();

	build(config);
}