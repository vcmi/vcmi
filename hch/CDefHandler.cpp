#include "../stdafx.h"
#include "SDL.h"
#include "CDefHandler.h"
#include <sstream>
#include "CLodHandler.h"
CLodHandler* CDefHandler::Spriteh = NULL;
long long pow(long long a, int b)
{
	if (!b) return 1;
	long c = a;
	while (--b)
		a*=c;
	return a;
}
CDefHandler::CDefHandler()
{
	//FDef = NULL;
	RWEntries = NULL;
	notFreeImgs = false;
}
CDefHandler::~CDefHandler()
{
	//if (FDef)
		//delete [] FDef;
	if (RWEntries)
		delete [] RWEntries;
	if (notFreeImgs)
		return;
	for (int i=0; i<ourImages.size(); i++)
	{
		if (ourImages[i].bitmap)
		{
			SDL_FreeSurface(ourImages[i].bitmap);
			ourImages[i].bitmap=NULL;
		}
	}
}
CDefEssential::~CDefEssential()
{
	for(int i=0;i<ourImages.size();i++)
		SDL_FreeSurface(ourImages[i].bitmap);
}
void CDefHandler::openDef(std::string name)
{
	int i,j, totalInBlock;
	char Buffer[13];
	BMPPalette palette[256];
	defName=name;

	int andame;
	std::ifstream * is = new std::ifstream();
	is -> open(name.c_str(),std::ios::binary);
	is->seekg(0,std::ios::end); // na koniec
	andame = is->tellg();  // read length
	is->seekg(0,std::ios::beg); // wracamy na poczatek
	unsigned char * FDef = new unsigned char[andame]; // allocate memory 
	is->read((char*)FDef, andame); // read map file to buffer
	is->close();
	delete is;
	i = 0;
	DEFType = readNormalNr(i,4,FDef); i+=4;
	fullWidth = readNormalNr(i,4,FDef); i+=4;
	fullHeight = readNormalNr(i,4,FDef); i+=4;
	i=0xc;
	totalBlocks = readNormalNr(i,4,FDef); i+=4;

	i=0x10;
	for (int it=0;it<256;it++)
	{
		palette[it].R = FDef[i++];
		palette[it].G = FDef[i++];
		palette[it].B = FDef[i++];
		palette[it].F = 0;
	}
	i=0x310;
	totalEntries=0;
	for (int z=0; z<totalBlocks; z++)
	{
		i+=4;
		totalInBlock = readNormalNr(i,4,FDef); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
			SEntries.push_back(SEntry());
		i+=8;
		for (j=0; j<totalInBlock; j++)
		{
			for (int k=0;k<13;k++) Buffer[k]=FDef[i+k]; 
			i+=13;
			SEntries[totalEntries+j].name=Buffer;
		}
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = readNormalNr(i,4,FDef);
			i+=4;
		}
		//totalEntries+=totalInBlock;
		for(int hh=0; hh<totalInBlock; ++hh)
		{
			SEntries[totalEntries].group = z;
			++totalEntries;
		}
	}
	for(j=0; j<SEntries.size(); ++j)
	{
		SEntries[j].name = SEntries[j].name.substr(0, SEntries[j].name.find('.')+4);
	}
	for(int i=0; i<SEntries.size(); ++i)
	{
		Cimage nimg;
		nimg.bitmap = getSprite(i, FDef, palette);
		nimg.imName = SEntries[i].name;
		nimg.groupNumber = SEntries[i].group;
		ourImages.push_back(nimg);
	}
	delete [] FDef;
	FDef = NULL;
}

