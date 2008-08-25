#include "stdafx.h"
#include "CMessage.h"
#include "SDL_ttf.h"
#include "hch/CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "hch/CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "CPlayerInterface.h"
#include "hch/CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include <sstream>
#include "hch/CGeneralTextHandler.h"
#include "client/Graphics.h"
#include "CAdvmapInterface.h"
SDL_Color tytulowy, tlo, zwykly ;
SDL_Rect genRect(int hh, int ww, int xx, int yy);

extern SDL_Surface * screen;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Color genRGB(int r, int g, int b, int a=0);
//void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=screen, unsigned char quality = 2);
bool isItIn(const SDL_Rect * rect, int x, int y);

using namespace NMessage;
template <typename T, typename U> std::pair<T,U> max(const std::pair<T,U> &x, const std::pair<T,U> &y)
{
	std::pair<T,U> ret;
	ret.first = std::max(x.first,y.first);
	ret.second = std::max(x.second,y.second);
	return ret;
}

namespace NMessage
{
	CDefHandler * ok, *cancel;
	std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	SDL_Surface * background = NULL;
}

CMessage::CMessage()
{
	//if (!NMessage::background)
	//	init();
}
void CMessage::init()
{
	{
		for (int i=0;i<PLAYER_LIMIT;i++)
		{
			CDefHandler * bluePieces = CDefHandler::giveDef("DIALGBOX.DEF");
			std::vector<SDL_Surface *> n;
			piecesOfBox.push_back(n);
			if (i==1)
			{
				for (int j=0;j<bluePieces->ourImages.size();j++)
				{
					piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
				}
			}
			for (int j=0;j<bluePieces->ourImages.size();j++)
			{
				graphics->blueToPlayersAdv(bluePieces->ourImages[j].bitmap,i);
				piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
			}
		}
		NMessage::background = BitmapHandler::loadBitmap("DIBOXBCK.BMP");
		SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	}
	ok = CDefHandler::giveDef("IOKAY.DEF");
	cancel = CDefHandler::giveDef("ICANCEL.DEF");
}


void CMessage::dispose()
{
	for (int i=0;i<PLAYER_LIMIT;i++)
	{
		for (int j=0;j<piecesOfBox[i].size();j++)
		{
			SDL_FreeSurface(piecesOfBox[i][j]);
		}
	}
	SDL_FreeSurface(background);
	delete ok;
	delete cancel;
}
SDL_Surface * CMessage::drawBox1(int w, int h, int playerColor) //draws box for window
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(screen->flags, w, h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	for (int i=0; i<h; i+=background->h)//background
	{
		for (int j=0; j<w; j+=background->w-1)
			SDL_BlitSurface(background,&genRect(background->h,background->w-1,1,0),ret,&genRect(h,w,j,i));
	}

	//obwodka I-szego rzedu pozioma
	for (int i=0; i<w; i+=piecesOfBox[playerColor][6]->w)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][6],NULL,ret,&genRect(piecesOfBox[playerColor][6]->h,piecesOfBox[playerColor][6]->w,i,0));
		SDL_BlitSurface
			(piecesOfBox[playerColor][7],NULL,ret,&genRect(piecesOfBox[playerColor][7]->h,piecesOfBox[playerColor][7]->w,i,h-piecesOfBox[playerColor][7]->h));
	}
	//obwodka I-szego rzedu pionowa
	for (int i=0; i<h; i+=piecesOfBox[playerColor][4]->h)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][4],NULL,ret,&genRect(piecesOfBox[playerColor][4]->h,piecesOfBox[playerColor][4]->w,0,i));
		SDL_BlitSurface
			(piecesOfBox[playerColor][5],NULL,ret,&genRect(piecesOfBox[playerColor][5]->h,piecesOfBox[playerColor][5]->w,w-piecesOfBox[playerColor][5]->w,i));
	}
	//corners
	SDL_BlitSurface
		(piecesOfBox[playerColor][0],NULL,ret,&genRect(piecesOfBox[playerColor][0]->h,piecesOfBox[playerColor][0]->w,0,0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][1],NULL,ret,&genRect(piecesOfBox[playerColor][1]->h,piecesOfBox[playerColor][1]->w,w-piecesOfBox[playerColor][1]->w,0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][2],NULL,ret,&genRect(piecesOfBox[playerColor][2]->h,piecesOfBox[playerColor][2]->w,0,h-piecesOfBox[playerColor][2]->h));
	SDL_BlitSurface
		(piecesOfBox[playerColor][3],NULL,ret,&genRect(piecesOfBox[playerColor][3]->h,piecesOfBox[playerColor][3]->w,w-piecesOfBox[playerColor][3]->w,h-piecesOfBox[playerColor][3]->h));
	//box gotowy!
	return ret;
}

