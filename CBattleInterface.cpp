#include "CBattleInterface.h"
#include "CGameInfo.h"
#include "hch\CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"

CBattleInterface::CBattleInterface(CCreatureSet * army1, CCreatureSet * army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2)
{
	std::vector< std::string > & backref = CGI->mh->battleBacks[ CGI->mh->ttiles[tile.x][tile.y][tile.z].terType ];
	background = CGI->bitmaph->loadBitmap(backref[ rand() % backref.size()] );
	menu = CGI->bitmaph->loadBitmap("CBAR.BMP");
	CSDL_Ext::blueToPlayersAdv(menu, hero1->tempOwner);

	blitAt(background, 0, 0);
	blitAt(menu, 0, 556);
	CSDL_Ext::update();
	
	bOptions = new AdventureMapButton<CBattleInterface> (std::string(), std::string(), &CBattleInterface::bOptionsf, 3, 561, "icm003.def", this, false, NULL, false);
	//bOptions->activate();
}

CBattleInterface::~CBattleInterface()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(menu);
	//delete 
}

void CBattleInterface::activate()
{
	bOptions->activate();
}

void CBattleInterface::deactivate()
{
	bOptions->deactivate();
}

void CBattleInterface::show(SDL_Surface * to)
{
	blitAt(background, 0, 0, to);
	blitAt(menu, 0, 556, to);
	bOptions->show(to);
}

void CBattleInterface::bOptionsf()
{
}

void CBattleInterface::bSurrenderf()
{
}

void CBattleInterface::bFleef()
{
}

void CBattleInterface::bAutofightf()
{
}

void CBattleInterface::bSpellf()
{
}

void CBattleInterface::bWaitf()
{
}

void CBattleInterface::bDefencef()
{
}

void CBattleInterface::bConsoleUpf()
{
}

void CBattleInterface::bConsoleDownf()
{
}