void CDefHandler::openFromMemory(unsigned char *table, std::string name)
{
	int i,j, totalInBlock;
	char Buffer[13];
	BMPPalette palette[256];
	defName=name;
	i = 0;
	DEFType = readNormalNr(i,4,table); i+=4;
	fullWidth = readNormalNr(i,4,table); i+=4;
	fullHeight = readNormalNr(i,4,table); i+=4;
	i=0xc;
	totalBlocks = readNormalNr(i,4,table); i+=4;

	i=0x10;
	for (int it=0;it<256;it++)
	{
		palette[it].R = table[i++];
		palette[it].G = table[i++];
		palette[it].B = table[i++];
		palette[it].F = 0;
	}
	i=0x310;
	totalEntries=0;
	for (int z=0; z<totalBlocks; z++)
	{
		int unknown1 = readNormalNr(i,4,table); i+=4;
		totalInBlock = readNormalNr(i,4,table); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
			SEntries.push_back(SEntry());
		int unknown2 = readNormalNr(i,4,table); i+=4;
		int unknown3 = readNormalNr(i,4,table); i+=4;
		for (j=0; j<totalInBlock; j++)
		{
			for (int k=0;k<13;k++) Buffer[k]=table[i+k]; 
			i+=13;
			SEntries[totalEntries+j].name=Buffer;
		}
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = readNormalNr(i,4,table);
			int unknown4 = readNormalNr(i,4,table); i+=4;
		}
		//totalEntries+=totalInBlock;
		for(int hh=0; hh<totalInBlock; ++hh)
		{
			SEntries[totalEntries].group = z;
			++totalEntries;
		}
	}
	for(j=0; j<SEntries.size(); ++j)
	{
		SEntries[j].name = SEntries[j].name.substr(0, SEntries[j].name.find('.')+4);
	}
	RWEntries = new unsigned int[fullHeight];
	for(int i=0; i<SEntries.size(); ++i)
	{
		Cimage nimg;
		nimg.bitmap = getSprite(i, table, palette);
		nimg.imName = SEntries[i].name;
		nimg.groupNumber = SEntries[i].group;
		ourImages.push_back(nimg);
	}
}

unsigned char * CDefHandler::writeNormalNr (int nr, int bytCon)
{
	//int tralalalatoniedziala = 2*9+100-4*bytCon;
	//unsigned char * ret = new unsigned char[bytCon];
	unsigned char * ret = NULL;
	for(int jj=0; jj<100; ++jj)
	{
		ret = (unsigned char*)calloc(1, bytCon);
		if(ret!=NULL)
			break;
	}
	long long amp = pow((long long int)256,bytCon-1);
	for (int i=bytCon-1; i>=0;i--)
	{
		int test2 = nr/(amp);
		ret[i]=test2;
		nr -= (nr/(amp))*amp;
		amp/=256;
	}
	return ret;
}
void CDefHandler::expand(unsigned char N,unsigned char & BL, unsigned char & BR)
{
	BL = (N & 0xE0) >> 5;
	BR = N & 0x1F;
}
int CDefHandler::readNormalNr (int pos, int bytCon, unsigned char * str, bool cyclic)
{
	int ret=0;
	int amp=1;
	if (str)
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp*=256;
		}
	}
	//else 
	//{
	//	for (int i=0; i<bytCon; i++)
	//	{
	//		ret+=FDef[pos+i]*amp;
	//		amp*=256;
	//	}
	//}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}
void CDefHandler::print (std::ostream & stream, int nr, int bytcon)
{
	unsigned char * temp = writeNormalNr(nr,bytcon);
	for (int i=0;i<bytcon;i++)
		stream << char(temp[i]);
	free(temp);
}