std::vector<std::string> * CMessage::breakText(std::string text, int line, bool userBreak, bool ifor)
{
	std::vector<std::string> * ret = new std::vector<std::string>();
	while (text.length()>line)
	{
		int whereCut = -1, braces=0;
		bool pom = true, opened=false;
		for (int i=0; i<line+braces; i++)
		{
			if (text[i]==10) //end of line sign
			{
				whereCut=i+1;
				pom=false;
				break;
			}
			else if (ifor && (text[i]=='{') || (text[i]=='}')) // ignore braces
			{
				if (text[i]=='{')
					opened=true;
				else
					opened=false;
				braces++;
			}
		}
		for (int i=line+braces; i>0&&pom; i--)
		{
			if (text[i]==' ')
			{
				whereCut = i;
				break;
			}
			else if (opened && text[i]=='{')
				opened = false;
			else if (text[i]=='}')
				opened = true;
		}
		ret->push_back(text.substr(0,whereCut));
		text.erase(0,whereCut);
		boost::algorithm::trim_left_if(text,boost::algorithm::is_any_of(" "));
		if (opened)
		{
			(*(ret->end()-1))+='}';
			text.insert(0,"{");
		}
	}
	for (int i=0;i<text.length();i++)
	{
		if (text[i]==10) //end of line sign
		{
			ret->push_back(text.substr(0,i));
			text.erase(0,i);
			i=0;
		}
	}
	if (text.length() > 0)
		ret->push_back(text);
	for (int i=0; i<ret->size(); i++)
	{
		boost::algorithm::trim((*ret)[i]);
	}
	return ret;
}
std::pair<int, int> CMessage::getMaxSizes(std::vector< std::vector<SComponent*> > * komp)
{
	std::pair<int,int> ret;
	for (int i=0;i<komp->size();i++)
	{
		int sumaw=0;
		int maxh=0;
		for(int j=0;j<(*komp)[i].size();j++)
		{
			sumaw += (*komp)[i][j]->getImg()->w;
			if (maxh < (*komp)[i][j]->getImg()->h)
				maxh = (*komp)[i][j]->getImg()->h;
		}
		if(sumaw>ret.first)
			ret.first = sumaw;
		ret.second+=maxh;
	}

	return ret;
}

