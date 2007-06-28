#include "stdafx.h"
#include "CPreGame.h"
#include "SDL.h"
#include "boost/filesystem.hpp"   // includes all needed Boost.Filesystem declarations
#include "boost/algorithm/string.hpp"
//#include "boost/foreach.hpp"
#include "zlib.h"
#include <sstream>
extern SDL_Surface * ekran;
extern SDL_Color tytulowy, tlo, zwykly ;
extern TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX;
SDL_Rect genRect(int hh, int ww, int xx, int yy);
SDL_Color genRGB(int r, int g, int b, int a=0);
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran);
void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran);
CPreGame * CPG;
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran);
bool isItIn(const SDL_Rect * rect, int x, int y);
namespace fs = boost::filesystem;
void OverButton::show()
{
	blitAt(imgs->ourImages[0].bitmap,pos.x,pos.y);
	updateRect(&pos);
}
void OverButton::press(bool down)
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

//void Slider::clickDown(int x, int y, bool bzgl=true);
//void Slider::clickUp(int x, int y, bool bzgl=true);
//void Slider::mMove(int x, int y, bool bzgl=true);

void Slider<>::updateSlid()
{
	float perc = ((float)whereAreWe)/((float)positionsAmnt-capacity);
	float myh=perc*((float)pos.h-48)+pos.y+16;
	SDL_FillRect(ekran,&genRect(pos.h-32,pos.w,pos.x,pos.y+16),0);
	blitAt(slider.imgs->ourImages[0].bitmap,pos.x,(int)myh);
	slider.pos.y=(int)myh;
	updateRect(&pos);
}

