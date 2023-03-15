/*
 * graphics.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
//code is copied from vcmiclient/Graphics.h with minimal changes

#include "../lib/GameConstants.h"
#include <QImage>

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGTownInstance;
class CGObjectInstance;
class EntityService;
class JsonNode;
class ObjectTemplate;

VCMI_LIB_NAMESPACE_END

class CHeroClass;
struct InfoAboutHero;
struct InfoAboutTown;
class Animation;

/// Handles fonts, hero images, town images, various graphics
class Graphics
{
	void addImageListEntry(size_t index, size_t group, const std::string & listName, const std::string & imageName);
	
	void addImageListEntries(const EntityService * service);
	
	void initializeBattleGraphics();
	void loadPaletteAndColors();
	
	void loadHeroAnimations();
	//loads animation and adds required rotated frames
	std::shared_ptr<Animation> loadHeroAnimation(const std::string &name);
	
	void loadHeroFlagAnimations();
	
	//loads animation and adds required rotated frames
	std::shared_ptr<Animation> loadHeroFlagAnimation(const std::string &name);
	
	void loadErmuToPicture();
	void loadFogOfWar();
	void loadFonts();
	void initializeImageLists();
	
public:
	//various graphics
	QVector<QRgb> playerColors; //array [8]
	QRgb neutralColor;
	QVector<QRgb> playerColorPalette; //palette to make interface colors good - array of size [256]
	QVector<QRgb> neutralColorPalette;
		
	// [hero class def name]  //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
	std::map< std::string, std::shared_ptr<Animation> > heroAnimations;
	std::vector< std::shared_ptr<Animation> > heroFlagAnimations;
	
	// [boat type: 0 .. 2]  //added group 10: up - left, 11 - left and 12 - left down // 13 - up-left standing; 14 - left standing; 15 - left down standing
	std::array< std::shared_ptr<Animation>, 3> boatAnimations;
	
	std::array< std::vector<std::shared_ptr<Animation> >, 3> boatFlagAnimations;
	
	//all other objects (not hero or boat)
	std::map< std::string, std::shared_ptr<Animation> > mapObjectAnimations;
		
	std::map<std::string, JsonNode> imageLists;
		
	//functions
	Graphics();
	~Graphics();
	
	void load();
	
	void blueToPlayersAdv(QImage * sur, PlayerColor player); //replaces blue interface colour with a color of player
	
	std::shared_ptr<Animation> getAnimation(const CGObjectInstance * obj);
	std::shared_ptr<Animation> getAnimation(const std::shared_ptr<const ObjectTemplate> info);
	std::shared_ptr<Animation> getHeroAnimation(const std::shared_ptr<const ObjectTemplate> info);
};

extern Graphics * graphics;
