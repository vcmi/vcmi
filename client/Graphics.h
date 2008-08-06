#pragma once
#include "../global.h"
class CDefEssential;
struct SDL_Surface;
class CGHeroInstance;
class CGTownInstance;
class CDefHandler;
class CHeroClass;
struct SDL_Color;
class Graphics
{
public:
	//various graphics
	SDL_Color * playerColors; //array [8]
	SDL_Color * neutralColor;
	SDL_Color * playerColorPalette; //palette to make interface colors good - array of size [256]
	SDL_Surface * hInfo, *tInfo; //hero and town infobox bgs
	std::vector<std::pair<int, int> > slotsPos; //creature slot positions in infoboxes
	CDefEssential *luck22, *luck30, *luck42, *luck82,
		*morale22, *morale30, *morale42, *morale82,
		*halls, *forts, *bigTownPic;
	std::map<int,SDL_Surface*> heroWins; //hero_ID => infobox
	std::map<int,SDL_Surface*> townWins; //town_ID => infobox
	CDefHandler * artDefs; //artifacts
	std::vector<SDL_Surface *> portraitSmall; //48x32 px portraits of heroes
	std::vector<SDL_Surface *> portraitLarge; //58x64 px portraits of heroes
	std::vector<CDefHandler *> flags1, flags2, flags3, flags4; //flags blitted on heroes when ,
	CDefHandler * pskillsb, *resources; //82x93
	CDefHandler * un44; //many things
	CDefHandler * smallIcons, *resources32; //resources 32x32
	//creatures
	std::map<int,SDL_Surface*> smallImgs; //creature ID -> small 32x32 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::map<int,SDL_Surface*> bigImgs; //creature ID -> big 58x64 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::map<int,SDL_Surface*> backgrounds; //castle ID -> 100x130 background creature image // -1 is for neutral
	//for battles
	std::vector< std::vector< std::string > > battleBacks; //battleBacks[terType] - vector of possible names for certain terrain type
	std::vector< std::string > battleHeroes; //battleHeroes[hero type] - name of def that has hero animation for battle
	//functions
	Graphics();	
	void initializeBattleGraphics();
	void loadPaletteAndColors();
	void loadHeroFlags();
	void loadHeroFlags(std::pair<std::vector<CDefHandler *> Graphics::*, std::vector<const char *> > &pr, bool mode);
	void loadHeroAnim(std::vector<CDefHandler **> & anims);
	void loadHeroPortraits();
	SDL_Surface * drawHeroInfoWin(const CGHeroInstance * curh);
	SDL_Surface * drawPrimarySkill(const CGHeroInstance *curh, SDL_Surface *ret, int from=0, int to=PRIMARY_SKILLS);
	SDL_Surface * drawTownInfoWin(const CGTownInstance * curh);
	SDL_Surface * getPic(int ID, bool fort=true, bool builded=false); //returns small picture of town: ID=-1 - blank; -2 - border; -3 - random
	void blueToPlayersAdv(SDL_Surface * sur, int player); //replaces blue interface colour with a color of player
};

extern Graphics * graphics;