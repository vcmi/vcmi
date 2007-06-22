#include "stdafx.h"
#include "CPreGame.h"
#include "SDL.h"
extern SDL_Surface * ekran;
extern SDL_Color tytulowy, tlo, zwykly ;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Rect genRect(int hh, int ww, int xx, int yy);
SDL_Color genRGB(int r, int g, int b, int a=0);
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran);
void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran);
CPreGame * CPG;
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran);
bool isItIn(const SDL_Rect * rect, int x, int y);

template <class T> void Button<T>::press(bool down)
{
	int i;
	if (down) state=i=1;
	else state=i=0;
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,ekran,&pos);
	updateRect(&pos);
}
template <class T> void Button<T>::hover(bool on=true)
{
	if (!highlightable) return;
	int i;
	if (on) state=i=2;
	else state=i=0;
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,ekran,&pos);
	updateRect(&pos);
}
template <class T> void Button<T>::select(bool on)
{
	int i;
	if (on) state=i=3;
	else state=i=0;
	SDL_BlitSurface(imgs->ourImages[i].bitmap,NULL,ekran,&pos);
	updateRect(&pos);
	if (ourGroup && on && ourGroup->type==1)
	{
		if (ourGroup->selected && ourGroup->selected!=this)
			ourGroup->selected->select(false);
		ourGroup->selected =this;
	}
}
CPreGame::CPreGame()
{

	tytulowy.r=229;tytulowy.g=215;tytulowy.b=123;tytulowy.unused=0;
	zwykly.r=255;zwykly.g=255;zwykly.b=255;zwykly.unused=0; //gbr
	tlo.r=66;tlo.g=44;tlo.b=24;tlo.unused=0;
	preth = new CPreGameTextHandler;
	preth->loadTexts();
	currentMessage=NULL;
	behindCurMes=NULL;
	initMainMenu();
	initNewMenu();
	initScenSel();
	showMainMenu();
	CPG=this;
}
void CPreGame::initScenSel()
{
	ourScenSel = new ScenSel();
	if (rand()%2) ourScenSel->background=SDL_LoadBMP("h3bitmap.lod\\ZPIC1000.bmp");
	else ourScenSel->background=SDL_LoadBMP("h3bitmap.lod\\ZPIC1001.bmp");

	ourScenSel->pressed=NULL;

	ourScenSel->scenInf=SDL_LoadBMP("h3bitmap.lod\\GSELPOP1.bmp");
	ourScenSel->scenList=SDL_LoadBMP("h3bitmap.lod\\SCSELBCK.bmp");
	ourScenSel->randMap=SDL_LoadBMP("h3bitmap.lod\\RANMAPBK.bmp");
	ourScenSel->options=SDL_LoadBMP("h3bitmap.lod\\ADVOPTBK.bmp");
	SDL_SetColorKey(ourScenSel->scenInf,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->scenInf->format,0,255,255));
	SDL_SetColorKey(ourScenSel->scenList,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->scenList->format,0,255,255));
	SDL_SetColorKey(ourScenSel->randMap,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->randMap->format,0,255,255));
	SDL_SetColorKey(ourScenSel->options,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->options->format,0,255,255));

	ourScenSel->difficulty = new CPoinGroup<>();
	ourScenSel->difficulty->type=1;
	ourScenSel->selectedDiff=-77;
	ourScenSel->difficulty->gdzie = &ourScenSel->selectedDiff;
	ourScenSel->bEasy = IntSelBut<>(genRect(0,0,506,456),NULL,slh->giveDef("GSPBUT3.DEF"),true,ourScenSel->difficulty,1);
	ourScenSel->bNormal = IntSelBut<>(genRect(0,0,538,456),NULL,slh->giveDef("GSPBUT4.DEF"),true,ourScenSel->difficulty,2);
	ourScenSel->bHard = IntSelBut<>(genRect(0,0,570,456),NULL,slh->giveDef("GSPBUT5.DEF"),true,ourScenSel->difficulty,3);
	ourScenSel->bExpert = IntSelBut<>(genRect(0,0,602,456),NULL,slh->giveDef("GSPBUT6.DEF"),true,ourScenSel->difficulty,4);
	ourScenSel->bImpossible = IntSelBut<>(genRect(0,0,634,456),NULL,slh->giveDef("GSPBUT7.DEF"),true,ourScenSel->difficulty,5);

	ourScenSel->bBack = Button<>(genRect(0,0,584,535),&CPreGame::showNewMenu,slh->giveDef("SCNRBACK.DEF"));
	ourScenSel->bBegin = Button<>(genRect(0,0,414,535),&CPreGame::showScenList,slh->giveDef("SCNRBEG.DEF"));

	ourScenSel->bScens = Button<>(genRect(0,0,414,81),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bScens.imgs->ourImages.size(); i++)
		printAt("Show Available Scenarios",25+i,2+i,GEOR13,zwykly,ourScenSel->bScens.imgs->ourImages[i].bitmap);
	ourScenSel->bRandom = Button<>(genRect(0,0,414,105),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bRandom.imgs->ourImages.size(); i++)
		printAt("Random Map",25+i,2+i,GEOR13,zwykly,ourScenSel->bRandom.imgs->ourImages[i].bitmap);
	ourScenSel->bOptions = Button<>(genRect(0,0,414,509),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bOptions.imgs->ourImages.size(); i++)
		printAt("Show Advanced Options",25+i,2+i,GEOR13,zwykly,ourScenSel->bOptions.imgs->ourImages[i].bitmap);


}