std::pair<int,int> CMessage::getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg)
{
	std::pair<int,int> ret;
	ret.first = -1;
	ret.second=0;
	for (int i=0; i<txtg->size();i++) //szukamy najszerszej linii i lacznej wysokosci
	{
		int lw=0;
		for (int j=0;j<(*txtg)[i].size();j++)
		{
			lw+=(*txtg)[i][j]->w;
			ret.second+=(*txtg)[i][j]->h;
		}
		if(!(*txtg)[i].size())
			ret.second+=19;
		if (ret.first<lw)
			ret.first=lw;
	}
	return ret;
}
SDL_Surface * CMessage::blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int & curh, SDL_Surface * ret)
{
	for (int i=0; i<txtg->size();i++)
	{
		int lw=0;
		for (int j=0;j<(*txtg)[i].size();j++)
			lw+=(*txtg)[i][j]->w; //lw - laczna szerokosc linii
		int pw = ret->w/2;
		pw -= lw/2; //poczatek tekstu (x)

		int tw = pw;
		for (int j=0;j<(*txtg)[i].size();j++) //blit text
		{
			blitAt((*txtg)[i][j],tw,curh+i*19,ret);
			tw+=(*txtg)[i][j]->w;
			SDL_FreeSurface((*txtg)[i][j]);
		}
	}
	curh+=txtg->size()*19;
	return ret;
}
SDL_Surface * CMessage::blitCompsOnSur(std::vector<SComponent*> & comps, int maxw, int inter, int & curh, SDL_Surface * ret)
{
	std::vector<std::string> * brdtext;
	if (comps.size())
		brdtext = breakText(comps[0]->subtitle,12,true,true);
	else
		brdtext = NULL;
	comps[0]->pos.x = (ret->w/2) - ((comps[0]->getImg()->w)/2);
	comps[0]->pos.y = curh;
	blitAt(comps[0]->getImg(),comps[0]->pos.x,comps[0]->pos.y,ret);
	curh += comps[0]->getImg()->h + 5; //obrazek + przerwa
	for (int i=0; i<brdtext->size();i++) //descr.
	{
		SDL_Surface * tesu = TTF_RenderText_Blended(GEOR13,(*brdtext)[i].c_str(),zwykly);
		blitAt(tesu,((comps[0]->getImg()->w - tesu->w)/2)+comps[0]->pos.x,curh,ret);
		curh+=tesu->h;
		SDL_FreeSurface(tesu);
	}
	return ret;
}
SDL_Surface* CMessage::blitCompsOnSur(SDL_Surface * _or, std::vector< std::vector<SComponent*> > *  komp, int inter, int &curh, SDL_Surface *ret)
{
	for (int i=0;i<komp->size();i++)
	{
		int totalw=0, maxh=0;
		for(int j=0;j<(*komp)[i].size();j++)
		{
			totalw+=(*komp)[i][j]->getImg()->w;
			if(maxh<(*komp)[i][j]->getImg()->h)
				maxh=(*komp)[i][j]->getImg()->h;
		}
		if(_or)
			totalw += (inter*2+_or->w) * ((*komp)[i].size() - 1);
		else
			totalw += (inter) * ((*komp)[i].size() - 1);

		curh+=maxh/2;
		int curw = (ret->w/2)-(totalw/2);
		for(int j=0;j<(*komp)[i].size();j++)
		{
			blitAt((*komp)[i][j]->getImg(),curw,curh-((*komp)[i][j]->getImg()->h/2),ret);
			(*komp)[i][j]->pos.x = curw;
			(*komp)[i][j]->pos.y = curh-((*komp)[i][j]->getImg()->h/2);
			CSDL_Ext::printAtMiddle((*komp)[i][j]->subtitle,curw+(*komp)[i][j]->getImg()->w/2,curh+((*komp)[i][j]->getImg()->h/2)+10,GEOR13,zwykly,ret);
			curw += (*komp)[i][j]->getImg()->w;
			if(j<((*komp)[i].size()-1))
			{
				if(_or)
				{
					curw+=inter;
					blitAt(_or,curw,curh-(_or->h/2),ret);
					curw+=_or->w;
				}
				curw+=inter;
			}
		}
		curh+=maxh/2;
		curh += 20; //todo: check subtitle length
	}
	return ret;
}
std::vector<std::vector<SDL_Surface*> > * CMessage::drawText(std::vector<std::string> * brtext)
{
	std::vector<std::vector<SDL_Surface*> > * txtg = new std::vector<std::vector<SDL_Surface*> >();
	txtg->resize(brtext->size());
	for (int i=0; i<brtext->size();i++) //foreach line
	{
		while((*brtext)[i].length()) //jesli zostalo cos
		{
			int z=0; bool br=true;
			while( ((*brtext)[i][z]) != ('{') )
			{
				if (z >= (((*brtext)[i].length())-1))
				{
					br=false;
					break;
				}
				z++;
			}
			if (!br)
				z++;
			if (z)
				(*txtg)[i].push_back(TTF_RenderText_Blended(TNRB16,(*brtext)[i].substr(0,z).c_str(),zwykly));
			(*brtext)[i].erase(0,z);
			z=0;
			if (  ((*brtext)[i].length()==0) || ((*brtext)[i][z]!='{')  )
			{
				continue;
			}
			while( ((*brtext)[i][++z]) != ('}') )
			{}
			//tyemp = (*brtext)[i].substr(1,z-1); //od 1 bo pomijamy otwierajaca klamre
			(*txtg)[i].push_back(TTF_RenderText_Blended(TNRB16,(*brtext)[i].substr(1,z-1).c_str(),tytulowy));
			(*brtext)[i].erase(0,z+1); //z+1 bo dajemy zamykajaca klamre
		} //ends while((*brtext)[i].length())
	} //ends for(int i=0; i<brtext->size();i++)
	return txtg;
}
CSimpleWindow * CMessage::genWindow(std::string text, int player, int Lmar, int Rmar, int Tmar, int Bmar)
{
	CSimpleWindow * ret = new CSimpleWindow();
	std::vector<std::string> * brtext = breakText(text,32,true,true);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(brtext);
	std::pair<int,int> txts = getMaxSizes(txtg);
	ret->bitmap = drawBox1(txts.first+Lmar+Rmar,txts.second+Tmar+Bmar,0);
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;
	int curh = ret->bitmap->h/2 - (19*txtg->size())/2;
	blitTextOnSur(txtg,curh,ret->bitmap);
	delete brtext;
	delete txtg;
	return ret;
}
std::vector< std::vector<SComponent*> > * CMessage::breakComps(std::vector<SComponent*> & comps,int maxw, SDL_Surface* _or)
{
	std::vector< std::vector<SComponent*> > * ret = new std::vector< std::vector<SComponent*> >();
	ret->resize(1);
	int rvi = 0;
	int curw = 0;
	for(int i=0;i<comps.size();i++)
	{
		curw += (comps[i]->getImg()->w + 12 + (_or ? _or->w : 0));
		if (curw > maxw)
		{
			curw = 0;
			rvi++;
			ret->resize(rvi+1);
		}
		(*ret)[rvi].push_back(comps[i]);
	}
	return ret;
}