SDL_Surface * CDefHandler::getSprite (int SIndex, unsigned char * FDef, BMPPalette * palette)
{
	SDL_Surface * ret=NULL;

	long BaseOffset, 
		SpriteWidth, SpriteHeight, //format sprite'a
		LeftMargin, RightMargin, TopMargin,BottomMargin,
		i, add, FullHeight,FullWidth,
		TotalRowLength, // dlugosc przeczytanego segmentu
		NextSpriteOffset, RowAdd;

	unsigned char SegmentType, SegmentLength, BL, BR;

	i=BaseOffset=SEntries[SIndex].offset;
	int prSize=readNormalNr(i,4,FDef);i+=4;
	int defType2 = readNormalNr(i,4,FDef);i+=4;
	FullWidth = readNormalNr(i,4,FDef);i+=4;
	FullHeight = readNormalNr(i,4,FDef);i+=4;
	SpriteWidth = readNormalNr(i,4,FDef);i+=4;
	SpriteHeight = readNormalNr(i,4,FDef);i+=4;
	LeftMargin = readNormalNr(i,4,FDef);i+=4;
	TopMargin = readNormalNr(i,4,FDef);i+=4;
	RightMargin = FullWidth - SpriteWidth - LeftMargin;
	BottomMargin = FullHeight - SpriteHeight - TopMargin;

	//if(LeftMargin + RightMargin < 0)
	//	SpriteWidth += LeftMargin + RightMargin; //ugly construction... TODO: check how to do it nicer
	if(LeftMargin<0)
		SpriteWidth+=LeftMargin;
	if(RightMargin<0)
		SpriteWidth+=RightMargin;
	
	add = 4 - FullWidth%4;
	if (add==4)
		add=0;

	int ftcp=0;
		
	ret = SDL_CreateRGBSurface(SDL_SWSURFACE, FullWidth, FullHeight, 8, 0, 0, 0, 0);
	//int tempee2 = readNormalNr(0,4,((unsigned char *)tempee.c_str()));

	int BaseOffsetor = BaseOffset = i;

	for(int i=0; i<256; ++i)
	{
		SDL_Color pr;
		pr.r = palette[i].R;
		pr.g = palette[i].G;
		pr.b = palette[i].B;
		pr.unused = palette[i].F;
		(*(ret->format->palette->colors+i))=pr;
	}

	if (defType2==0)
	{
		if (TopMargin>0)
		{
			for (int i=0;i<TopMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			if (LeftMargin>0)
			{
				for (int j=0;j<LeftMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			for (int j=0; j<SpriteWidth;j++)
				((char*)(ret->pixels))[ftcp++]=FDef[BaseOffset++];
			if (RightMargin>0)
			{
				for (int j=0;j<add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		if (BottomMargin>0)
		{
			for (int i=0;i<BottomMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}

	}
	if (defType2==1)
	{
		if (TopMargin>0)
		{
			for (int i=0;i<TopMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			RWEntries[i]=readNormalNr(BaseOffset,4,FDef);BaseOffset+=4;
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor+RWEntries[i];
			if (LeftMargin>0)
			{
				for (int j=0;j<LeftMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			TotalRowLength=0;
			do
			{
				SegmentType=FDef[BaseOffset++];
				SegmentLength=FDef[BaseOffset++];
				if (SegmentType==0xFF)
				{
					for (int k=0;k<=SegmentLength;k++)
					{
						((char*)(ret->pixels))[ftcp++]=FDef[BaseOffset+k];
						if ((TotalRowLength+k+1)>=SpriteWidth)
							break;
					}
					BaseOffset+=SegmentLength+1;////
					TotalRowLength+=SegmentLength+1;
				}
				else
				{
					for (int k=0;k<SegmentLength+1;k++)
					{
						((char*)(ret->pixels))[ftcp++]=SegmentType;//
						//((char*)(ret->pixels))+='\0';
					}
					TotalRowLength+=SegmentLength+1;
				}
			}while(TotalRowLength<SpriteWidth);
			RowAdd=SpriteWidth-TotalRowLength;
			if (RightMargin>0)
			{
				for (int j=0;j<RightMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			if (add>0)
			{
				for (int j=0;j<add+RowAdd;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		if (BottomMargin>0)
		{
			for (int i=0;i<BottomMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
	}
	if (defType2==2)
	{
		if (TopMargin>0)
		{
			for (int i=0;i<TopMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor+i*2*(SpriteWidth/32);
			RWEntries[i] = readNormalNr(BaseOffset,2,FDef);
		}
		BaseOffset = BaseOffsetor+RWEntries[0];
		for (int i=0;i<SpriteHeight;i++)
		{
			//BaseOffset = BaseOffsetor+RWEntries[i];
			if (LeftMargin>0)
			{
				for (int j=0;j<LeftMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			TotalRowLength=0;
			do
			{
				SegmentType=FDef[BaseOffset++];
				unsigned char code = SegmentType / 32;
				unsigned char value = (SegmentType & 31) + 1;
				if(code==7)
				{
					for(int h=0; h<value; ++h)
					{
						((char*)(ret->pixels))[ftcp++]=FDef[BaseOffset++];
					}
				}
				else
				{
					for(int h=0; h<value; ++h)
					{
						((char*)(ret->pixels))[ftcp++]=code;
					}
				}
				TotalRowLength+=value;
			} while(TotalRowLength<SpriteWidth);
			if (RightMargin>0)
			{
				for (int j=0;j<RightMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			if (add>0)
			{
				for (int j=0;j<add+RowAdd;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		if (BottomMargin>0)
		{
			for (int i=0;i<BottomMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
	}
	if (defType2==3)
	{
		if (add==4)
			add=0;
		if (TopMargin>0)
		{
			for (int i=0;i<TopMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor+i*2*(SpriteWidth/32);
			RWEntries[i] = readNormalNr(BaseOffset,2,FDef);
		}
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset = BaseOffsetor+RWEntries[i];
			if (LeftMargin>0)
			{
				for (int j=0;j<LeftMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			TotalRowLength=0;
			do
			{
				SegmentType=FDef[BaseOffset++];
				unsigned char code = SegmentType / 32;
				unsigned char value = (SegmentType & 31) + 1;
				if(code==7)
				{
					for(int h=0; h<value; ++h)
					{
						if(h<-LeftMargin)
							continue;
						if(h+TotalRowLength>=SpriteWidth)
							break;
						((char*)(ret->pixels))[ftcp++]=FDef[BaseOffset++];
					}
				}
				else
				{
					for(int h=0; h<value; ++h)
					{
						if(h<-LeftMargin)
							continue;
						if(h+TotalRowLength>=SpriteWidth)
							break;
						((char*)(ret->pixels))[ftcp++]=code;
					}
				}
				TotalRowLength+=( LeftMargin>=0 ? value : value+LeftMargin );
			}while(TotalRowLength<SpriteWidth);
			if (RightMargin>0)
			{
				for (int j=0;j<RightMargin;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
			if (add>0)
			{
				for (int j=0;j<add+RowAdd;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
		if (BottomMargin>0)
		{
			for (int i=0;i<BottomMargin;i++)
			{
				for (int j=0;j<FullWidth+add;j++)
					((char*)(ret->pixels))[ftcp++]='\0';
			}
		}
	}
	SDL_Color ttcol = ret->format->palette->colors[0];
	Uint32 keycol = SDL_MapRGBA(ret->format, ttcol.r, ttcol.b, ttcol.g, ttcol.unused);
	SDL_SetColorKey(ret, SDL_SRCCOLORKEY, keycol);
	return ret;
};

CDefEssential * CDefHandler::essentialize()
{
	CDefEssential * ret = new CDefEssential;
	ret->ourImages = ourImages;
	notFreeImgs = true;
	return ret;
}

CDefHandler * CDefHandler::giveDef(std::string defName, CLodHandler * spriteh)
{
	if(!spriteh) spriteh=Spriteh;
	unsigned char * data = spriteh->giveFile(defName);
	if(!data)
		throw "bad def name!";
	CDefHandler * nh = new CDefHandler();
	nh->openFromMemory(data, defName);
	nh->alphaTransformed = false;
	delete [] data;
	return nh;
}
CDefEssential * CDefHandler::giveDefEss(std::string defName, CLodHandler * spriteh)
{
	CDefEssential * ret;
	CDefHandler * temp = giveDef(defName,spriteh);
	ret = temp->essentialize();
	delete temp;
	return ret;
}

