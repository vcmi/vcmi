#include "../stdafx.h"
#include "Graphics.h"
#include "../hch/CDefHandler.h"
#include "../hch/CObjectHandler.h"
#include "../SDL_Extensions.h"
using namespace CSDL_Ext;
Graphics * graphics = NULL;
SDL_Surface * Graphics::drawPrimarySkill(const CGHeroInstance *curh, SDL_Surface *ret, int from, int to)
{
	char * buf = new char[10];
	for (int i=from;i<to;i++)
	{
		itoa(curh->primSkills[i],buf,10);
		printAtMiddle(buf,84+28*i,68,GEOR13,zwykly,ret);
	}
	delete[] buf;
	return ret;
}
SDL_Surface * Graphics::drawHeroInfoWin(const CGHeroInstance * curh)
{
	char * buf = new char[10];
	SDL_Surface * ret = SDL_DisplayFormat(hInfo);
	blueToPlayersAdv(hInfo,curh->tempOwner,1);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);
	drawPrimarySkill(curh, ret);
	for (std::map<int,std::pair<CCreature*,int> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		blitAt(graphics->smallImgs[(*i).second.first->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}
	blitAt(graphics->portraitLarge[curh->subID],11,12,ret);
	itoa(curh->mana,buf,10);
	printAtMiddle(buf,166,109,GEORM,zwykly,ret); //mana points
	delete[] buf;
	blitAt(morale22->ourImages[curh->getCurrentMorale()+3].bitmap,14,84,ret);
	blitAt(luck22->ourImages[curh->getCurrentLuck()+3].bitmap,14,101,ret);
	//SDL_SaveBMP(ret,"inf1.bmp");
	return ret;
}
SDL_Surface * Graphics::drawTownInfoWin(const CGTownInstance * curh)
{
	char * buf = new char[10];
	blueToPlayersAdv(tInfo,curh->tempOwner,1);
	SDL_Surface * ret = SDL_DisplayFormat(tInfo);
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	printAt(curh->name,75,15,GEOR13,zwykly,ret);

	int pom = curh->fortLevel() - 1; if(pom<0) pom = 3;
	blitAt(forts->ourImages[pom].bitmap,115,42,ret);
	if((pom=curh->hallLevel())>=0)
		blitAt(halls->ourImages[pom].bitmap,77,42,ret);
	itoa(curh->dailyIncome(),buf,10);
	printAtMiddle(buf,167,70,GEORM,zwykly,ret);
	for (std::map<int,std::pair<CCreature*,int> >::const_iterator i=curh->army.slots.begin(); i!=curh->army.slots.end();i++)
	{
		if(!i->second.first)
			continue;
		blitAt(graphics->smallImgs[(*i).second.first->idNumber],slotsPos[(*i).first].first+1,slotsPos[(*i).first].second+1,ret);
		itoa((*i).second.second,buf,10);
		printAtMiddle(buf,slotsPos[(*i).first].first+17,slotsPos[(*i).first].second+39,GEORM,zwykly,ret);
	}

	//blit town icon
	pom = curh->subID*2;
	if (!curh->hasFort())
		pom += F_NUMBER*2;
	if(curh->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(bigTownPic->ourImages[pom].bitmap,13,13,ret);
	delete[] buf;
	return ret;
}
Graphics::Graphics()
{
	artDefs = CDefHandler::giveDef("ARTIFACT.DEF");
	hInfo = BitmapHandler::loadBitmap("HEROQVBK.bmp");
	SDL_SetColorKey(hInfo,SDL_SRCCOLORKEY,SDL_MapRGB(hInfo->format,0,255,255));
	tInfo = BitmapHandler::loadBitmap("TOWNQVBK.bmp");
	SDL_SetColorKey(tInfo,SDL_SRCCOLORKEY,SDL_MapRGB(tInfo->format,0,255,255));
	slotsPos.push_back(std::pair<int,int>(44,82));
	slotsPos.push_back(std::pair<int,int>(80,82));
	slotsPos.push_back(std::pair<int,int>(116,82));
	slotsPos.push_back(std::pair<int,int>(26,131));
	slotsPos.push_back(std::pair<int,int>(62,131));
	slotsPos.push_back(std::pair<int,int>(98,131));
	slotsPos.push_back(std::pair<int,int>(134,131));

	luck22 = CDefHandler::giveDefEss("ILCK22.DEF");
	luck30 = CDefHandler::giveDefEss("ILCK30.DEF");
	luck42 = CDefHandler::giveDefEss("ILCK42.DEF");
	luck82 = CDefHandler::giveDefEss("ILCK82.DEF");
	morale22 = CDefHandler::giveDefEss("IMRL22.DEF");
	morale30 = CDefHandler::giveDefEss("IMRL30.DEF");
	morale42 = CDefHandler::giveDefEss("IMRL42.DEF");
	morale82 = CDefHandler::giveDefEss("IMRL82.DEF");
	halls = CDefHandler::giveDefEss("ITMTLS.DEF");
	forts = CDefHandler::giveDefEss("ITMCLS.DEF");
	bigTownPic =  CDefHandler::giveDefEss("ITPT.DEF");
	std::ifstream ifs;
	ifs.open("config/cr_bgs.txt"); 
	while(!ifs.eof())
	{
		int id;
		std::string name;
		ifs >> id >> name;
		backgrounds[id]=BitmapHandler::loadBitmap(name);
	}
	ifs.close();
	ifs.clear();

	//loading 32x32px imgs
	CDefHandler *smi = CDefHandler::giveDef("CPRSMALL.DEF");
	smi->notFreeImgs = true;
	for (int i=0; i<smi->ourImages.size(); i++)
	{
		smallImgs[i-2] = smi->ourImages[i].bitmap;
	}
	delete smi;
	smi = CDefHandler::giveDef("TWCRPORT.DEF");
	smi->notFreeImgs = true;
	for (int i=0; i<smi->ourImages.size(); i++)
	{
		bigImgs[i-2] = smi->ourImages[i].bitmap;
	}
	delete smi;

	std::ifstream of("config/portrety.txt");
	for (int j=0;j<HEROES_QUANTITY;j++)
	{
		int ID;
		of>>ID;
		std::string path;
		of>>path;
		portraitSmall.push_back(BitmapHandler::loadBitmap(path));
		//if (!heroes[ID]->portraitSmall)
		//	std::cout<<"Can't read small portrait for "<<ID<<" ("<<path<<")\n";
		for(int ff=0; ff<path.size(); ++ff) //size letter is usually third one, but there are exceptions an it should fix the problem
		{
			if(path[ff]=='S')
			{
				path[ff]='L';
				break;
			}
		}
		portraitLarge.push_back(BitmapHandler::loadBitmap(path));
		//if (!heroes[ID]->portraitLarge)
		//	std::cout<<"Can't read large portrait for "<<ID<<" ("<<path<<")\n";	
		SDL_SetColorKey(portraitLarge[portraitLarge.size()-1],SDL_SRCCOLORKEY,SDL_MapRGB(portraitLarge[portraitLarge.size()-1]->format,0,255,255));

	}
	of.close();
	pskillsb = CDefHandler::giveDef("PSKILL.DEF");
	resources = CDefHandler::giveDef("RESOUR82.DEF");
	un44 = CDefHandler::giveDef("UN44.DEF");
	smallIcons = CDefHandler::giveDef("ITPA.DEF");
	resources32 = CDefHandler::giveDef("RESOURCE.DEF");
	loadHeroFlags();



		//std::stringstream nm;
		//nm<<"AH";
		//nm<<std::setw(2);
		//nm<<std::setfill('0');
		//nm<<heroClasses.size();
		//nm<<"_.DEF";
		//hc->moveAnim = CDefHandler::giveDef(nm.str());

		//for(int o=0; o<hc->moveAnim->ourImages.size(); ++o)
		//{
		//	if(hc->moveAnim->ourImages[o].groupNumber==6)
		//	{
		//		for(int e=0; e<8; ++e)
		//		{
		//			Cimage nci;
		//			nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
		//			nci.groupNumber = 10;
		//			nci.imName = std::string();
		//			hc->moveAnim->ourImages.push_back(nci);
		//		}
		//		o+=8;
		//	}
		//	if(hc->moveAnim->ourImages[o].groupNumber==7)
		//	{
		//		for(int e=0; e<8; ++e)
		//		{
		//			Cimage nci;
		//			nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
		//			nci.groupNumber = 11;
		//			nci.imName = std::string();
		//			hc->moveAnim->ourImages.push_back(nci);
		//		}
		//		o+=8;
		//	}
		//	if(hc->moveAnim->ourImages[o].groupNumber==8)
		//	{
		//		for(int e=0; e<8; ++e)
		//		{
		//			Cimage nci;
		//			nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o+e].bitmap);
		//			nci.groupNumber = 12;
		//			nci.imName = std::string();
		//			hc->moveAnim->ourImages.push_back(nci);
		//		}
		//		o+=8;
		//	}
		//}
		//for(int o=0; o<hc->moveAnim->ourImages.size(); ++o)
		//{
		//	if(hc->moveAnim->ourImages[o].groupNumber==1)
		//	{
		//		Cimage nci;
		//		nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
		//		nci.groupNumber = 13;
		//		nci.imName = std::string();
		//		hc->moveAnim->ourImages.push_back(nci);
		//		//o+=1;
		//	}
		//	if(hc->moveAnim->ourImages[o].groupNumber==2)
		//	{
		//		Cimage nci;
		//		nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
		//		nci.groupNumber = 14;
		//		nci.imName = std::string();
		//		hc->moveAnim->ourImages.push_back(nci);
		//		//o+=1;
		//	}
		//	if(hc->moveAnim->ourImages[o].groupNumber==3)
		//	{
		//		Cimage nci;
		//		nci.bitmap = CSDL_Ext::rotate01(hc->moveAnim->ourImages[o].bitmap);
		//		nci.groupNumber = 15;
		//		nci.imName = std::string();
		//		hc->moveAnim->ourImages.push_back(nci);
		//		//o+=1;
		//	}
		//}

		//for(int ff=0; ff<hc->moveAnim->ourImages.size(); ++ff)
		//{
		//	CSDL_Ext::alphaTransform(hc->moveAnim->ourImages[ff].bitmap);
		//}
		//hc->moveAnim->alphaTransformed = true;
}
void Graphics::loadHeroFlags()
{
	flags1.push_back(CDefHandler::giveDef("ABF01L.DEF")); //red
	flags1.push_back(CDefHandler::giveDef("ABF01G.DEF")); //blue
	flags1.push_back(CDefHandler::giveDef("ABF01R.DEF")); //tan
	flags1.push_back(CDefHandler::giveDef("ABF01D.DEF")); //green
	flags1.push_back(CDefHandler::giveDef("ABF01B.DEF")); //orange
	flags1.push_back(CDefHandler::giveDef("ABF01P.DEF")); //purple
	flags1.push_back(CDefHandler::giveDef("ABF01W.DEF")); //teal
	flags1.push_back(CDefHandler::giveDef("ABF01K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags1[q]->ourImages.size(); ++o)
		{
			if(flags1[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags1[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags1[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags1[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags1[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags1[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags1[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags1[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags1[q]->alphaTransformed = true;
	}

	flags2.push_back(CDefHandler::giveDef("ABF02L.DEF")); //red
	flags2.push_back(CDefHandler::giveDef("ABF02G.DEF")); //blue
	flags2.push_back(CDefHandler::giveDef("ABF02R.DEF")); //tan
	flags2.push_back(CDefHandler::giveDef("ABF02D.DEF")); //green
	flags2.push_back(CDefHandler::giveDef("ABF02B.DEF")); //orange
	flags2.push_back(CDefHandler::giveDef("ABF02P.DEF")); //purple
	flags2.push_back(CDefHandler::giveDef("ABF02W.DEF")); //teal
	flags2.push_back(CDefHandler::giveDef("ABF02K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags2[q]->ourImages.size(); ++o)
		{
			if(flags2[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags2[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags2[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags2[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags2[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags2[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags2[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags2[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags2[q]->alphaTransformed = true;
	}

	flags3.push_back(CDefHandler::giveDef("ABF03L.DEF")); //red
	flags3.push_back(CDefHandler::giveDef("ABF03G.DEF")); //blue
	flags3.push_back(CDefHandler::giveDef("ABF03R.DEF")); //tan
	flags3.push_back(CDefHandler::giveDef("ABF03D.DEF")); //green
	flags3.push_back(CDefHandler::giveDef("ABF03B.DEF")); //orange
	flags3.push_back(CDefHandler::giveDef("ABF03P.DEF")); //purple
	flags3.push_back(CDefHandler::giveDef("ABF03W.DEF")); //teal
	flags3.push_back(CDefHandler::giveDef("ABF03K.DEF")); //pink

	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags3[q]->ourImages.size(); ++o)
		{
			if(flags3[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags3[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags3[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags3[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags3[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags3[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags3[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags3[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags3[q]->alphaTransformed = true;
	}

	flags4.push_back(CDefHandler::giveDef("AF00.DEF")); //red
	flags4.push_back(CDefHandler::giveDef("AF01.DEF")); //blue
	flags4.push_back(CDefHandler::giveDef("AF02.DEF")); //tan
	flags4.push_back(CDefHandler::giveDef("AF03.DEF")); //green
	flags4.push_back(CDefHandler::giveDef("AF04.DEF")); //orange
	flags4.push_back(CDefHandler::giveDef("AF05.DEF")); //purple
	flags4.push_back(CDefHandler::giveDef("AF06.DEF")); //teal
	flags4.push_back(CDefHandler::giveDef("AF07.DEF")); //pink


	for(int q=0; q<8; ++q)
	{
		for(int o=0; o<flags4[q]->ourImages.size(); ++o)
		{
			if(flags4[q]->ourImages[o].groupNumber==6)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==7)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 11;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==8)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 12;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int o=0; o<flags4[q]->ourImages.size(); ++o)
		{
			if(flags4[q]->ourImages[o].groupNumber==1)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 13;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==2)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 14;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
			if(flags4[q]->ourImages[o].groupNumber==3)
			{
				for(int e=0; e<8; ++e)
				{
					Cimage nci;
					nci.bitmap = CSDL_Ext::rotate01(flags4[q]->ourImages[o+e].bitmap);
					nci.groupNumber = 10;
					nci.groupNumber = 15;
					nci.imName = std::string();
					flags4[q]->ourImages.push_back(nci);
				}
				o+=8;
			}
		}

		for(int ff=0; ff<flags4[q]->ourImages.size(); ++ff)
		{
			SDL_SetColorKey(flags4[q]->ourImages[ff].bitmap, SDL_SRCCOLORKEY,
				SDL_MapRGB(flags4[q]->ourImages[ff].bitmap->format, 0, 255, 255)
				);
		}
		flags4[q]->alphaTransformed = true;
	}
}
SDL_Surface * Graphics::getPic(int ID, bool fort, bool builded)
{
	if (ID==-1)
		return smallIcons->ourImages[0].bitmap;
	else if (ID==-2)
		return smallIcons->ourImages[1].bitmap;
	else if (ID==-3)
		return smallIcons->ourImages[2+F_NUMBER*4].bitmap;
	else if (ID>F_NUMBER || ID<-3)
		throw new std::exception("Invalid ID");
	else
	{
		int pom = 3;
		if(!fort)
			pom+=F_NUMBER*2;
		pom += ID*2;
		if (!builded)
			pom--;
		return smallIcons->ourImages[pom].bitmap;
	}
}