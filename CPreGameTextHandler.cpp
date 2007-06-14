#include "stdafx.h"
#include "CPreGameTextHandler.h"
std::string CPreGameTextHandler::getTitle(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='{'));
	while ((text[i]!='}') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
std::string CPreGameTextHandler::getDescr(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='}'));
	i+=2;
	while ((text[i]!='"') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
void CPreGameTextHandler::loadTexts()
{
	std::ifstream inp("H3bitmap.lod\\ZELP.TXT", std::ios::in|std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=3;

	int befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainNewGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainLoadGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainHighScores = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainCredits = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainQuit = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngSingleScenario = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngCampain = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngMultiplayer = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngTutorial = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	ngBack = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==4)
			break;
	}
	i+=2;
	loadToIt(singleChooseScenario, buf, i);
	loadToIt(singleSetAdvOptions, buf, i);
	loadToIt(singleRandomMap, buf, i);
	loadToIt(singleScenarioName, buf, i);
	loadToIt(singleDescriptionTitle, buf, i);
	loadToIt(singleDescriptionText, buf, i);
	loadToIt(singleEasy, buf, i);
	loadToIt(singleNormal, buf, i);
	loadToIt(singleHard, buf, i);
	loadToIt(singleExpert, buf, i);
	loadToIt(singleImpossible, buf, i);
	loadToIt(singleAllyFlag[0], buf, i);
	loadToIt(singleAllyFlag[1], buf, i);
	loadToIt(singleAllyFlag[2], buf, i);
	loadToIt(singleAllyFlag[3], buf, i, 1);
	loadToIt(singleAllyFlag[4], buf, i, 1);
	loadToIt(singleAllyFlag[5], buf, i, 1);
	loadToIt(singleAllyFlag[6], buf, i, 1);
	loadToIt(singleAllyFlag[7], buf, i, 1);
	loadToIt(singleEnemyFlag[0], buf, i, 1);
	loadToIt(singleEnemyFlag[1], buf, i, 1);
	loadToIt(singleEnemyFlag[2], buf, i, 1);
	loadToIt(singleEnemyFlag[3], buf, i, 1);
	loadToIt(singleEnemyFlag[4], buf, i, 1);
	loadToIt(singleEnemyFlag[5], buf, i, 1);
	loadToIt(singleEnemyFlag[6], buf, i, 1);
	loadToIt(singleEnemyFlag[7], buf, i, 1);
	loadToIt(singleViewHideScenarioList, buf, i, 1);
	loadToIt(singleViewHideAdvOptions, buf, i, 1);
	loadToIt(singlePlayRandom, buf, i, 1);
	loadToIt(singleChatDesc, buf, i, 1);
	loadToIt(singleMapDifficulty, buf, i, 1);
	loadToIt(singleRating, buf, i, 1);
	loadToIt(singleMapPossibleDifficulties, buf, i, 1);
	loadToIt(singleVicCon, buf, i, 1);
	loadToIt(singleLossCon, buf, i, 1);
	loadToIt(singleSFilter, buf, i, 1);
	loadToIt(singleMFilter, buf, i, 1);
	loadToIt(singleLFilter, buf, i, 1);
	loadToIt(singleXLFilter, buf, i, 1);
	loadToIt(singleAllFilter, buf, i, 1);
}

void CPreGameTextHandler::loadToIt(std::string &dest, std::string &src, int &iter, int mode)
{
	switch(mode)
	{
	case 0:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			++iter;

			hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					++hmcr;
				if(hmcr==1)
					break;
			}
			iter+=2;
			break;
		}
	case 1:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter+=2;
		}
	}
}