SDL_Surface * CMessage::drawBoxTextBitmapSub( int player, std::string text, SDL_Surface* bitmap, std::string sub, int charperline/*=30*/, int imgToBmp/*=55*/ )
{
	int curh;
	std::vector<std::string> * tekst = breakText(text,charperline);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(tekst);
	std::pair<int,int> txts = getMaxSizes(txtg), boxs;
	boxs.first = std::max(txts.first,bitmap->w) // text/bitmap max width
		+ 50; //side margins
	boxs.second =
		(curh=45) //top margin
		+ txts.second //text total height
		+ imgToBmp //text <=> img
		+ bitmap->h
		+ 5 // to sibtitle
		+ (*txtg)[0][0]->h
		+ 30;
	SDL_Surface *ret = drawBox1(boxs.first,boxs.second,player);
	blitTextOnSur(txtg,curh,ret);
	curh += imgToBmp;
	blitAt(bitmap,(ret->w/2)-(bitmap->w/2),curh,ret);
	curh += bitmap->h + 5;
	CSDL_Ext::printAtMiddle(sub,ret->w/2,curh+(  ((*txtg)[0][0]->h) / 2  ),GEOR13,zwykly,ret);
	delete tekst;
	delete txtg;
	return ret;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, int player, int charperline)
{
	std::vector<std::string> * brtext = breakText(text,charperline,true,true);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(brtext);
	std::pair<int,int> txts = getMaxSizes(txtg);
	if(ret->buttons.size())
		txts.second += 20 + //before button
		ok->ourImages[0].bitmap->h; //button
	if (ret->components.size())
		txts.second += 30 //space to first component
		+ ret->components[0]->getImg()->h
		+ 5 //img <-> subtitle
		+ 20; //subtitle //TODO: check how much place will be needed for subtitle
	ret->bitmap = drawBox1(txts.first+70,txts.second+70,0);
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;
	ret->pos.x=400-(ret->pos.w/2);
	ret->pos.y=300-(ret->pos.h/2);
	int curh = 30; //gorny margines
	blitTextOnSur(txtg,curh,ret->bitmap);
	if (ret->components.size())
	{
		curh += 30;
		if(ret->components.size()==1)
		{
			blitCompsOnSur(ret->components,200,0,curh,ret->bitmap);
		}
		else
		{
			SDL_Surface * _or = 0;
			if(dynamic_cast<CSelWindow*>(ret)) //it's selection window, so we'll blit "or" between components
				_or = TTF_RenderText_Blended(GEOR13,CGI->generaltexth->allTexts[4].c_str(),zwykly);
			std::vector< std::vector<SComponent*> > * komp = breakComps(reinterpret_cast<std::vector<SComponent*>&>(ret->components),500,_or);
			blitCompsOnSur(_or,komp,10,curh,ret->bitmap);
			delete komp;
		}
	}
	if(ret->buttons.size())
	{
		curh += 20; //to buttton
		int bw = 20*(ret->buttons.size()-1); //total width of buttons - start with distance between them
		for(int i=0; i<ret->buttons.size(); i++) //and add buttons width
			bw+=ret->buttons[i]->imgs[0][0]->w; 
		bw = (ret->bitmap->w/2) - (bw/2);
		for(int i=0; i<ret->buttons.size(); i++)
		{
			ret->buttons[i]->pos.x = bw + ret->pos.x;
			ret->buttons[i]->pos.y = curh + ret->pos.y;
			bw += ret->buttons[i]->imgs[0][0]->w + 20;
		}
	}
	for(int i=0; i<ret->components.size(); i++)
	{
		ret->components[i]->pos.x += ret->pos.x;
		ret->components[i]->pos.y += ret->pos.y;
	}
	delete brtext;
	delete txtg;
}
CSelWindow * CMessage::genSelWindow(std::string text, int player, int charperline, std::vector<CSelectableComponent*> & comps, int owner)
{
	//CSelWindow * ret = new CSelWindow();
	//std::vector<std::string> * tekst = breakText(text,charperline);
	//std::vector<std::vector<SDL_Surface*> > * txtg = drawText(tekst);
	//std::pair<int,int> txts = getMaxSizes(txtg);
	//txts.first+=45; //side margins
	//int curh = 50; //top margin
	////std::pair<int,int> txts2 = getMaxSizes(komp);
	//ret->pos.h = txts.second //wys. tekstu
	//	//+ txts2.second //wys komponentow
	//	+ 20 //podpis pod komponentami
	//	+ 55 //gorny margines
	//	+ 60 //text <=> comps
	//	+ 20 //comps <=> button
	//	+ ok->ourImages[0].bitmap->h //button
	//	+ 30; //bottom margin
	//ret->pos.w = std::max(txts.first,txts.second);
	//ret->bitmap = drawBox1(ret->pos.w,ret->pos.h,player);
	//blitTextOnSur(txtg,curh,ret->bitmap);
	//curh += 50;
	//blitCompsOnSur(_or,komp,10,curh,ret->bitmap);
	//curh += 30; //to buttton
	//ret->buttons[0]->pos.x = (ret->bitmap->w/2) - (ret->buttons[0]->imgs[0][0]->w/2);
	//ret->buttons[0]->pos.y = curh;
	//ret->buttons[0]->show();
	//curh += ret->buttons[0]->imgs[0][0]->h;
	//SDL_FreeSurface(_or);
	//delete komp;
	//delete tekst;
	return NULL;
}

