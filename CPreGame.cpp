#include "stdafx.h"
#include "CPreGame.h"
#include "SDL.h"
extern SDL_Surface * ekran;
bool isItIn(const SDL_Rect * rect, int x, int y)
{
	if ((x>rect->x && x<rect->x+rect->w) && (y>rect->y && y<rect->y+rect->h))
		return true;
	else return false;
}
CPreGame::CPreGame()
{
	initMainMenu();
	showMainMenu();
}
void CPreGame::initMainMenu()
{
	ourMainMenu = new menuItems();
	ourMainMenu->background = SDL_LoadBMP("h3bitmap.lod\\ZPIC1005.bmp");
	CSemiLodHandler * slh = new CSemiLodHandler();
	slh->openLod("H3sprite.lod");
	 //loading menu buttons
	ourMainMenu->newGame = slh->giveDef("ZMENUNG.DEF");
	ourMainMenu->loadGame = slh->giveDef("ZMENULG.DEF");
	ourMainMenu->highScores = slh->giveDef("ZMENUHS.DEF");
	ourMainMenu->credits = slh->giveDef("ZMENUCR.DEF");
	ourMainMenu->quit = slh->giveDef("ZMENUQT.DEF");
	// new game button location
	ourMainMenu->lNewGame.h=ourMainMenu->newGame->ourImages[0].bitmap->h;
	ourMainMenu->lNewGame.w=ourMainMenu->newGame->ourImages[0].bitmap->w;
	ourMainMenu->lNewGame.x=540;
	ourMainMenu->lNewGame.y=10;
	//load game location
	ourMainMenu->lLoadGame.h=ourMainMenu->loadGame->ourImages[0].bitmap->h;
	ourMainMenu->lLoadGame.w=ourMainMenu->loadGame->ourImages[0].bitmap->w;
	ourMainMenu->lLoadGame.x=532;
	ourMainMenu->lLoadGame.y=132;
	//high scores
	ourMainMenu->lHighScores.h=ourMainMenu->highScores->ourImages[0].bitmap->h;
	ourMainMenu->lHighScores.w=ourMainMenu->highScores->ourImages[0].bitmap->w;
	ourMainMenu->lHighScores.x=524;
	ourMainMenu->lHighScores.y=251;
	//credits
	ourMainMenu->lCredits.h=ourMainMenu->credits->ourImages[0].bitmap->h;
	ourMainMenu->lCredits.w=ourMainMenu->credits->ourImages[0].bitmap->w;
	ourMainMenu->lCredits.x=557;
	ourMainMenu->lCredits.y=359;
	//quit
	ourMainMenu->lQuit.h=ourMainMenu->quit->ourImages[0].bitmap->h;
	ourMainMenu->lQuit.w=ourMainMenu->quit->ourImages[0].bitmap->w;
	ourMainMenu->lQuit.x=586;
	ourMainMenu->lQuit.y=469;
	
	ourMainMenu->highlighted=0;
	handledLods.push_back(slh);
	delete slh;
}
void CPreGame::showMainMenu()
{
	SDL_BlitSurface(ourMainMenu->background,NULL,ekran,NULL);
	SDL_Flip(ekran);
	SDL_BlitSurface(ourMainMenu->newGame->ourImages[0].bitmap,NULL,ekran,&ourMainMenu->lNewGame);
	SDL_BlitSurface(ourMainMenu->loadGame->ourImages[0].bitmap,NULL,ekran,&ourMainMenu->lLoadGame);
	SDL_BlitSurface(ourMainMenu->highScores->ourImages[0].bitmap,NULL,ekran,&ourMainMenu->lHighScores);
	SDL_BlitSurface(ourMainMenu->credits->ourImages[0].bitmap,NULL,ekran,&ourMainMenu->lCredits);
	SDL_BlitSurface(ourMainMenu->quit->ourImages[0].bitmap,NULL,ekran,&ourMainMenu->lQuit);
	SDL_Flip(ekran);
}
void CPreGame::highlightButton(int which, int on)
{
	switch (which)
	{
	case 1:
		{
			SDL_BlitSurface(ourMainMenu->newGame->ourImages[on].bitmap,NULL,ekran,&ourMainMenu->lNewGame);
			break;
		}
	case 2:
		{
			SDL_BlitSurface(ourMainMenu->loadGame->ourImages[on].bitmap,NULL,ekran,&ourMainMenu->lLoadGame);
			break;
		}
	case 3:
		{
			SDL_BlitSurface(ourMainMenu->highScores->ourImages[on].bitmap,NULL,ekran,&ourMainMenu->lHighScores);
			break;
		}
	case 4:
		{
			SDL_BlitSurface(ourMainMenu->credits->ourImages[on].bitmap,NULL,ekran,&ourMainMenu->lCredits);
			break;
		}
	case 5:
		{
			SDL_BlitSurface(ourMainMenu->quit->ourImages[on].bitmap,NULL,ekran,&ourMainMenu->lQuit);
			break;
		}
	}
	SDL_Flip(ekran);
}
void CPreGame::runLoop()
{
	SDL_Event sEvent;
	while(true)
	{
		try
		{
			if(SDL_PollEvent(&sEvent))  //wait for event...
			{
				if(sEvent.type==SDL_QUIT) 
					return ;
				else if (sEvent.type==SDL_MOUSEMOTION)
				{
					if (ourMainMenu->highlighted)
					{
						switch (ourMainMenu->highlighted)
						{
							case 1:
								{
									if(isItIn(&ourMainMenu->lNewGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										ourMainMenu->highlighted=0;
										highlightButton(1,0);
									}
									break;
								}
							case 2:
								{
									if(isItIn(&ourMainMenu->lLoadGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										ourMainMenu->highlighted=0;
										highlightButton(2,0);
									}
									break;
								}
							case 3:
								{
									if(isItIn(&ourMainMenu->lHighScores,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										ourMainMenu->highlighted=0;
										highlightButton(3,0);
									}
									break;
								}
							case 4:
								{
									if(isItIn(&ourMainMenu->lCredits,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										ourMainMenu->highlighted=0;
										highlightButton(4,0);
									}
									break;
								}
							case 5:
								{
									if(isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										ourMainMenu->highlighted=0;
										highlightButton(5,0);
									}
									break;
								}
						} //switch (ourMainMenu->highlighted)
					} // if (ourMainMenu->highlighted)
					if (isItIn(&ourMainMenu->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						ourMainMenu->highlighted=1;
					}
					else if (isItIn(&ourMainMenu->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						ourMainMenu->highlighted=2;
					}
					else if (isItIn(&ourMainMenu->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						ourMainMenu->highlighted=3;
					}
					else if (isItIn(&ourMainMenu->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						ourMainMenu->highlighted=4;
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						ourMainMenu->highlighted=5;
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && SDL_BUTTON(1))
				{
					if (isItIn(&ourMainMenu->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,1);
						ourMainMenu->highlighted=1;
					}
					else if (isItIn(&ourMainMenu->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,1);
						ourMainMenu->highlighted=2;
					}
					else if (isItIn(&ourMainMenu->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,1);
						ourMainMenu->highlighted=3;
					}
					else if (isItIn(&ourMainMenu->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,1);
						ourMainMenu->highlighted=4;
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,1);
						ourMainMenu->highlighted=5;
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && SDL_BUTTON(1))
				{
					if (isItIn(&ourMainMenu->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						ourMainMenu->highlighted=1;
					}
					else if (isItIn(&ourMainMenu->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						ourMainMenu->highlighted=2;
					}
					else if (isItIn(&ourMainMenu->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						ourMainMenu->highlighted=3;
					}
					else if (isItIn(&ourMainMenu->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						ourMainMenu->highlighted=4;
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						ourMainMenu->highlighted=5;
					}
				}
				else if (sEvent.type==SDL_KEYDOWN)
				{
					switch (sEvent.key.keysym.sym)
					{
					//case SDLK_LEFT:
					//	{
					//		if(ourMainMenu->lQuit.x>0)
					//			ourMainMenu->lQuit.x--;
					//		break;
					//	}
					//case (SDLK_RIGHT):
					//	{
					//			ourMainMenu->lQuit.x++;
					//		break;
					//	}
					//case (SDLK_UP):
					//	{
					//		if(ourMainMenu->lQuit.y>0)
					//			ourMainMenu->lQuit.y--;
					//		break;
					//	}
					//case (SDLK_DOWN):
					//	{
					//			ourMainMenu->lQuit.y++;
					//		break;
					//	}
					case (SDLK_q):
						{
							return ;
							break;
						}
					}
					showMainMenu();
				}
			}
		}
		catch(...)
		{ continue; }

		SDL_Delay(30); //give time for other apps
	}

}