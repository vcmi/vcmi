// CMT.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "SDL.h"
#include "SDL_TTF.h"
#include "SDL_mixer.h"
#include "CBuildingHandler.h"
#include "SDL_Extensions.h"
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <string>
#include <assert.h>
#include <vector>
#include "zlib.h"
#include <cmath>
#include <ctime>
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "CCreatureHandler.h"
#include "CAbilityHandler.h"
#include "CSpellHandler.h"
#include "CBuildingHandler.h"
#include "CObjectHandler.h"
#include "CGameInfo.h"
#include "CMusicHandler.h"
#include "CSemiLodHandler.h"
#include "CLodHandler.h"
#include "CDefHandler.h"
#include "CSndHandler.h"
#include "CDefObjInfoHandler.h"
#include "CAmbarCendamo.h"
#include "mapHandler.h"
#include "global.h"
#include "CPreGame.h"
#include "CGeneralTextHandler.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384
const char * NAME = "VCMI 0.2";

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
SDL_Surface * ekran;
TTF_Font * TNRB16, *TNR, *GEOR13, *GEORXX;
int def(FILE *source, FILE *dest, int level, int winBits=15, int memLevel =8)
{
	int ret, flush;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit2(&strm, level,Z_DEFLATED,winBits,memLevel,0);//8-15, 1-9, 0-2
	if (ret != Z_OK)
		return ret;

	/* compress until end of file */
	do {
		strm.avail_in = fread(in, 1, CHUNK, source);
		if (ferror(source)) {
			(void)deflateEnd(&strm);
			return Z_ERRNO;
		}
		flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = in;

		/* run deflate() on input until output buffer not full, finish
		   compression if all of source has been read in */
		do {
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = deflate(&strm, flush);	/* no bad return value */
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			have = CHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
				(void)deflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);	 /* all input will be used */

		/* done when last data in file processed */
	} while (flush != Z_FINISH);
	assert(ret == Z_STREAM_END);		/* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
//int inf(FILE *source, FILE *dest, int wBits = 15)
//{
//	int ret;
//	unsigned have;
//	z_stream strm;
//	unsigned char in[CHUNK];
//	unsigned char out[CHUNK];
//
//	/* allocate inflate state */
//	strm.zalloc = Z_NULL;
//	strm.zfree = Z_NULL;
//	strm.opaque = Z_NULL;
//	strm.avail_in = 0;
//	strm.next_in = Z_NULL;
//	ret = inflateInit2(&strm, wBits);
//	if (ret != Z_OK)
//		return ret;
//
//	/* decompress until deflate stream ends or end of file */
//	do {
//		strm.avail_in = fread(in, 1, CHUNK, source);
//		if (ferror(source)) {
//			(void)inflateEnd(&strm);
//			return Z_ERRNO;
//		}
//		if (strm.avail_in == 0)
//			break;
//		strm.next_in = in;
//
//		/* run inflate() on input until output buffer not full */
//		do {
//			strm.avail_out = CHUNK;
//			strm.next_out = out;
//			ret = inflate(&strm, Z_NO_FLUSH);
//			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
//			switch (ret) {
//			case Z_NEED_DICT:
//				ret = Z_DATA_ERROR;	 /* and fall through */
//			case Z_DATA_ERROR:
//			case Z_MEM_ERROR:
//				(void)inflateEnd(&strm);
//				return ret;
//			}
//			have = CHUNK - strm.avail_out;
//			if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
//				(void)inflateEnd(&strm);
//				return Z_ERRNO;
//			}
//		} while (strm.avail_out == 0);
//
//		/* done when inflate() says it's done */
//	} while (ret != Z_STREAM_END);
//
//	/* clean up and return */
//	(void)inflateEnd(&strm);
//	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
//}

/* report a zlib or i/o error */
void zerr(int ret)
{
	fputs("zpipe: ", stderr);
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			fputs("error reading stdin\n", stderr);
		if (ferror(stdout))
			fputs("error writing stdout\n", stderr);
		break;
	case Z_STREAM_ERROR:
		fputs("invalid compression level\n", stderr);
		break;
	case Z_DATA_ERROR:
		fputs("invalid or incomplete deflate data\n", stderr);
		break;
	case Z_MEM_ERROR:
		fputs("out of memory\n", stderr);
		break;
	case Z_VERSION_ERROR:
		fputs("zlib version mismatch!\n", stderr);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{ 
	THC timeHandler tmh;
	THC tmh.getDif();
	int xx=0, yy=0, zz=0;
	SDL_Event sEvent;
	srand ( time(NULL) );
	SDL_Surface *screen, *temp;
	std::vector<SDL_Surface*> Sprites;
	float i;
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_AUDIO/*|SDL_INIT_EVENTTHREAD*/)==0)
	{
		CPG=NULL;
		TTF_Init();
		atexit(TTF_Quit);
		atexit(SDL_Quit);
		//TNRB = TTF_OpenFont("Fonts\\tnrb.ttf",16);
		TNRB16 = TTF_OpenFont("Fonts\\tnrb.ttf",16);
		//TNR = TTF_OpenFont("Fonts\\tnr.ttf",10);
		GEOR13 = TTF_OpenFont("Fonts\\georgia.ttf",13);
		GEORXX = TTF_OpenFont("Fonts\\tnrb.ttf",22);

		//initializing audio
		CMusicHandler * mush = new CMusicHandler;
		mush->initMusics();
		//CSndHandler snd("Heroes3.snd");
		//snd.extract("AELMMOVE.wav","snddd.wav");
		//audio initialized

		/*if(Mix_PlayMusic(mush->mainMenuWoG, -1)==-1) //uncomment this fragment to have music
		{
			printf("Mix_PlayMusic: %s\n", Mix_GetError());
			// well, there's no music, but most games don't break without music...
		}*/

		screen = SDL_SetVideoMode(800,600,24,SDL_HWSURFACE|SDL_DOUBLEBUF/*|SDL_FULLSCREEN*/);
		ekran = screen;
		//FILE * zr = fopen("mal.txt","r");
		//FILE * ko = fopen("wyn.txt","w");
		//FILE * kodd = fopen("kod.txt","r");
		//FILE * deko = fopen("dekod.txt","w");
		//def(zr,ko,1);
		//inf(kodd, deko);
		//fclose(ko);fclose(zr);
		//for (int i=0;i<=20;i++)
		//{
		//	zr = fopen("kod2.txt","r");
		//	char c [200];
		//	sprintf(c,"wyn%d.txt",i);
		//	ko = fopen(c,"w");
		//	def(zr,ko,i);
		//	fclose(ko);fclose(zr);
		//}
		SDL_WM_SetCaption(NAME,""); //set window title
		CGameInfo * cgi = new CGameInfo; //contains all global informations about game (texts, lodHandlers, map handler itp.)
		CGameInfo::mainObj = cgi;
		cgi->mush = mush;
		THC std::cout<<"Initializing screen, fonts and sound handling: "<<tmh.getDif()<<std::endl;
		cgi->spriteh = new CLodHandler;
		cgi->spriteh->init(std::string("newH3sprite.lod"));
		cgi->bitmaph = new CLodHandler;
		cgi->bitmaph->init(std::string("newH3bitmap.lod"));
		THC std::cout<<"Loading .lods: "<<tmh.getDif()<<std::endl;
		CPreGame * cpg = new CPreGame(); //main menu and submenus
		THC std::cout<<"Initialization CPreGame (together): "<<tmh.getDif()<<std::endl;
		cpg->mush = mush;
		cpg->runLoop();
		THC tmh.getDif();

		//////////////////////////////////////////////////////////////////////////////// lod testing

		//CLodHandler * clod = new CLodHandler;
		//clod->loadLod("h3abp_bm.lod");

		//CLodHandler * test = new CLodHandler;
		//test->init(std::string("h3abp_bm.lod"));

		//CDefHandler * tdef = new CDefHandler;
		//tdef->openDef(std::string("newh3sprite\\AVLSPTR3.DEF"));
		//tdef->getSprite(0);

		//CLodHandler * bitmapLod = new CLodHandler;
		//bitmapLod->init(std::string("newH3bitmap.lod"));

		//CPCXConv * tconv = new CPCXConv;
		//tconv->fromFile(std::string("newh3bitmap\\ADOPBPNL.PCX"));
		//tconv->convert();
		//tconv->saveBMP(std::string("tesciczekConva.bmp"));

		//CSemiDefHandler * semek = new CSemiDefHandler;
		//semek->openDef(std::string("EDG.DEF"), std::string("H3sprite.lod"));

		//////////////////////////////////////////////////////////////////////////////// lod testing end

		cgi->sspriteh = new CSemiLodHandler();
		cgi->sspriteh->openLod("H3sprite.lod");
		CArtHandler * arth = new CArtHandler;
		arth->loadArtifacts();
		cgi->arth = arth;
		CCreatureHandler * creh = new CCreatureHandler;
		creh->loadCreatures();
		cgi->creh = creh;
		CAbilityHandler * abilh = new CAbilityHandler;
		abilh->loadAbilities();
		cgi->abilh = abilh;
		CHeroHandler * heroh = new CHeroHandler;
		heroh->loadHeroes();
		cgi->heroh = heroh;
		CSpellHandler * spellh = new CSpellHandler;
		spellh->loadSpells();
		cgi->spellh = spellh;
		CBuildingHandler * buildh = new CBuildingHandler;
		buildh->loadBuildings();
		cgi->buildh = buildh;
		CObjectHandler * objh = new CObjectHandler;
		objh->loadObjects();
		cgi->objh = objh;
		cgi->generaltexth = new CGeneralTextHandler;
		cgi->generaltexth->load();
		cgi->dobjinfo = new CDefObjInfoHandler;
		cgi->dobjinfo->load();

		THC std::cout<<"Handlers initailization: "<<tmh.getDif()<<std::endl;
		std::string mapname;
		CPG->ourScenSel->mapsel.selected = 1;
		if (CPG) mapname = CPG->ourScenSel->mapsel.ourMaps[CPG->ourScenSel->mapsel.selected].filename;
		gzFile map = gzopen(mapname.c_str(),"rb");
		std::string mapstr;int pom;
		while((pom=gzgetc(map))>=0)
		{
			mapstr+=pom;
		}
		CAmbarCendamo * ac = new CAmbarCendamo((unsigned char*)mapstr.c_str()); //4gryf
		//CAmbarCendamo * ac = new CAmbarCendamo("4gryf"); //4gryf
		CMapHeader * mmhh = new CMapHeader(ac->bufor); //czytanie nag³ówka
		cgi->ac = ac;
		THC std::cout<<"Reading file: "<<tmh.getDif()<<std::endl;
		ac->deh3m();
		THC std::cout<<"Detecting file (together): "<<tmh.getDif()<<std::endl;
		ac->loadDefs();
		THC std::cout<<"Reading terrain defs: "<<tmh.getDif()<<std::endl;
		CMapHandler * mh = new CMapHandler();
		mh->reader = ac;
		THC std::cout<<"Creating mapHandler: "<<tmh.getDif()<<std::endl;
		mh->init();
		THC std::cout<<"Initializing mapHandler: "<<tmh.getDif()<<std::endl;
		//SDL_Rect * sr = new SDL_Rect(); sr->h=64;sr->w=64;sr->x=0;sr->y=0;
		SDL_Surface * teren = mh->terrainRect(xx,yy,25,19);
		THC std::cout<<"Preparing terrain to display: "<<tmh.getDif()<<std::endl;
		SDL_BlitSurface(teren,NULL,ekran,NULL);
		SDL_FreeSurface(teren);
		SDL_UpdateRect(ekran, 0, 0, ekran->w, ekran->h);
		THC std::cout<<"Displaying terrain: "<<tmh.getDif()<<std::endl;

		//SDL_Surface * ss = ac->defs[0]->ourImages[0].bitmap;
		//SDL_BlitSurface(ss, NULL, ekran, NULL);

		bool scrollingLeft = false;
		bool scrollingRight = false;
		bool scrollingUp = false;
		bool scrollingDown = false;
		bool updateScreen = false;
		unsigned char animVal=0; //for animations handling
		for(;;) // main loop
		{
			try
			{
				if(SDL_PollEvent(&sEvent))  //wait for event...
				{
					if(sEvent.type==SDL_QUIT) 
						return 0;
					else if (sEvent.type==SDL_KEYDOWN)
					{
						switch (sEvent.key.keysym.sym)
						{
						case SDLK_LEFT:
							{
								scrollingLeft = true;
								break;
							}
						case (SDLK_RIGHT):
							{
								scrollingRight = true;
								break;
							}
						case (SDLK_UP):
							{
								scrollingUp = true;
								break;
							}
						case (SDLK_DOWN):
							{
								scrollingDown = true;
								break;
							}
						case (SDLK_q):
							{
								return 0;
								break;
							}
						case (SDLK_u):
							{
								if(!ac->map.twoLevel)
									break;
								if (zz)
									zz--;
								else zz++;
								updateScreen = true;
								break;
							}
						}
					} //keydown end
					else if(sEvent.type==SDL_KEYUP) 
					{
						switch (sEvent.key.keysym.sym)
						{
						case SDLK_LEFT:
							{
								scrollingLeft = false;
								break;
							}
						case (SDLK_RIGHT):
							{
								scrollingRight = false;
								break;
							}
						case (SDLK_UP):
							{
								scrollingUp = false;
								break;
							}
						case (SDLK_DOWN):
							{
								scrollingDown = false;
								break;
							}
						}
					}//keyup end
					else if(sEvent.type==SDL_MOUSEMOTION)
					{
						if(sEvent.motion.x<5)
						{
							scrollingLeft = true;
						}
						else
						{
							scrollingLeft = false;
						}
						if(sEvent.motion.x>screen->w-5)
						{
							scrollingRight = true;
						}
						else
						{
							scrollingRight = false;
						}
						if(sEvent.motion.y<5)
						{
							scrollingUp = true;
						}
						else
						{
							scrollingUp = false;
						}
						if(sEvent.motion.y>screen->h-5)
						{
							scrollingDown = true;
						}
						else
						{
							scrollingDown = false;
						}
					}
				} //event end

				/////////////// scrolling terrain
				if(scrollingLeft)
				{
					if(xx>0)
					{
						xx--;
						updateScreen = true;
					}
				}
				if(scrollingRight)
				{
					if(xx<ac->map.width-25+8)
					{
						xx++;
						updateScreen = true;
					}
				}
				if(scrollingUp)
				{
					if(yy>0)
					{
						yy--;
						updateScreen = true;
					}
				}
				if(scrollingDown)
				{
					if(yy<ac->map.height-19+8)
					{
						yy++;
						updateScreen = true;
					}
				}
				if(updateScreen)
				{
					SDL_FillRect(ekran, NULL, SDL_MapRGB(ekran->format, 0, 0, 0));
					SDL_Surface * help = mh->terrainRect(xx,yy,25,19,zz,animVal);
					SDL_BlitSurface(help,NULL,ekran,NULL);
					SDL_FreeSurface(help);
					SDL_UpdateRect(ekran, 0, 0, ekran->w, ekran->h);
					updateScreen = false;
				}
				/////////
			}
			catch(...)
			{ continue; }

			updateScreen = true;
			++animVal; //for animations

			SDL_Delay(30); //give time for other apps
		}
		return 0;
	}
	else
	{
		printf("Something was wrong: %s/n", SDL_GetError());
		return -1;
	}
}