void Slider<>::moveDown()
{
	if (whereAreWe<positionsAmnt-capacity)
		(CPG->*fun)(++whereAreWe);
	updateSlid();
}
void Slider<>::moveUp()
{
	if (whereAreWe>0)
		(CPG->*fun)(--whereAreWe);
	updateSlid();
}
//void Slider::moveByOne(bool up);
Slider<>::Slider(int x, int y, int h, int amnt, int cap)
{
	positionsAmnt = amnt;
	capacity = cap;
	pos = genRect(h,16,x,y);
	down = Button<void(Slider::*)()>(genRect(16,16,x,y+h-16),&Slider::moveDown,CPG->slh->giveDef("SCNRBDN.DEF"),false);
	up = Button<void(Slider::*)()>(genRect(16,16,x,y),&Slider::moveUp,CPG->slh->giveDef("SCNRBUP.DEF"),false);
	slider = Button<void(Slider::*)()>(genRect(16,16,x,y+16),NULL,CPG->slh->giveDef("SCNRBSL.DEF"),false);
	moving = false;
	whereAreWe=0;

	pos = genRect(h,16,x,y);
}
void Slider<>::activate(MapSel * ms)
{
	SDL_FillRect(ekran,&pos,0);
	up.show();
	down.show();
	slider.show();
	SDL_Flip(ekran);
	CPG->interested.push_back(this);
}	
void Slider<>::handleIt(SDL_Event sEvent)
{
	if ((sEvent.type==SDL_MOUSEBUTTONDOWN) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		if (isItIn(&down.pos,sEvent.motion.x,sEvent.motion.y))
		{
			down.press();
		}
		else if (isItIn(&up.pos,sEvent.motion.x,sEvent.motion.y))
		{
			up.press();
		}
		else if (isItIn(&slider.pos,sEvent.motion.x,sEvent.motion.y))
		{
			//slider.press();
			moving=true;
		}
		else if (isItIn(&pos,sEvent.motion.x,sEvent.motion.y))
		{
			float dy = sEvent.motion.y-pos.y-16;
			float pe = dy/((float)(pos.h-32));
			if (pe>1) pe=1;
			if (pe<0) pe=0;
			whereAreWe = pe*(positionsAmnt-capacity);
			if (whereAreWe<0)whereAreWe=0;
			updateSlid();
			(CPG->*fun)(whereAreWe);
		}
	}
	else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{

		if ((down.state==1) && isItIn(&down.pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*down.fun)();
		}
		if ((up.state==1) && isItIn(&up.pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*up.fun)();
		}
		if (down.state==1) down.press(false);
		if (up.state==1) up.press(false);
		if (moving)
		{
			//slider.press();
			moving=false;
		}
	}
	//else if (sEvent.type==SDL_KEYDOWN)
	//{
	//	switch (sEvent.key.keysym.sym)
	//	{
	//	case (SDLK_UP):
	//		moveUp();
	//		break;
	//	case (SDLK_DOWN):
	//		moveDown();
	//		break;
	//	}
	//}
	else if (moving && sEvent.type==SDL_MOUSEMOTION)
	{
		if (isItIn(&genRect(pos.h,pos.w+64,pos.x-32,pos.y),sEvent.motion.x,sEvent.motion.y))
		{
			int my = sEvent.motion.y-(pos.y+16);
			int all =pos.h-48;
			float ile = (float)my / (float)all;
			if (ile>1) ile=1;
			if (ile<0) ile=0;
			int ktory = ile*(positionsAmnt-capacity);
			if (ktory!=whereAreWe)
			{
				whereAreWe=ktory;
				updateSlid();
			}
			(CPG->*fun)(whereAreWe);
		}
	}
	/*else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
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


		if (isItIn(&down.pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*down.fun)();
		}
		if (isItIn(&up.pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*up.fun)();
		}
		if (isItIn(&slider.pos,sEvent.motion.x,sEvent.motion.y))
		{
			(this->*slider.fun)();
		}

	}*/
}
MapSel::~MapSel()
{
	SDL_FreeSurface(bg);
	for (int i=0;i<scenImgs.size();i++)
		SDL_FreeSurface(scenImgs[i]);
	for (int i=0;i<scenList.size();i++)
		delete scenList[i];
}
int MapSel::countWL()
{
	int ret=0;
	for (int i=0;i<ourMaps.size();i++)
	{
		if (sizeFilter && ((ourMaps[i].width) != sizeFilter))
			continue;
		else ret++;
	}
	return ret;
}
void MapSel::printMaps(int from, int to, int at, bool abs)
{
	if (true)//
	{
		int help=-1;
		for (int i=0;i<ourMaps.size();i++)
		{
			if (sizeFilter && ((ourMaps[i].width) != sizeFilter))
				continue;
			else help++;
			if (help==from)
			{
				from=i;
				break;
			}
		}
	}
	SDL_Color nasz;
	for (int i=at;i<to;i++)
	{
		if ((i-at+from) > ourMaps.size()-1)
		{
			SDL_Surface * scenin = SDL_CreateRGBSurface(ekran->flags,351,25,ekran->format->BitsPerPixel,ekran->format->Rmask,ekran->format->Gmask,ekran->format->Bmask,ekran->format->Amask);
			SDL_BlitSurface(bg,&genRect(25,351,22,(i-at)*25+115),scenin,NULL);
			blitAt(scenin,24,121+(i-at)*25);
			SDL_Flip(ekran);
			SDL_FreeSurface(scenin);
			continue;
		}
		if (sizeFilter && ((ourMaps[(i-at)+from].width) != sizeFilter))
		{
			to++;
			at++;
			from++;
			if (((i-at)+from)>ourMaps.size()-1) break;
			else continue;
		}
		if ((i-at+from) == selected)
			nasz=tytulowy;
		else nasz=zwykly;
		//SDL_Rect pier = genRect(25,351,24,126+(i*25));
		SDL_Surface * scenin = SDL_CreateRGBSurface(ekran->flags,351,25,ekran->format->BitsPerPixel,ekran->format->Rmask,ekran->format->Gmask,ekran->format->Bmask,ekran->format->Amask);
		SDL_BlitSurface(bg,&genRect(25,351,22,(i-at)*25+115),scenin,NULL);
		int temp=-1;
		std::ostringstream ostr(std::ostringstream::out); ostr << ourMaps[(i-at)+from].playerAmnt << "/" << ourMaps[(i-at)+from].humenPlayers;
		printAt(ostr.str(),6,4,GEOR13,nasz,scenin); 
		std::string temp2;
		switch (ourMaps[(i-at)+from].width)
		{
		case 36:
			temp2="S";
			break;
		case 72:
			temp2="M";
			break;
		case 108:
			temp2="L";
			break;
		case 144:
			temp2="XL";
			break;
		default:
			temp2="C";
			break;
		}
		printAt(temp2,43,5,GEOR13,nasz,scenin);
		switch (ourMaps[(i-at)+from].version)
		{
		case Eformat::SoD:
			temp=2;
			break;
		case Eformat::WoG:
			temp=3;
			break;
		}
		blitAt(Dtypes->ourImages[temp].bitmap,67,2,scenin);
		if (!(ourMaps[(i-at)+from].name.length()))
			ourMaps[(i-at)+from].name = "Unnamed";
		printAt(ourMaps[(i-at)+from].name,105,6,GEOR13,nasz,scenin);
		if (ourMaps[(i-at)+from].victoryCondition==EvictoryConditions::winStandard)
			temp=11;
		else temp=ourMaps[(i-at)+from].victoryCondition;
		blitAt(Dvic->ourImages[temp].bitmap,285,2,scenin);
		if (ourMaps[(i-at)+from].lossCondition.typeOfLossCon == ElossCon::lossStandard)
			temp=3;
		else temp=ourMaps[(i-at)+from].lossCondition.typeOfLossCon;
		blitAt(Dloss->ourImages[temp].bitmap,318,2,scenin);

		blitAt(scenin,24,121+(i-at)*25);

		SDL_Flip(ekran);

		SDL_FreeSurface(scenin);
	}
}
int MapSel::whichWL(int nr)
{
	int help=-1;
	for (int i=0;i<ourMaps.size();i++)
	{
		if (sizeFilter && ((ourMaps[i].width) != sizeFilter))
			continue;
		else help++;
		if (help==nr)
		{
			help=i;
			break;
		}
	}
	return help;
}
void MapSel::draw()
{
	//blit bg
	blitAt(bg,2,6);
	printAt("Map Sizes",55,60,GEOR13);
	printAt("Select a Scenario to Play",110,25,TNRB16);
	//size buttons
	small.show();
	medium.show();
	large.show();
	xlarge.show();
	all.show();
	CPG->btns.push_back(&small);
	CPG->btns.push_back(&medium);
	CPG->btns.push_back(&large);
	CPG->btns.push_back(&xlarge);
	CPG->btns.push_back(&all);
	//print scenario list
	printMaps(0,18);

	slid->activate(this);

	SDL_Flip(ekran);
}
void MapSel::init()
{
	bg = SDL_LoadBMP("h3bitmap.lod\\SCSELBCK.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	small.imgs = CPG->slh->giveDef("SCSMBUT.DEF");
	small.fun = NULL;
	small.pos = genRect(small.imgs->ourImages[0].bitmap->h,small.imgs->ourImages[0].bitmap->w,161,52);
	small.ourGroup=NULL;
	medium.imgs = CPG->slh->giveDef("SCMDBUT.DEF");
	medium.fun = NULL;
	medium.pos = genRect(medium.imgs->ourImages[0].bitmap->h,medium.imgs->ourImages[0].bitmap->w,208,52);
	medium.ourGroup=NULL;
	large.imgs = CPG->slh->giveDef("SCLGBUT.DEF");
	large.fun = NULL;
	large.pos = genRect(large.imgs->ourImages[0].bitmap->h,large.imgs->ourImages[0].bitmap->w,255,52);
	large.ourGroup=NULL;
	xlarge.imgs = CPG->slh->giveDef("SCXLBUT.DEF");
	xlarge.fun = NULL;
	xlarge.pos = genRect(xlarge.imgs->ourImages[0].bitmap->h,xlarge.imgs->ourImages[0].bitmap->w,302,52);
	xlarge.ourGroup=NULL;
	all.imgs = CPG->slh->giveDef("SCALBUT.DEF");
	all.fun = NULL;
	all.pos = genRect(all.imgs->ourImages[0].bitmap->h,all.imgs->ourImages[0].bitmap->w,349,52);
	all.ourGroup=NULL;
	all.selectable=xlarge.selectable=large.selectable=medium.selectable=small.selectable=false;
	small.what=medium.what=large.what=xlarge.what=all.what=&sizeFilter;
	small.key=36;medium.key=72;large.key=108;xlarge.key=144;all.key=0;
	Dtypes = CPG->slh->giveDef("SCSELC.DEF");
	Dvic = CPG->slh->giveDef("SCNRVICT.DEF");
	Dloss = CPG->slh->giveDef("SCNRLOSS.DEF");
	Dsizes = CPG->slh->giveDef("SCNRMPSZ.DEF");
	//get map files names
	std::vector<std::string> pliczkiTemp;
	boost::filesystem::path tie( (fs::initial_path<fs::path>())/"\maps" );
	fs::directory_iterator end_iter;
	for ( boost::filesystem::directory_iterator dir (tie); dir!=end_iter; ++dir )
	{
		if (boost::filesystem::is_regular(dir->status()));
		{
			if (boost::ends_with(dir->path().leaf(),std::string(".h3m")))
				pliczkiTemp.push_back("Maps/"+(dir->path().leaf()));
		}
	}
	for (int i=0; i<pliczkiTemp.size();i++)
	{
		gzFile tempf = gzopen(pliczkiTemp[i].c_str(),"rb");
		std::string sss;
		int iii=0;
		while(++iii)
		{
			if (iii>3300) break;
			int z = gzgetc (tempf);
			if (z>=0) 
			{
				sss+=unsigned char(z);
			}
			else break;
		}
		gzclose(tempf);
		if (sss[0]<28) continue; //zly format
		if (!sss[4]) continue; //nie ma graczy? mapa niegrywalna
		unsigned char* file2 = new unsigned char[sss.length()];
		for (int j=0;j<sss.length();j++)
			file2[j]=sss[j];
		ourMaps.push_back(CMapInfo(pliczkiTemp[i],file2));
	}
	std::sort(ourMaps.begin(),ourMaps.end(),mapSorter(ESortBy::name));
	slid = new Slider<>(375,92,480,ourMaps.size(),18);
	slid->fun = &CPreGame::printMapsFrom;
}
void MapSel::select(int which)
{
	//selected = which;

	//if ((slid->whereAreWe > which) || (slid->whereAreWe+18 < which))
	//	selected = which;
	//else 
	{
		selected = which;
		printMaps(slid->whereAreWe,18,0,true);
	}
	printSelectedInfo();
}
MapSel::MapSel():selected(0),sizeFilter(0)
{
}
void MapSel::printSelectedInfo()
{
	SDL_BlitSurface(CPG->ourScenSel->scenInf,&genRect(399,337,17,23),ekran,&genRect(399,337,412,29));
	SDL_BlitSurface(CPG->ourScenSel->scenInf,&genRect(50,91,18,447),ekran,&genRect(50,91,413,453));
	SDL_BlitSurface(CPG->ourScenSel->bScens.imgs->ourImages[0].bitmap,NULL,ekran,&CPG->ourScenSel->bScens.pos);
	SDL_BlitSurface(CPG->ourScenSel->bOptions.imgs->ourImages[0].bitmap,NULL,ekran,&CPG->ourScenSel->bOptions.pos);
	SDL_BlitSurface(CPG->ourScenSel->bRandom.imgs->ourImages[0].bitmap,NULL,ekran,&CPG->ourScenSel->bRandom.pos);
	//blit texts
	printAt(CPG->preth->singleScenarioName,420,25,GEOR13);
	printAt("Scenario Description:",420,135,GEOR13);
	printAt("Victory Condition:",420,285,GEOR13);
	printAt("Loss Condition:",420,340,GEOR13);
	printAt("Allies:",420,406,GEOR13,zwykly);
	printAt("Enemies:",585,406,GEOR13,zwykly);

	int temp = ourMaps[selected].victoryCondition+1;
	if (temp>20) temp=0;
	std::string sss = CPG->preth->victoryConditions[temp];
	if (temp && ourMaps[selected].vicConDetails->allowNormalVictory) sss+= "/" + CPG->preth->victoryConditions[0];
	printAt(sss,452,310,GEOR13,zwykly);


	temp = ourMaps[selected].lossCondition.typeOfLossCon+1;
	if (temp>20) temp=0;
	sss = CPG->preth->lossCondtions[temp];
	printAt(sss,452,370,GEOR13,zwykly);

	//blit descrption
	std::vector<std::string> desc = *CMessage::breakText(ourMaps[selected].description,50);
	for (int i=0;i<desc.size();i++)
		printAt(desc[i],417,152+i*13,GEOR13,zwykly);

	if ((selected < 0) || (selected >= ourMaps.size()))
		return; 
	if (ourMaps[selected].name.length())
		printAt(ourMaps[selected].name,420,41,GEORXX);
	else printAt("Unnamed",420,41,GEORXX);
	std::string diff;
	switch (ourMaps[selected].difficulty)
	{
	case 0:
		diff=gdiff(CPG->preth->singleEasy);
		break;
	case 1:
		diff=gdiff(CPG->preth->singleNormal);
		break;
	case 2:
		diff=gdiff(CPG->preth->singleHard);
		break;
	case 3:
		diff=gdiff(CPG->preth->singleExpert);
		break;
	case 4:
		diff=gdiff(CPG->preth->singleImpossible);
		break;
	}
	temp=-1;
	switch (ourMaps[selected].width)
	{
	case 36:
		temp=0;
		break;
	case 72:
		temp=1;
		break;
	case 108:
		temp=2;
		break;
	case 144:
		temp=3;
		break;
	default:
		temp=4;
		break;
	}
	blitAt(Dsizes->ourImages[temp].bitmap,714,28);
	temp=ourMaps[selected].victoryCondition;
	if (temp>12) temp=11;
	blitAt(Dvic->ourImages[temp].bitmap,420,308); //v
	temp=ourMaps[selected].lossCondition.typeOfLossCon;
	if (temp>12) temp=3;
	blitAt(Dloss->ourImages[temp].bitmap,420,366); //l



	printAt(diff,435,470,GEOR13,zwykly);
	SDL_Flip(ekran);
}
std::string MapSel::gdiff(std::string ss)
{
	std::string ret;
	for (int i=2;i<ss.length();i++)
	{
		if (ss[i]==' ')
			break;
		ret+=ss[i];
	}
	return ret;
}
void CPreGame::printMapsFrom(int from)
{
	ourScenSel->mapsel.printMaps(from);
}
void CPreGame::showScenList()
{
	ourScenSel->listShowed=true;
	ourScenSel->mapsel.draw();
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
	ourScenSel->listShowed=false;
	if (rand()%2) ourScenSel->background=SDL_LoadBMP("h3bitmap.lod\\ZPIC1000.bmp");
	else ourScenSel->background=SDL_LoadBMP("h3bitmap.lod\\ZPIC1001.bmp");

	ourScenSel->pressed=NULL;

	ourScenSel->scenInf=SDL_LoadBMP("h3bitmap.lod\\GSELPOP1.bmp");
	ourScenSel->randMap=SDL_LoadBMP("h3bitmap.lod\\RANMAPBK.bmp");
	ourScenSel->options=SDL_LoadBMP("h3bitmap.lod\\ADVOPTBK.bmp");
	SDL_SetColorKey(ourScenSel->scenInf,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->scenInf->format,0,255,255));
	//SDL_SetColorKey(ourScenSel->scenList,SDL_SRCCOLORKEY,SDL_MapRGB(ourScenSel->scenList->format,0,255,255));
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
	ourScenSel->bBegin = Button<>(genRect(0,0,414,535),&CPreGame::showNewMenu,slh->giveDef("SCNRBEG.DEF"));

	ourScenSel->bScens = Button<>(genRect(0,0,414,81),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bScens.imgs->ourImages.size(); i++)
		printAt("Show Available Scenarios",25+i,2+i,GEOR13,zwykly,ourScenSel->bScens.imgs->ourImages[i].bitmap);
	ourScenSel->bRandom = Button<>(genRect(0,0,414,105),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bRandom.imgs->ourImages.size(); i++)
		printAt("Random Map",25+i,2+i,GEOR13,zwykly,ourScenSel->bRandom.imgs->ourImages[i].bitmap);
	ourScenSel->bOptions = Button<>(genRect(0,0,414,509),&CPreGame::showScenList,slh->giveDef("GSPBUTT.DEF"));
	for (int i=0; i<ourScenSel->bOptions.imgs->ourImages.size(); i++)
		printAt("Show Advanced Options",25+i,2+i,GEOR13,zwykly,ourScenSel->bOptions.imgs->ourImages[i].bitmap);

	CPG=this;
	ourScenSel->mapsel.init();
}