SDL_Surface * CMessage::genMessage
(std::string title, std::string text, EWindowType type, std::vector<CDefHandler*> *addPics, void * cb)
{
//max x 320 okolo 30 znakow
	std::vector<std::string> * tekst;
	if (text.length() < 30) //nie trzeba polamac
	{
		tekst = new std::vector<std::string>();
		tekst->push_back(text);
	}
	else tekst = breakText(text);
	int ww, hh; //wymiary boksa
	if (319>30+13*text.length())
		ww = 30+13*text.length();
	else ww = 319;
	if (title.length())
		hh=110+(21*tekst->size());
	else hh=60+(21*tekst->size());
	if (type==yesOrNO) //make place for buttons
	{
		if (ww<200) ww=200;
		hh+=70;
	}

	SDL_Surface * ret = drawBox1(ww,hh,0);
	//prepare title text

	if (title.length())
	{
		//SDL_Surface * titleText = TTF_RenderText_Shaded(TNRB16,title.c_str(),tytulowy,tlo);
		SDL_Surface * titleText = TTF_RenderText_Blended(TNRB16,title.c_str(),tytulowy);

		//draw title
		SDL_Rect tytul = genRect(titleText->h,titleText->w,((ret->w/2)-(titleText->w/2)),37);
		SDL_BlitSurface(titleText,NULL,ret,&tytul);
		SDL_FreeSurface(titleText);
	}
	//draw text
	for (int i=0; i<tekst->size(); i++)
	{
		int by = 37+i*21;
		if (title.length()) by+=40;
		//SDL_Surface * tresc = TTF_RenderText_Shaded(TNRB16,(*tekst)[i].c_str(),zwykly,tlo);
		SDL_Surface * tresc = TTF_RenderText_Blended(TNRB16,(*tekst)[i].c_str(),zwykly);
		SDL_Rect trescRect = genRect(tresc->h,tresc->w,((ret->w/2)-(tresc->w/2)),by);
		SDL_BlitSurface(tresc,NULL,ret,&trescRect);
		SDL_FreeSurface(tresc);
	}
	if (type==yesOrNO) // add buttons
	{
		int by = 77+tekst->size()*21;
		if (title.length()) by+=40;
		int hwo = (*addPics)[0]->ourImages[0].bitmap->w, hwc=(*addPics)[0]->ourImages[0].bitmap->w;
		//ok
		SDL_Rect trescRect = genRect((*addPics)[0]->ourImages[0].bitmap->h,hwo,((ret->w/2)-hwo-10),by);
		SDL_BlitSurface((*addPics)[0]->ourImages[0].bitmap,NULL,ret,&trescRect);
		((std::vector<SDL_Rect>*)(cb))->push_back(trescRect);
		//cancel
		trescRect = genRect((*addPics)[1]->ourImages[0].bitmap->h,hwc,((ret->w/2)+10),by);
		SDL_BlitSurface((*addPics)[1]->ourImages[0].bitmap,NULL,ret,&trescRect);
		((std::vector<SDL_Rect>*)(cb))->push_back(trescRect);
	}
	delete tekst;
	return ret;
}
