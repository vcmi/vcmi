/*
 * CAdventureMapWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/InterfaceObjectConfigurable.h"

class CHeroList;
class CTownList;
class CMinimap;
class MapView;
class CInfoBar;
class IImage;
class AdventureMapShortcuts;
enum class EAdventureState;

/// Internal class of AdventureMapInterface that contains actual UI elements
class CAdventureMapWidget : public InterfaceObjectConfigurable
{
	int mapLevel;
	/// temporary stack of sizes of currently building widgets
	std::vector<Rect> subwidgetSizes;

	/// list of images on which player-colored palette will be applied
	std::vector<std::string> playerColorerImages;

	/// list of named images shared between widgets
	std::map<std::string, std::shared_ptr<IImage>> images;
	std::map<std::string, std::shared_ptr<CAnimation>> animations;

	/// Widgets that require access from adventure map
	std::shared_ptr<CHeroList> heroList;
	std::shared_ptr<CTownList> townList;
	std::shared_ptr<CMinimap> minimap;
	std::shared_ptr<MapView> mapView;
	std::shared_ptr<CInfoBar> infoBar;

	std::shared_ptr<AdventureMapShortcuts> shortcuts;

	Rect readTargetArea(const JsonNode & source);
	Rect readSourceArea(const JsonNode & source, const JsonNode & sourceCommon);
	Rect readArea(const JsonNode & source, const Rect & boundingBox);

	std::shared_ptr<IImage> loadImage(const std::string & name);
	std::shared_ptr<CAnimation> loadAnimation(const std::string & name);

	std::shared_ptr<CIntObject> buildInfobox(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapImage(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapButton(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapContainer(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapGameArea(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapHeroList(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapIcon(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMapTownList(const JsonNode & input);
	std::shared_ptr<CIntObject> buildMinimap(const JsonNode & input);
	std::shared_ptr<CIntObject> buildResourceDateBar(const JsonNode & input);
	std::shared_ptr<CIntObject> buildStatusBar(const JsonNode & input);

	void setPlayerChildren(CIntObject * widget, const PlayerColor & player);
	void updateActiveStateChildden(CIntObject * widget);
public:
	explicit CAdventureMapWidget( std::shared_ptr<AdventureMapShortcuts> shortcuts );

	std::shared_ptr<CHeroList> getHeroList();
	std::shared_ptr<CTownList> getTownList();
	std::shared_ptr<CMinimap> getMinimap();
	std::shared_ptr<MapView> getMapView();
	std::shared_ptr<CInfoBar> getInfoBar();

	void setPlayer(const PlayerColor & player);

	void onMapViewMoved(const Rect & visibleArea, int mapLevel);
	void updateActiveState();
};

/// Small helper class that provides ownership for shared_ptr's of child elements
class CAdventureMapContainerWidget : public CIntObject
{
	friend class CAdventureMapWidget;
	std::vector<std::shared_ptr<CIntObject>> ownedChildren;
	std::string disableCondition;
};

class CAdventureMapOverlayWidget : public CAdventureMapContainerWidget
{
public:
	void show(SDL_Surface * to) override;
};

/// Small helper class that provides player-colorable icon using animation file
class CAdventureMapIcon : public CIntObject
{
	std::shared_ptr<CAnimImage> image;

	size_t index;
	size_t iconsPerPlayer;
public:
	CAdventureMapIcon(const Point & position, std::shared_ptr<CAnimation> image, size_t index, size_t iconsPerPlayer);

	void setPlayer(const PlayerColor & player);
};
