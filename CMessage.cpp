#include "stdafx.h"
#include "CMessage.h"
#include "SDL_TTF.h"
#include "hch\CSemiDefHandler.h"
#include "hch\CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "hch\CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "CPlayerInterface.h"
#include "hch\CDefHandler.h"
#include "hch\CSemiDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include <sstream>
#include "CLua.h"
SDL_Color tytulowy, tlo, zwykly ;
SDL_Rect genRect(int hh, int ww, int xx, int yy);

extern SDL_Surface * ekran;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Color genRGB(int r, int g, int b, int a=0);
//void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2);
bool isItIn(const SDL_Rect * rect, int x, int y);

using namespace NMessage;


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
			CDefHandler * bluePieces = CGI->spriteh->giveDef("DIALGBOX.DEF");
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
				CSDL_Ext::blueToPlayersAdv(bluePieces->ourImages[j].bitmap,i);
				piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
			}
		}
		NMessage::background = CGI->bitmaph->loadBitmap("DIBOXBCK.BMP");
		SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	}
	ok = CGI->spriteh->giveDef("IOKAY.DEF");
	cancel = CGI->spriteh->giveDef("ICANCEL.DEF");
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
SDL_Surface * CMessage::drawBox1(int w, int h, int playerColor)
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(ekran->flags, w, h, ekran->format->BitsPerPixel, ekran->format->Rmask, ekran->format->Gmask, ekran->format->Bmask, ekran->format->Amask);
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
	return ret;
}
SDL_Surface * CMessage::blitCompsOnSur(std::vector<SComponent*> & comps, int maxw, int inter, int & curh, SDL_Surface * ret)
{
	std::vector<std::string> * brdtext;
	if (comps.size())
		brdtext = breakText(comps[0]->subtitle,12,true,true);
	else 
		brdtext = NULL;
	curh += 30;
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
	for (int i=0; i<txtg->size();i++)
	{
		int lw=0;
		for (int j=0;j<(*txtg)[i].size();j++)
			lw+=(*txtg)[i][j]->w;
		int pw = ret->bitmap->w/2, ph =  ret->bitmap->h/2;
		//int pw = Tmar, ph = Lmar;
		pw -= lw/2;
		ph -= (19*txtg->size())/2;

		int tw = pw;
		for (int j=0;j<(*txtg)[i].size();j++)
		{
				//std::stringstream n;
				//n <<"temp_"<<i<<"__"<<j<<".bmp";
			blitAt((*txtg)[i][j],tw,ph+i*19,ret->bitmap);
				//SDL_SaveBMP(ret->bitmap,n.str().c_str());	
			tw+=(*txtg)[i][j]->w;
			SDL_FreeSurface((*txtg)[i][j]);
		}
	}
	return ret;
}

CInfoWindow * CMessage::genIWindow(std::string text, int player, int charperline, std::vector<SComponent*> & comps)
{
	//TODO: support for more than one component
	CInfoWindow * ret = new CInfoWindow();
	ret->components = comps;
	std::vector<std::string> * brtext = breakText(text,charperline,true,true);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(brtext);
	std::pair<int,int> txts = getMaxSizes(txtg);
	txts.second = txts.second
		+ ok->ourImages[0].bitmap->h //button
		+ 15 //after button
		+ 20; // space between subtitle and button
	if (comps.size())
			txts.second = txts.second
			+ 30 //space to first component
			+ comps[0]->getImg()->h
			+ 5 //img <-> subtitle
			+ 20; //subtitle //!!!!!!!!!!!!!!!!!!!!
	ret->bitmap = drawBox1(txts.first+70,txts.second+70,0); 
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;

	int curh = 30; //gorny margines
	blitTextOnSur(txtg,curh,ret->bitmap);
	curh += (19 * txtg->size()); //wys. tekstu

	if (comps.size())
	{
		blitCompsOnSur(comps,200,0,curh,ret->bitmap);
	}
	curh += 20; //to buttton

	ret->okb.posr.x = (ret->bitmap->w/2) - (ret->okb.imgs[0][0]->w/2);
	ret->okb.posr.y = curh;
	ret->okb.show();
	curh+=ret->okb.imgs[0][0]->h;

	return ret;
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
	if (type==EWindowType::yesOrNO) //make place for buttons
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
	if (type==EWindowType::yesOrNO) // add buttons
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