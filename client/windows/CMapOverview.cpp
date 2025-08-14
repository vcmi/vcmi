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

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"
#include "../render/CanvasImage.h"
#include "../render/IImage.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/StartInfo.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/serializer/CLoadFile.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"

CMapOverview::CMapOverview(const std::string & mapName, const std::string & fileName, const std::string & date, const std::string & author, const std::string & version, const ResourcePath & resource, ESelectionScreen tabType)
	: CWindowObject(BORDERED | RCLICK_POPUP), resource(resource), mapName(mapName), fileName(fileName), date(date), author(author), version(version), tabType(tabType)
{

	OBJECT_CONSTRUCTION;

	widget = std::make_shared<CMapOverviewWidget>(*this);

	updateShadow();

	center(ENGINE->getCursorPosition()); //center on mouse
#ifdef VCMI_MOBILE
	moveBy({0, -pos.h / 2});
#endif
	fitToScreen(10);
}

std::shared_ptr<CanvasImage> CMapOverviewWidget::createMinimapForLayer(std::unique_ptr<CMap> & map, int layer) const
{
	auto canvasImage = ENGINE->renderHandler().createImage(Point(map->width, map->height), CanvasScalingPolicy::IGNORE);
	auto canvas = canvasImage->getCanvas();

	for (int y = 0; y < map->height; ++y)
		for (int x = 0; x < map->width; ++x)
		{
			TerrainTile & tile = map->getTile(int3(x, y, layer));

			ColorRGBA color = tile.getTerrain()->minimapUnblocked;
			if (tile.blocked() && !tile.visitable())
				color = tile.getTerrain()->minimapBlocked;

			if(drawPlayerElements)
				// if object at tile is owned - it will be colored as its owner
				for (ObjectInstanceID objectID : tile.blockingObjects)
				{
					const auto * object = map->getObject(objectID);
					PlayerColor player = object->getOwner();
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
	
	return canvasImage;
}

std::vector<std::shared_ptr<CanvasImage>> CMapOverviewWidget::createMinimaps(const ResourcePath & resource) const
{
	std::vector<std::shared_ptr<CanvasImage>> ret;

	CMapService mapService;
	std::unique_ptr<CMap> map;
	try
	{
		auto cb = std::make_unique<EditorCallback>(map.get());
		map = mapService.loadMap(resource, cb.get());
	}
	catch (const std::exception & e)
	{
		logGlobal->warn("Failed to generate map preview! %s", e.what());
		return ret;
	}

	return createMinimaps(map);
}

std::vector<std::shared_ptr<CanvasImage>> CMapOverviewWidget::createMinimaps(std::unique_ptr<CMap> & map) const
{
	std::vector<std::shared_ptr<CanvasImage>> ret;

	for(int i = 0; i < map->levels(); i++)
		ret.push_back(createMinimapForLayer(map, i));

	return ret;
}

std::shared_ptr<CPicture> CMapOverviewWidget::buildDrawMinimap(const JsonNode & config) const
{
	// TODO: multilevel support
	logGlobal->debug("Building widget drawMinimap");

	auto rect = readRect(config["rect"]);
	auto id = config["id"].Integer();

	if(id >= minimaps.size())
		return nullptr;

	Point minimapRect = minimaps[id]->dimensions();
	double maxSideLengthSrc = std::max(minimapRect.x, minimapRect.y);
	double maxSideLengthDst = std::max(rect.w, rect.h);
	double resize = maxSideLengthSrc / maxSideLengthDst;
	Point newMinimapSize(minimapRect.x / resize, minimapRect.y / resize);

	minimaps[id]->scaleTo(newMinimapSize, EScalingAlgorithm::NEAREST); // for sharp-looking minimap

	return std::make_shared<CPicture>(minimaps[id], Point(rect.x, rect.y));
}

CMapOverviewWidget::CMapOverviewWidget(CMapOverview& parent):
	InterfaceObjectConfigurable(), p(parent)
{
	drawPlayerElements = p.tabType == ESelectionScreen::newGame;

	const JsonNode config(JsonPath::builtin("config/widgets/mapOverview.json"));

	if(settings["lobby"]["mapPreview"].Bool() && p.tabType != ESelectionScreen::campaignList)
	{
		ResourcePath res = ResourcePath(p.resource.getName(), EResType::MAP);
		std::unique_ptr<CMap> campaignMap = nullptr;
		if(p.tabType != ESelectionScreen::newGame && config["variables"]["mapPreviewForSaves"].Bool())
		{
			CLoadFile lf(*CResourceHandler::get()->getResourceName(ResourcePath(p.resource.getName(), EResType::SAVEGAME)), nullptr);
			CMapHeader mapHeader;
			StartInfo startInfo;
			lf.load(mapHeader);
			lf.load(startInfo);

			if(startInfo.campState)
				campaignMap = startInfo.campState->getMap(*startInfo.campState->currentScenario(), nullptr);
			res = ResourcePath(startInfo.fileURI, EResType::MAP);
		}
		if(!campaignMap)
			minimaps = createMinimaps(res);
		else
			minimaps = createMinimaps(campaignMap);
	}

	REGISTER_BUILDER("drawMinimap", &CMapOverviewWidget::buildDrawMinimap);

	build(config);

	if(auto w = widget<CFilledTexture>("background"))
	{
		p.pos = w->pos;
	}
	if(auto w = widget<CTextBox>("fileName"))
	{
		w->setText(p.fileName);
	}
	if(auto w = widget<CLabel>("mapName"))
	{
		w->setText(p.mapName);
	}
	if(auto w = widget<CLabel>("date"))
	{
		if(p.date.empty())
		{
			std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(ResourcePath(p.resource.getName(), p.tabType == ESelectionScreen::campaignList ? EResType::CAMPAIGN : EResType::MAP)));
			w->setText(TextOperations::getFormattedDateTimeLocal(time));
		}
		else
			w->setText(p.date);
	}
	if(auto w = widget<CLabel>("author"))
	{
		w->setText(p.author.empty() ? "-" : p.author);
	}
	if(auto w = widget<CLabel>("version"))
	{
		w->setText(p.version);
	}
	if(auto w = widget<CLabel>("noUnderground"))
	{
		if(minimaps.size() == 0)
			w->setText("");
	}
}
