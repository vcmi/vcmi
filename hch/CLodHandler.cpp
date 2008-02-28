#include "../stdafx.h"
#include "CLodHandler.h"
#include "../SDL_Extensions.h"
#include "CDefHandler.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>
#include "boost/filesystem.hpp"   // includes all needed Boost.Filesystem declarations
int readNormalNr (int pos, int bytCon, unsigned char * str)
{
	int ret=0;
	int amp=1;
	if (str)
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp<<=8;
		}
	}
	else return -1;
	return ret;
}
void CPCXConv::openPCX(char * PCX, int len)
{
	pcxs=len;
	pcx=(unsigned char*)PCX;
	/*pcx = new unsigned char[len];
	for (int i=0;i<len;i++)
		pcx[i]=PCX[i];*/
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
		int bmask = 0x0000ff;
		int gmask = 0x00ff00;
		int rmask = 0xff0000;
		ret = SDL_CreateRGBSurface(SDL_SWSURFACE, bh.x+add, bh.y, 24, rmask, gmask, bmask, 0);
	}
	if (format==PCX8B)
	{
		it = pcxs-256*3;
		for (int i=0;i<256;i++)
		{
			SDL_Color tp;
			tp.r = pcx[it++];
			tp.g = pcx[it++];
			tp.b = pcx[it++];
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
SDL_Surface * CLodHandler::loadBitmap(std::string fname)
{
	if(!fname.size())
		return NULL;
	unsigned char * pcx;
	std::transform(fname.begin(),fname.end(),fname.begin(),toupper);
	fname.replace(fname.find_first_of('.'),fname.find_first_of('.')+4,".PCX");
	Entry *e = entries.znajdz(fname);
	if(!e)
	{
		std::cout<<"File "<<fname<<" not found"<<std::endl;
		return NULL;
	}
	if(e->offset<0)
	{
		fname.replace(fname.find_first_of('.'),fname.find_first_of('.')+4,".BMP");
		fname = "Data/"+fname;
		return SDL_LoadBMP(fname.c_str());
	}
	fseek(FLOD, e->offset, 0);
	if (e->size==0) //file is not compressed
	{
		pcx = new unsigned char[e->realSize];
		fread((char*)pcx, 1, e->realSize, FLOD);
	}
	else 
	{
		unsigned char * pcd = new unsigned char[e->size];
		fread((char*)pcd, 1, e->size, FLOD);
		int res=infs2(pcd,e->size,e->realSize,pcx);
		if(res!=0)
		{
			std::cout<<"an error "<<res<<" occured while extracting file "<<fname<<std::endl;
		}
		delete [] pcd;
	}
	CPCXConv cp;
	cp.openPCX((char*)pcx,e->realSize);
	return cp.getSurface();
}

int CLodHandler::decompress (unsigned char * source, int size, int realSize, std::string & dest)
{
	std::ofstream lb;
	lb.open("lodbuf\\buf.gz", std::ios::out|std::ios::binary);
	for(int i=0; i<size; ++i)
	{
		lb<<source[i];
	}
	lb.close();

	FILE * inputf = fopen("lodbuf\\buf.gz", "rb+");
	FILE * outputf = fopen(dest.c_str(), "wb+");

	int ret = infm(inputf, outputf);
	fclose(inputf);
	fclose(outputf);
	return ret;
} 

int CLodHandler::infm(FILE *source, FILE *dest, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[NLoadHandlerHelp::fCHUNK];
	unsigned char out[NLoadHandlerHelp::fCHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, wBits);
	if (ret != Z_OK)
		return ret;

	/* decompress until deflate stream ends or end of file */
	do
	{
		strm.avail_in = fread(in, 1, NLoadHandlerHelp::fCHUNK, source);
		if (ferror(source)) {
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}
		if (strm.avail_in == 0)
			break;
		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = NLoadHandlerHelp::fCHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;	 /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = NLoadHandlerHelp::fCHUNK - strm.avail_out;
			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
CDefHandler * CLodHandler::giveDef(std::string defName) 
{
	std::transform(defName.begin(), defName.end(), defName.begin(), (int(*)(int))toupper);
	Entry * ourEntry = entries.znajdz(Entry(defName));
	CDefHandler * ret;
	fseek(FLOD, ourEntry->offset, 0);
	unsigned char * outp;
	if (ourEntry->offset<0) //file in the sprites/ folder
	{
		char * outp = new char[ourEntry->realSize];
		char name[120];for(int i=0;i<120;i++) name[i]='\0';
		strcat(name,"Sprites/");
		strcat(name,(char*)ourEntry->name);
		FILE * f = fopen(name,"rb");
		int result = fread(outp,1,ourEntry->realSize,f);
		if(result<0) {std::cout<<"Error in file reading: "<<name<<std::endl;delete[] outp; return NULL;}
		CDefHandler * nh = new CDefHandler();
		nh->openFromMemory((unsigned char*)outp, ourEntry->realSize, std::string((char*)ourEntry->name));
		nh->alphaTransformed = false;
		ret = nh;
		delete[] outp;
	}
	else if (ourEntry->size==0) //file is not compressed
	{
		outp = new unsigned char[ourEntry->realSize];
		fread((char*)outp, 1, ourEntry->realSize, FLOD);
		CDefHandler * nh = new CDefHandler();
		nh->openFromMemory(outp, ourEntry->realSize, std::string((char*)ourEntry->name));
		nh->alphaTransformed = false;
		ret = nh;
		delete[] outp;
	}
	else //we will decompress file
	{
		outp = new unsigned char[ourEntry->size];
		fread((char*)outp, 1, ourEntry->size, FLOD);
		fseek(FLOD, 0, 0);
		unsigned char * decomp = NULL;
		int decRes = infs2(outp, ourEntry->size, ourEntry->realSize, decomp);
		CDefHandler * nh = new CDefHandler();
		nh->openFromMemory(decomp, ourEntry->realSize, std::string((char*)ourEntry->name));
		nh->alphaTransformed = false;
		ret = nh;
		delete[] decomp;
		delete[] outp;
	}
	return ret;
}
CDefEssential * CLodHandler::giveDefEss(std::string defName)
{
	CDefEssential * ret;
	CDefHandler * temp = giveDef(defName);
	ret = temp->essentialize();
	delete temp;
	return ret;
}
std::vector<CDefHandler *> CLodHandler::extractManyFiles(std::vector<std::string> defNamesIn)
{
	std::vector<CDefHandler *> ret(defNamesIn.size()); 
	for(int hh=0; hh<defNamesIn.size(); ++hh)
	{
		//std::transform(defNamesIn[hh].begin(), defNamesIn[hh].end(), defNamesIn[hh].begin(), (int(*)(int))toupper);
		Entry * e = entries.znajdz(defNamesIn[hh]);
		if(!e)
			continue;

		fseek(FLOD, e->offset, 0);
		unsigned char * outp;
		if (e->size==0) //file is not compressed
		{
			outp = new unsigned char[e->realSize];
			fread((char*)outp, 1, e->realSize, FLOD);
			CDefHandler * nh = new CDefHandler;
			nh->openFromMemory(outp, e->realSize, std::string((char*)e->name));
			nh->alphaTransformed = false;
			ret[hh] = nh;
		}
		else //we will decompressing file
		{
			outp = new unsigned char[e->size];
			fread((char*)outp, 1, e->size, FLOD);
			fseek(FLOD, 0, 0);
			unsigned char * decomp = NULL;
			int decRes = infs2(outp, e->size, e->realSize, decomp);
			CDefHandler * nh = new CDefHandler;
			nh->openFromMemory(decomp, e->realSize, std::string((char*)e->name));
			nh->alphaTransformed = false;
			delete [] decomp;
			ret[hh] = nh;
		}
		delete[] outp;
	}
	return ret;
}
int CLodHandler::infs(unsigned char * in, int size, int realSize, std::ofstream & out, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char inx[NLoadHandlerHelp::fCHUNK];
	unsigned char outx[NLoadHandlerHelp::fCHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, wBits);
	if (ret != Z_OK)
		return ret;
	int chunkNumber = 0;
	do
	{
		int readBytes = 0;
		for(int i=0; i<NLoadHandlerHelp::fCHUNK && (chunkNumber * NLoadHandlerHelp::fCHUNK + i)<size; ++i)
		{
			inx[i] = in[chunkNumber * NLoadHandlerHelp::fCHUNK + i];
			++readBytes;
		}
		++chunkNumber;
		strm.avail_in = readBytes;
		//strm.avail_in = fread(inx, 1, NLoadHandlerHelp::fCHUNK, source);
		/*if (in.bad())
		{
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}*/
		if (strm.avail_in == 0)
			break;
		strm.next_in = inx;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = NLoadHandlerHelp::fCHUNK;
			strm.next_out = outx;
			ret = inflate(&strm, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;	 /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = NLoadHandlerHelp::fCHUNK - strm.avail_out;
			/*if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}*/
			out.write((char*)outx, have);
			if(out.bad())
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int CLodHandler::infs2(unsigned char * in, int size, int realSize, unsigned char *& out, int wBits)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char inx[NLoadHandlerHelp::fCHUNK];
	unsigned char outx[NLoadHandlerHelp::fCHUNK];
	out = new unsigned char [realSize];
	int latPosOut = 0;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit2(&strm, wBits);
	if (ret != Z_OK)
		return ret;
	int chunkNumber = 0;
	do
	{
		int readBytes = 0;
		for(int i=0; i<NLoadHandlerHelp::fCHUNK && (chunkNumber * NLoadHandlerHelp::fCHUNK + i)<size; ++i)
		{
			inx[i] = in[chunkNumber * NLoadHandlerHelp::fCHUNK + i];
			++readBytes;
		}
		++chunkNumber;
		strm.avail_in = readBytes;
		//strm.avail_in = fread(inx, 1, NLoadHandlerHelp::fCHUNK, source);
		/*if (in.bad())
		{
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}*/
		if (strm.avail_in == 0)
			break;
		strm.next_in = inx;

		/* run inflate() on input until output buffer not full */
		do
		{
			strm.avail_out = NLoadHandlerHelp::fCHUNK;
			strm.next_out = outx;
			ret = inflate(&strm, Z_NO_FLUSH);
			//assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
			switch (ret)
			{
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR;	 /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					return ret;
			}
			have = NLoadHandlerHelp::fCHUNK - strm.avail_out;
			/*if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}*/
			//out.write((char*)outx, have);
			for(int oo=0; oo<have; ++oo)
			{
				out[latPosOut] = outx[oo];
				++latPosOut;
			}
			/*if(out.bad())
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}*/
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void CLodHandler::extract(std::string FName)
{
	
	std::ofstream FOut;
	int i;

	//std::cout<<" done\n";
	for (int i=0;i<totalFiles;i++)
	{
		fseek(FLOD, entries[i].offset, 0);
		std::string bufff = (FName.substr(0, FName.size()-4) + "\\" + (char*)entries[i].name);
		unsigned char * outp;
		if (entries[i].size==0) //file is not compressed
		{
			outp = new unsigned char[entries[i].realSize];
			fread((char*)outp, 1, entries[i].realSize, FLOD);
			std::ofstream out;
			out.open(bufff.c_str(), std::ios::binary);
			if(!out.is_open())
			{
				std::cout<<"Unable to create "<<bufff;
			}
			else
			{
				for(int hh=0; hh<entries[i].realSize; ++hh)
				{
					out<<*(outp+hh);
				}
				out.close();
			}
		}
		else 
		{
			outp = new unsigned char[entries[i].size];
			fread((char*)outp, 1, entries[i].size, FLOD);
			fseek(FLOD, 0, 0);
			std::ofstream destin;
			destin.open(bufff.c_str(), std::ios::binary);
			//int decRes = decompress(outp, entries[i].size, entries[i].realSize, bufff);
			int decRes = infs(outp, entries[i].size, entries[i].realSize, destin);
			destin.close();
			if(decRes!=0)
			{
				std::cout<<"LOD Extraction error"<<"  "<<decRes<<" while extracting to "<<bufff<<std::endl;
			}
		}
		delete[] outp;
	}
	fclose(FLOD);
}

void CLodHandler::extractFile(std::string FName, std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
	for (int i=0;i<totalFiles;i++)
	{
		std::string buf1 = std::string((char*)entries[i].name);
		std::transform(buf1.begin(), buf1.end(), buf1.begin(), (int(*)(int))toupper);
		if(buf1!=name)
			continue;
		fseek(FLOD, entries[i].offset, 0);
		std::string bufff = (FName);
		unsigned char * outp;
		if (entries[i].size==0) //file is not compressed
		{
			outp = new unsigned char[entries[i].realSize];
			fread((char*)outp, 1, entries[i].realSize, FLOD);
			std::ofstream out;
			out.open(bufff.c_str(), std::ios::binary);
			if(!out.is_open())
			{
				std::cout<<"Unable to create "<<bufff;
			}
			else
			{
				for(int hh=0; hh<entries[i].realSize; ++hh)
				{
					out<<*(outp+hh);
				}
				out.close();
			}
		}
		else //we will decompressing file
		{
			outp = new unsigned char[entries[i].size];
			fread((char*)outp, 1, entries[i].size, FLOD);
			fseek(FLOD, 0, 0);
			std::ofstream destin;
			destin.open(bufff.c_str(), std::ios::binary);
			//int decRes = decompress(outp, entries[i].size, entries[i].realSize, bufff);
			int decRes = infs(outp, entries[i].size, entries[i].realSize, destin);
			destin.close();
			if(decRes!=0)
			{
				std::cout<<"LOD Extraction error"<<"  "<<decRes<<" while extracting to "<<bufff<<std::endl;
			}
		}
		delete[] outp;
	}
}

int CLodHandler::readNormalNr (unsigned char* bufor, int bytCon, bool cyclic)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[i]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}

void CLodHandler::init(std::string lodFile)
{
	std::string Ts;
	FLOD = fopen(lodFile.c_str(), "rb");
	fseek(FLOD, 8, 0);
	unsigned char temp[4];
	fread((char*)temp, 1, 4, FLOD);
	totalFiles = readNormalNr(temp,4);
	fseek(FLOD, 0x5c, 0);
	for (int i=0; i<totalFiles; i++)
	{
		Entry entry;
		char * bufc = new char;
		bool appending = true;
		for(int kk=0; kk<12; ++kk)
		{
			//FLOD.read(bufc, 1);
			fread(bufc, 1, 1, FLOD);
			if(appending)
			{
				entry.name[kk] = toupper(*bufc);
			}
			else
			{
				entry.name[kk] = 0;
				appending = false;
			}
		}
		delete bufc;
		fread((char*)entry.hlam_1, 1, 4, FLOD);
		fread((char*)temp, 1, 4, FLOD);
		entry.offset=readNormalNr(temp,4);
		fread((char*)temp, 1, 4, FLOD);
		entry.realSize=readNormalNr(temp,4);
		fread((char*)entry.hlam_2, 1, 4, FLOD);
		fread((char*)temp, 1, 4, FLOD);
		entry.size=readNormalNr(temp,4);
		for (int z=0;z<12;z++)
		{
			if (entry.name[z])
				entry.nameStr+=entry.name[z];
			else break;
		}
		entries.push_back(entry);
	}
}

std::string CLodHandler::getTextFile(std::string name)
{
	std::string ret0;
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
	Entry *e;
	e = entries.znajdz(name);
	if(!e)
	{
		std::cout << "Error: cannot load "<<name<<" from the .lod file!"<<std::endl;
		return ret0;
	}
	if(e->offset<0)
	{
		char * outp = new char[e->realSize];
		char name[120];for(int i=0;i<120;i++) name[i]='\0';
		strcat(name,"Data/");
		strcat(name,(char*)e->name);
		FILE * f = fopen(name,"rb");
		int result = fread(outp,1,e->realSize,f);
		if(result<0) {std::cout<<"Error in file reading: "<<name<<std::endl;return ret0;}
		for (int itr=0;itr<e->realSize;itr++)
			ret0+= *(outp+itr);
		delete[] outp;
		return ret0;
	}
	fseek(FLOD, e->offset, 0);
	unsigned char * outp;
	if (e->size==0) //file is not compressed
	{
		outp = new unsigned char[e->realSize];
		fread((char*)outp, 1, e->realSize, FLOD);
		std::string ret = std::string((char*)outp);
		delete[] outp;
		return ret;
	}
	else //we will decompressing file
	{
		outp = new unsigned char[e->size];
		fread((char*)outp, 1, e->size, FLOD);
		fseek(FLOD, 0, 0);
		unsigned char * decomp = NULL;
		int decRes = infs2(outp, e->size, e->realSize, decomp);
		std::string ret;
		for (int itr=0;itr<e->realSize;itr++)
			ret+= *((char*)decomp+itr);
		delete[] outp;
		delete[] decomp;
		return ret;
	}
	return ret0;
}
