#include "stdafx.h"
#include "CPreGame.h"
#include "SDL.h"
#include "CMessage.h"
extern SDL_Surface * ekran;
SDL_Rect genRect(int hh, int ww, int xx, int yy);
SDL_Color genRGB(int r, int g, int b, int a=0);
CPreGame * CPG;
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran);
bool isItIn(const SDL_Rect * rect, int x, int y)
{
	if ((x>rect->x && x<rect->x+rect->w) && (y>rect->y && y<rect->y+rect->h))
		return true;
	else return false;
}
CPreGame::CPreGame()
{
	preth = new CPreGameTextHandler;
	preth->loadTexts();
	currentMessage=NULL;
	behindCurMes=NULL;
	initMainMenu();
	showMainMenu();
	CPG=this;
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
	ok = slh->giveDef("IOKAY.DEF");
	cancel = slh->giveDef("ICANCEL.DEF");
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
void CPreGame::showCenBox (std::string data)
{
	CMessage * cmh = new CMessage();
	SDL_Surface * infoBox = cmh->genMessage(preth->getTitle(data), preth->getDescr(data));
	behindCurMes = SDL_CreateRGBSurface(ekran->flags,infoBox->w,infoBox->h,ekran->format->BitsPerPixel,ekran->format->Rmask,ekran->format->Gmask,ekran->format->Bmask,ekran->format->Amask);
	SDL_Rect pos = genRect(infoBox->h,infoBox->w,
		(ekran->w/2)-(infoBox->w/2),(ekran->h/2)-(infoBox->h/2));
	SDL_BlitSurface(ekran,&pos,behindCurMes,NULL);
	SDL_BlitSurface(infoBox,NULL,ekran,&pos);
	SDL_UpdateRect(ekran,pos.x,pos.y,pos.w,pos.h);
	SDL_FreeSurface(infoBox);
	currentMessage = new SDL_Rect(pos);
	delete cmh;
}
void CPreGame::showAskBox (std::string data, void(*f1)(),void(*f2)())
{
	CMessage * cmh = new CMessage();
	std::vector<CSemiDefHandler*> * przyciski = new std::vector<CSemiDefHandler*>(0);
	std::vector<SDL_Rect> * btnspos= new std::vector<SDL_Rect>(0);
	przyciski->push_back(ok);
	przyciski->push_back(cancel);
	SDL_Surface * infoBox = cmh->genMessage(preth->getTitle(data), preth->getDescr(data), EWindowType::yesOrNO, przyciski, btnspos);
	behindCurMes = SDL_CreateRGBSurface(ekran->flags,infoBox->w,infoBox->h,ekran->format->BitsPerPixel,ekran->format->Rmask,ekran->format->Gmask,ekran->format->Bmask,ekran->format->Amask);
	SDL_Rect pos = genRect(infoBox->h,infoBox->w,
		(ekran->w/2)-(infoBox->w/2),(ekran->h/2)-(infoBox->h/2));
	SDL_BlitSurface(ekran,&pos,behindCurMes,NULL);
	SDL_BlitSurface(infoBox,NULL,ekran,&pos);
	SDL_UpdateRect(ekran,pos.x,pos.y,pos.w,pos.h);
	SDL_FreeSurface(infoBox);
	currentMessage = new SDL_Rect(pos);
	(*btnspos)[0].x+=pos.x;
	(*btnspos)[0].y+=pos.y;
	(*btnspos)[1].x+=pos.x;
	(*btnspos)[1].y+=pos.y;
	btns.push_back(Button<>(1,(*btnspos)[0],&CPreGame::quit,ok));
	btns.push_back(Button<>(2,(*btnspos)[1],(&CPreGame::hideBox),cancel));
	delete cmh;
	delete przyciski;
	delete btnspos;
}
void CPreGame::hideBox ()
{
	SDL_BlitSurface(behindCurMes,NULL,ekran,currentMessage);
	SDL_UpdateRect
		(ekran,currentMessage->x,currentMessage->y,currentMessage->w,currentMessage->h);
	btns.clear();
	SDL_FreeSurface(behindCurMes);
	delete currentMessage;
	currentMessage = NULL;
	behindCurMes=NULL;
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
					if (currentMessage) continue;
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
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (int i=0;i<btns.size(); i++)
					{
						if (isItIn(&btns[i].pos,sEvent.motion.x,sEvent.motion.y))
						{
							SDL_BlitSurface((btns[i].imgs)->ourImages[1].bitmap,NULL,ekran,&btns[i].pos);
							updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
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
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (int i=0;i<btns.size(); i++)
					{
						if (isItIn(&btns[i].pos,sEvent.motion.x,sEvent.motion.y))
							(this->*(btns[i].fun))();
						else
						{
							SDL_BlitSurface((btns[i].imgs)->ourImages[0].bitmap,NULL,ekran,&btns[i].pos);
							updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
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
						showAskBox("\"{BBBB}BBDDDDDDDDDD\"",NULL,NULL);
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_RIGHT))
				{
					if (currentMessage) continue;
					if (isItIn(&ourMainMenu->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(preth->mainNewGame);
					}
					else if (isItIn(&ourMainMenu->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(preth->mainLoadGame);
					}
					else if (isItIn(&ourMainMenu->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(preth->mainHighScores);
					}
					else if (isItIn(&ourMainMenu->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(preth->mainCredits);
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(preth->mainQuit);
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_RIGHT) && currentMessage)
				{
					hideBox();
				}
				else if (sEvent.type==SDL_KEYDOWN)
				{
					switch (sEvent.key.keysym.sym)
					{
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