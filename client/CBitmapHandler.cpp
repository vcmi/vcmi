#include "../stdafx.h"
#include "SDL.h"
#include "SDL_image.h"
#include "CBitmapHandler.h"
#include "../hch/CDefHandler.h"
#include "../hch/CLodHandler.h"
#include <sstream>
#include <boost/thread.hpp>
boost::mutex bitmap_handler_mx;
int readNormalNr (int pos, int bytCon, unsigned char * str);
CLodHandler * BitmapHandler::bitmaph = NULL;
void BMPHeader::print(std::ostream & out)
{
	CDefHandler::print(out,fullSize,4);
	CDefHandler::print(out,_h1,4);
	CDefHandler::print(out,_c1,4);
	CDefHandler::print(out,_c2,4);
	CDefHandler::print(out,x,4);
	CDefHandler::print(out,y,4);
	CDefHandler::print(out,_c3,2);
	CDefHandler::print(out,_c4,2);
	CDefHandler::print(out,_h2,4);
	CDefHandler::print(out,_h3,4);
	CDefHandler::print(out,dataSize1,4);
	CDefHandler::print(out,dataSize2,4);
	for (int i=0;i<8;i++)
		out << _c5[i];
	out.flush();
}
void CPCXConv::openPCX(char * PCX, int len)
{
	pcxs=len;
	pcx=(unsigned char*)PCX;
}
void CPCXConv::fromFile(std::string path)
{
	std::ifstream * is = new std::ifstream();
	is -> open(path.c_str(),std::ios::binary);
	is->seekg(0,std::ios::end); // to the end
	pcxs = is->tellg();  // read length
	is->seekg(0,std::ios::beg); // wracamy na poczatek
	pcx = new unsigned char[pcxs]; // allocate memory 
	is->read((char*)pcx, pcxs); // read map file to buffer
	is->close();
	delete is;
}
void CPCXConv::saveBMP(std::string path)
{
	std::ofstream os;
	os.open(path.c_str(), std::ios::binary);
	os.write((char*)bmp,bmps);
	os.close();
}
void CPCXConv::convert()
{
	BMPHeader bh;
	BMPPalette pal[256];
	Epcxformat format;
	int fSize,i,y;
	bool check1, check2;
	unsigned char add;
	int it=0;

	std::stringstream out;

	fSize = readNormalNr(it,4,pcx);it+=4;
	bh.x = readNormalNr(it,4,pcx);it+=4;
	bh.y = readNormalNr(it,4,pcx);it+=4;
	if (fSize==bh.x*bh.y*3)
		check1=true;
	else 
		check1=false;
	if (fSize==bh.x*bh.y)
		check2=true;
	else 
		check2=false;
	if (check1)
		format=PCX24B;
	else if (check2)
		format=PCX8B;
	else 
		return;
	add = 4 - bh.x%4;
	if (add==4)
		add=0;
	bh._h3=bh.x*bh.y;
	if (format==PCX8B)
	{
		bh._c1=0x436;
		bh._c2=0x28;
		bh._c3=1;
		bh._c4=8;
		//bh.dataSize2=bh.dataSize1=maxx*maxy;
		bh.dataSize1=bh.x;
		bh.dataSize2=bh.y;
		bh.fullSize = bh.dataSize1+436;
	}
	else
	{
		bh._c1=0x36;
		bh._c2=0x28;
		bh._c3=1;
		bh._c4=0x18;
		//bh.dataSize2=bh.dataSize1=0xB12;
		bh.dataSize1=bh.x;
		bh.dataSize2=bh.y;
		bh.fullSize=(bh.x+add)*bh.y*3+36+18;
		bh._h3*=3;
	}
	if (format==PCX8B)
	{
		it = pcxs-256*3;
		for (int i=0;i<256;i++)
		{
			pal[i].R=pcx[it++];
			pal[i].G=pcx[it++];
			pal[i].B=pcx[it++];
			pal[i].F='\0';
		}
	}
	out<<"BM";
	bh.print(out);
	if (format==PCX8B)
	{
		for (int i=0;i<256;i++)
		{
			out<<pal[i].B;
			out<<pal[i].G;
			out<<pal[i].R;
			out<<pal[i].F;
		}
		for (y=bh.y;y>0;y--)
		{
			it=0xC+(y-1)*bh.x;
			for (int j=0;j<bh.x;j++)
				out<<pcx[it+j];
			if (add>0)
			{
				for (int j=0;j<add;j++)
					out<<'\0'; //bylo z buforu, ale onnie byl incjalizowany (?!)
			}
		}
	}
	else
	{
		for (y=bh.y; y>0; y--)
		{
			it=0xC+(y-1)*bh.x*3;
			for (int j=0;j<bh.x*3;j++)
				out<<pcx[it+j];
			if (add>0)
			{
				for (int j=0;j<add*3;j++)
					out<<'\0'; //bylo z buforu, ale onnie byl incjalizowany (?!)
			}
		}
	}
	std::string temp = out.str();
	bmp = new unsigned char[temp.length()];
	bmps=temp.length();
	for (int a=0;a<temp.length();a++)
	{
		bmp[a]=temp[a];
	}
}