void CPreGame::showScenSel()
{
	state=EState::ScenarioList;
	SDL_BlitSurface(ourScenSel->background,NULL,ekran,NULL);
	SDL_BlitSurface(ourScenSel->scenInf,NULL,ekran,&genRect(ourScenSel->scenInf->h,ourScenSel->scenInf->w,395,6));
	//blit texts
	printAt(preth->singleScenarioName,420,25,GEOR13);
	printAt("Scenario Description:",420,135,GEOR13);
	printAt("Victory Condition:",420,285,GEOR13);
	printAt("Loss Condition:",420,340,GEOR13);
	printAt("Allies:",420,406,GEOR13,zwykly);
	printAt("Enemies:",585,406,GEOR13,zwykly);
	printAt("Map Diff:",427,438,GEOR13);
	printAt("Player Difficulty:",527,438,GEOR13);
	printAt("Rating:",685,438,GEOR13);
	//blit buttons
	SDL_BlitSurface(ourScenSel->bEasy.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bEasy.pos);
	SDL_BlitSurface(ourScenSel->bNormal.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bNormal.pos);
	SDL_BlitSurface(ourScenSel->bHard.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bHard.pos);
	SDL_BlitSurface(ourScenSel->bExpert.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bExpert.pos);
	SDL_BlitSurface(ourScenSel->bImpossible.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bImpossible.pos);
	SDL_BlitSurface(ourScenSel->bScens.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bScens.pos);
	SDL_BlitSurface(ourScenSel->bOptions.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bOptions.pos);
	SDL_BlitSurface(ourScenSel->bBegin.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bBegin.pos);
	SDL_BlitSurface(ourScenSel->bBack.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bBack.pos);
	SDL_BlitSurface(ourScenSel->bRandom.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bRandom.pos);
	//blitAt(ourScenSel->bScens.imgs->ourImages[0].bitmap,ourScenSel->bScens.pos.x,ourScenSel->bScens.pos.y);
	//blitAt(ourScenSel->bRandom.imgs->ourImages[0].bitmap,414,105);
	//blitAt(ourScenSel->bOptions.imgs->ourImages[0].bitmap,414,509);
	//blitAt(ourScenSel->bBegin.imgs->ourImages[0].bitmap,414,535);
	//blitAt(ourScenSel->bBack.imgs->ourImages[0].bitmap,584,535);
	//add buttons info
	btns.push_back(&ourScenSel->bEasy);
	btns.push_back(&ourScenSel->bNormal);
	btns.push_back(&ourScenSel->bHard);
	btns.push_back(&ourScenSel->bExpert);
	btns.push_back(&ourScenSel->bImpossible);
	btns.push_back(&ourScenSel->bScens);
	btns.push_back(&ourScenSel->bRandom);
	btns.push_back(&ourScenSel->bOptions);
	btns.push_back(&ourScenSel->bBegin);
	btns.push_back(&ourScenSel->bBack);

	ourScenSel->selectedDiff=1;
	ourScenSel->bNormal.select(true);

	for (int i=0;i<btns.size();i++)
	{
		btns[i]->pos.w=btns[i]->imgs->ourImages[0].bitmap->w;
		btns[i]->pos.h=btns[i]->imgs->ourImages[0].bitmap->h;
	}

	handleOther = &CPreGame::scenHandleEv;
	SDL_Flip(ekran);

}
void CPreGame::showScenList()
{}
void CPreGame::showOptions()
{}
void CPreGame::initNewMenu()
{
	ourNewMenu = new menuItems();
	ourNewMenu->background = SDL_LoadBMP("h3bitmap.lod\\ZPIC1005.bmp");
	slh = new CSemiLodHandler();
	slh->openLod("H3sprite.lod");
	 //loading menu buttons
	ourNewMenu->newGame = slh->giveDef("ZTSINGL.DEF");
	ourNewMenu->loadGame = slh->giveDef("ZTMULTI.DEF");
	ourNewMenu->highScores = slh->giveDef("ZTCAMPN.DEF");
	ourNewMenu->credits = slh->giveDef("ZTTUTOR.DEF");
	ourNewMenu->quit = slh->giveDef("ZTBACK.DEF");
	ok = slh->giveDef("IOKAY.DEF");
	cancel = slh->giveDef("ICANCEL.DEF");
	// single scenario
	ourNewMenu->lNewGame.h=ourNewMenu->newGame->ourImages[0].bitmap->h;
	ourNewMenu->lNewGame.w=ourNewMenu->newGame->ourImages[0].bitmap->w;
	ourNewMenu->lNewGame.x=545;
	ourNewMenu->lNewGame.y=4;
	ourNewMenu->fNewGame=&CPreGame::showScenSel;
	//multiplayer
	ourNewMenu->lLoadGame.h=ourNewMenu->loadGame->ourImages[0].bitmap->h;
	ourNewMenu->lLoadGame.w=ourNewMenu->loadGame->ourImages[0].bitmap->w;
	ourNewMenu->lLoadGame.x=568;
	ourNewMenu->lLoadGame.y=120;
	//campaign
	ourNewMenu->lHighScores.h=ourNewMenu->highScores->ourImages[0].bitmap->h;
	ourNewMenu->lHighScores.w=ourNewMenu->highScores->ourImages[0].bitmap->w;
	ourNewMenu->lHighScores.x=541;
	ourNewMenu->lHighScores.y=233;
	//tutorial
	ourNewMenu->lCredits.h=ourNewMenu->credits->ourImages[0].bitmap->h;
	ourNewMenu->lCredits.w=ourNewMenu->credits->ourImages[0].bitmap->w;
	ourNewMenu->lCredits.x=545;
	ourNewMenu->lCredits.y=358;
	//back
	ourNewMenu->lQuit.h=ourNewMenu->quit->ourImages[0].bitmap->h;
	ourNewMenu->lQuit.w=ourNewMenu->quit->ourImages[0].bitmap->w;
	ourNewMenu->lQuit.x=582;
	ourNewMenu->lQuit.y=464;
	ourNewMenu->fQuit=&CPreGame::showMainMenu;
	
	ourNewMenu->highlighted=0;
}
void CPreGame::showNewMenu()
{
	btns.clear();
	state = EState::newGame;
	SDL_BlitSurface(ourNewMenu->background,NULL,ekran,NULL);
	SDL_BlitSurface(ourNewMenu->newGame->ourImages[0].bitmap,NULL,ekran,&ourNewMenu->lNewGame);
	SDL_BlitSurface(ourNewMenu->loadGame->ourImages[0].bitmap,NULL,ekran,&ourNewMenu->lLoadGame);
	SDL_BlitSurface(ourNewMenu->highScores->ourImages[0].bitmap,NULL,ekran,&ourNewMenu->lHighScores);
	SDL_BlitSurface(ourNewMenu->credits->ourImages[0].bitmap,NULL,ekran,&ourNewMenu->lCredits);
	SDL_BlitSurface(ourNewMenu->quit->ourImages[0].bitmap,NULL,ekran,&ourNewMenu->lQuit);
	SDL_Flip(ekran);
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
	ourMainMenu->fNewGame=&CPreGame::showNewMenu;
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
	ourMainMenu->lQuit.y=468;
	ourMainMenu->fQuit=&CPreGame::quitAskBox;
	
	ourMainMenu->highlighted=0;
	handledLods.push_back(slh);
	delete slh;
}
void CPreGame::showMainMenu()
{
	state = EState::mainMenu;
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

	menuItems * current = currentItems();
	switch (which)
	{
	case 1:
		{
			SDL_BlitSurface(current->newGame->ourImages[on].bitmap,NULL,ekran,&current->lNewGame);
			break;
		}
	case 2:
		{
			SDL_BlitSurface(current->loadGame->ourImages[on].bitmap,NULL,ekran,&current->lLoadGame);
			break;
		}
	case 3:
		{
			SDL_BlitSurface(current->highScores->ourImages[on].bitmap,NULL,ekran,&current->lHighScores);
			break;
		}
	case 4:
		{
			SDL_BlitSurface(current->credits->ourImages[on].bitmap,NULL,ekran,&current->lCredits);
			break;
		}
	case 5:
		{
			SDL_BlitSurface(current->quit->ourImages[on].bitmap,NULL,ekran,&current->lQuit);
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
	btns.push_back(new Button<>((*btnspos)[0],&CPreGame::quit,ok,false, NULL,2));
	btns.push_back(new Button<>((*btnspos)[1],(&CPreGame::hideBox),cancel,false, NULL,2));
	delete cmh;
	delete przyciski;
	delete btnspos;

}
void CPreGame::hideBox ()
{
	SDL_BlitSurface(behindCurMes,NULL,ekran,currentMessage);
	SDL_UpdateRect
		(ekran,currentMessage->x,currentMessage->y,currentMessage->w,currentMessage->h);
	for (int i=0;i<btns.size();i++)
	{
		if (btns[i]->ID==2)
		{
			delete btns[i];
			btns.erase(btns.begin()+i);
			i--;
		}
	}
	SDL_FreeSurface(behindCurMes);
	delete currentMessage;
	currentMessage = NULL;
	behindCurMes=NULL;
}
CPreGame::menuItems * CPreGame::currentItems()
{
	switch (state)
	{
	case EState::mainMenu:
		return ourMainMenu;
	case EState::newGame:
		return ourNewMenu;
	default:
		return NULL;
	}
}

void CPreGame::scenHandleEv(SDL_Event& sEvent)
{
	if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		for (int i=0;i<btns.size(); i++)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				
				mush->playClick();
				btns[i]->press(true);
				ourScenSel->pressed=btns[i];
			}
		}
	}
	else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		if (ourScenSel->pressed)
		{
			ourScenSel->pressed->press(false);
			ourScenSel->pressed=NULL;
		}
		for (int i=0;i<btns.size(); i++)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				if (btns[i]->selectable)
					btns[i]->select(true);
				if (btns[i]->fun)
					(this->*(btns[i]->fun))();
				return;
			}
		}
	}
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
				menuItems * current = currentItems();
				if(sEvent.type==SDL_QUIT) 
					return ;
				if (!current)
				{
					(this->*handleOther)(sEvent);
				}
				else if (sEvent.type==SDL_KEYDOWN)
				{
					if (sEvent.key.keysym.sym==SDLK_q)
						{
							return ;
							break;
						}
					/*if (state==EState::newGame)
					{
						switch (sEvent.key.keysym.sym)
						{
						case SDLK_LEFT:
							{
								if(currentItems()->lNewGame.x>0)
									currentItems()->lNewGame.x--;
								break;
							}
						case (SDLK_RIGHT):
							{
									currentItems()->lNewGame.x++;
								break;
							}
						case (SDLK_UP):
							{
								if(currentItems()->lNewGame.y>0)
									currentItems()->lNewGame.y--;
								break;
							}
						case (SDLK_DOWN):
							{
									currentItems()->lNewGame.y++;
								break;
							}
						}
						showNewMenu();
					}*/
				}
				else if (sEvent.type==SDL_MOUSEMOTION)
				{
					if (currentMessage) continue;
					if (current->highlighted)
					{
						switch (current->highlighted)
						{
							case 1:
								{
									if(isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(1,0);
									}
									break;
								}
							case 2:
								{
									if(isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(2,0);
									}
									break;
								}
							case 3:
								{
									if(isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(3,0);
									}
									break;
								}
							case 4:
								{
									if(isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(4,0);
									}
									break;
								}
							case 5:
								{
									if(isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
										continue;
									else
									{
										current->highlighted=0;
										highlightButton(5,0);
									}
									break;
								}
						} //switch (current->highlighted)
					} // if (current->highlighted)
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						current->highlighted=1;
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						current->highlighted=2;
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						current->highlighted=5;
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (int i=0;i<btns.size(); i++)
					{
						if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
						{
							btns[i]->press(true);
							mush->playClick();
							//SDL_BlitSurface((btns[i].imgs)->ourImages[1].bitmap,NULL,ekran,&btns[i].pos);
							//updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,1);
						current->highlighted=1;
						mush->playClick();
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,1);
						current->highlighted=2;
						mush->playClick();
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,1);
						current->highlighted=3;
						mush->playClick();
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,1);
						current->highlighted=4;
						mush->playClick();
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,1);
						current->highlighted=5;
						mush->playClick();
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
				{
					for (int i=0;i<btns.size(); i++)
					{
						if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
							(this->*(btns[i]->fun))();
						else
						{
							btns[i]->press(false);
							//SDL_BlitSurface((btns[i].imgs)->ourImages[0].bitmap,NULL,ekran,&btns[i].pos);
							//updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,2);
						current->highlighted=1;
						(this->*(current->fNewGame))();
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,2);
						current->highlighted=2;
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,2);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,2);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,2);
						current->highlighted=5;
						(this->*(current->fQuit))();
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_RIGHT))
				{
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(0));
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(1));
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(2));
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(3));
					}
					else if (isItIn(&ourMainMenu->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						showCenBox(buttonText(4));
					}
				}
				else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_RIGHT) && currentMessage)
				{
					hideBox();
				}
			}
		}
		catch(...)
		{ continue; }

		SDL_Delay(30); //give time for other apps
	}
}
std::string CPreGame::buttonText(int which)
{
	if (state==EState::mainMenu)
	{
		switch (which)
		{
		case 0:
			return preth->mainNewGame;
		case 1:
			return preth->mainLoadGame;
		case 2:
			return preth->mainHighScores;
		case 3:
			return preth->mainCredits;
		case 4:
			return preth->mainQuit;
		}
	}
	else if (state==EState::newGame)
	{
		switch (which)
		{
		case 0:
			return preth->ngSingleScenario;
		case 1:
			return preth->ngMultiplayer;
		case 2:
			return preth->ngCampain;
		case 3:
			return preth->ngTutorial;
		case 4:
			return preth->ngBack;
		}
	}
}
void CPreGame::quitAskBox()
{
	showAskBox("\"{}  Are you sure you want to quit?\"",NULL,NULL);
}