void CPreGame::showScenSel()
{
	state=EState::ScenarioList;
	SDL_BlitSurface(ourScenSel->background,NULL,ekran,NULL);
	SDL_BlitSurface(ourScenSel->scenInf,NULL,ekran,&genRect(ourScenSel->scenInf->h,ourScenSel->scenInf->w,395,6));
	printAt("Map Diff:",427,438,GEOR13);
	printAt("Player Difficulty:",527,438,GEOR13);
	printAt("Rating:",685,438,GEOR13);
	//blit buttons
	SDL_BlitSurface(ourScenSel->bEasy.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bEasy.pos);
	SDL_BlitSurface(ourScenSel->bNormal.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bNormal.pos);
	SDL_BlitSurface(ourScenSel->bHard.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bHard.pos);
	SDL_BlitSurface(ourScenSel->bExpert.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bExpert.pos);
	SDL_BlitSurface(ourScenSel->bImpossible.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bImpossible.pos);
	SDL_BlitSurface(ourScenSel->bBegin.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bBegin.pos);
	SDL_BlitSurface(ourScenSel->bBack.imgs->ourImages[0].bitmap,NULL,ekran,&ourScenSel->bBack.pos);
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

	ourScenSel->mapsel.printSelectedInfo();
	SDL_Flip(ekran);

}
void CPreGame::showOptions()
{}
void CPreGame::initNewMenu()
{
	ourNewMenu = new menuItems();
	ourNewMenu->bgAd = SDL_LoadBMP("h3bitmap.lod\\ZNEWGAM.bmp");
	ourNewMenu->background = SDL_LoadBMP("h3bitmap.lod\\ZPIC1005.bmp");
	blitAt(ourNewMenu->bgAd,114,312,ourNewMenu->background);
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
	handleOther=NULL;
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
				btns[i]->press(true);
				ourScenSel->pressed=btns[i];
			}
		}
		if (ourScenSel->listShowed && (sEvent.button.y>121) &&(sEvent.button.y<570) 
									&& (sEvent.button.x>55) && (sEvent.button.x<372))
		{
			int py = ((sEvent.button.y-121)/25)+ourScenSel->mapsel.slid->whereAreWe;
			ourScenSel->mapsel.select(ourScenSel->mapsel.whichWL(py));
		}

	}
	else if ((sEvent.type==SDL_MOUSEBUTTONUP) && (sEvent.button.button == SDL_BUTTON_LEFT))
	{
		for (int i=0;i<btns.size(); i++)
		{
			if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
			{
				if (btns[i]->selectable)
					btns[i]->select(true);
				if (btns[i]->fun)
					(this->*(btns[i]->fun))();
				int zz = btns.size();
				if (i>=zz) 
					break;
				if  (btns[i]->state && btns[i]->type==2)
				{
					((IntBut<> *)(btns[i]))->set();
					ourScenSel->mapsel.slid->whereAreWe=0;
					ourScenSel->mapsel.slid->updateSlid();
					ourScenSel->mapsel.slid->positionsAmnt=ourScenSel->mapsel.countWL();
					ourScenSel->mapsel.printMaps(0);
				}
			}
		}
		if (ourScenSel->pressed && ourScenSel->pressed->state==1)
		{
			ourScenSel->pressed->press(false);
			ourScenSel->pressed=NULL;
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
				for (int i=0;i<interested.size();i++)
					interested[i]->handleIt(sEvent);
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
					mush->playClick();
					for (int i=0;i<btns.size(); i++)
					{
						if (isItIn(&btns[i]->pos,sEvent.motion.x,sEvent.motion.y))
						{
							btns[i]->press(true);
							//SDL_BlitSurface((btns[i].imgs)->ourImages[1].bitmap,NULL,ekran,&btns[i].pos);
							//updateRect(&btns[i].pos);
						}
					}
					if (currentMessage) continue;
					if (isItIn(&current->lNewGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(1,1);
						current->highlighted=1;
					}
					else if (isItIn(&current->lLoadGame,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(2,1);
						current->highlighted=2;
					}
					else if (isItIn(&current->lHighScores,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(3,1);
						current->highlighted=3;
					}
					else if (isItIn(&current->lCredits,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(4,1);
						current->highlighted=4;
					}
					else if (isItIn(&current->lQuit,sEvent.motion.x,sEvent.motion.y))
					{
						highlightButton(5,1);
						current->highlighted=5;
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