SDL_Surface * CPCXConv::getSurface()
{
	SDL_Surface * ret;

	BMPHeader bh;
	Epcxformat format;
	int fSize,i,y;
	bool check1, check2;
	unsigned char add;
	int it=0;

	fSize = readNormalNr(it,4,pcx);it+=4;
	bh.x = readNormalNr(it,4,pcx);it+=4;
	bh.y = readNormalNr(it,4,pcx);it+=4;
	if (fSize==bh.x*bh.y*3)
		check1=true;
	else 
		check1=false;
	if (fSize==bh.x*bh.y)
		check2=true;
	else 
		check2=false;
	if (check1)
		format=PCX24B;
	else if (check2)
		format=PCX8B;
	else 
		return NULL;
	add = 4 - bh.x%4;
	if (add==4)
		add=0;
	if (format==PCX8B)
	{
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, bh.x+add, bh.y, 8, 0, 0, 0, 0);
	}
	else
	{
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
		int bmask = 0xff0000;
		int gmask = 0x00ff00;
		int rmask = 0x0000ff;
#else
		int bmask = 0x0000ff;
		int gmask = 0x00ff00;
		int rmask = 0xff0000;
#endif
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, bh.x+add, bh.y, 24, rmask, gmask, bmask, 0);
	}
	if (format==PCX8B)
	{
		it = pcxs-256*3;
		for (int i=0;i<256;i++)
		{
			SDL_Color tp;
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
			tp.b = pcx[it++];
			tp.g = pcx[it++];
			tp.r = pcx[it++];
#else
			tp.r = pcx[it++];
			tp.g = pcx[it++];
			tp.b = pcx[it++];
#endif
			tp.unused = 0;
			*(ret->format->palette->colors+i) = tp;
		}
		for (y=bh.y;y>0;y--)
		{
			it=0xC+(y-1)*bh.x;
			for (int j=0;j<bh.x;j++)
			{
				*((char*)ret->pixels + ret->pitch * (y-1) + ret->format->BytesPerPixel * j) = pcx[it+j];
			}
			if (add>0)
			{
				for (int j=0;j<add;j++)
				{
					*((char*)ret->pixels + ret->pitch * (y-1) + ret->format->BytesPerPixel * (j+bh.x)) = 0;
				}
			}
		}
	}
	else
	{
		for (y=bh.y; y>0; y--)
		{
			it=0xC+(y-1)*bh.x*3;
			for (int j=0;j<bh.x*3;j++)
			{
				*((char*)ret->pixels + ret->pitch * (y-1) + j) = pcx[it+j];
			}
			if (add>0)
			{
				for (int j=0;j<add*3;j++)
				{
					*((char*)ret->pixels + ret->pitch * (y-1) + (j+bh.x*3)) = 0;
				}
			}
		}
	}
	return ret;
}

SDL_Surface * BitmapHandler::loadBitmap(std::string fname, bool setKey)
{
	if(!fname.size())
		return NULL;
	unsigned char * pcx;
	std::transform(fname.begin(),fname.end(),fname.begin(),toupper);
	fname.replace(fname.find_first_of('.'),fname.find_first_of('.')+4,".PCX");
	Entry *e = bitmaph->entries.znajdz(fname);
	if(!e)
	{
		tlog2<<"File "<<fname<<" not found"<<std::endl;
		return NULL;
	}
	if(e->offset<0)
	{
		fname.replace(fname.find_first_of('.'),fname.find_first_of('.')+4,".BMP");
		fname = "Data/"+fname;
		FILE * f = fopen(fname.c_str(),"r");
		if(f)
		{
			fclose(f);
			return SDL_LoadBMP(fname.c_str());
		}
		else  //file .bmp not present, check .pcx
		{
			char sign[3];
			fname.replace(fname.find_first_of('.'),fname.find_first_of('.')+4,".PCX");
			f = fopen(fname.c_str(),"r");
			if(!f)
				return NULL; 
			fread(sign,1,3,f);
			if(sign[0]=='B' && sign[1]=='M') //BMP named as PCX - people (eg. Kulex) sometimes use such files
			{
				fclose(f);
				return SDL_LoadBMP(fname.c_str());
			}
			else //PCX - but we don't know which
			{
				if((sign[0]==10) && (sign[1]<6) && (sign[2]==1)) //ZSoft PCX
				{
					fclose(f);
					return IMG_Load(fname.c_str());
				}
				else //H3-style PCX
				{
					CPCXConv cp;
					pcx = new unsigned char[e->realSize];
					memcpy(pcx,sign,3);
					int res = fread((char*)pcx+3, 1, e->realSize-3, f);
					fclose(f);
					cp.openPCX((char*)pcx,e->realSize);
					return cp.getSurface();
				}
			}
		}
	}
	bitmap_handler_mx.lock();
	fseek(bitmaph->FLOD, e->offset, 0);
	if (e->size==0) //file is not compressed
	{
		pcx = new unsigned char[e->realSize];
		fread((char*)pcx, 1, e->realSize, bitmaph->FLOD);
		bitmap_handler_mx.unlock();
	}
	else 
	{
		unsigned char * pcd = new unsigned char[e->size];
		fread((char*)pcd, 1, e->size, bitmaph->FLOD);
		bitmap_handler_mx.unlock();
		int res=bitmaph->infs2(pcd,e->size,e->realSize,pcx);
		if(res!=0)
		{
			tlog2<<"an error "<<res<<" occured while extracting file "<<fname<<std::endl;
		}
		delete [] pcd;
	}
	CPCXConv cp;
	cp.openPCX((char*)pcx,e->realSize);
	SDL_Surface * ret = cp.getSurface();
	if(setKey)
		SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,0,255,255));
	return ret;
}