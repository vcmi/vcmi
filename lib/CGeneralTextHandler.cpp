#include "StdInc.h"
#include "CGeneralTextHandler.h"

#include "Filesystem/CResourceLoader.h"
#include "Filesystem/CInputStream.h"
#include "GameConstants.h"

// #include <locale> //needed?

/*
 * CGeneralTextHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//Helper for string -> float conversion
class LocaleWithComma: public std::numpunct<char>
{
protected:
	char do_decimal_point() const
	{
		return ',';
	}
};

CLegacyConfigParser::CLegacyConfigParser(std::string URI)
{
	init(CResourceHandler::get()->load(ResourceID(URI, EResType::TEXT)));
}

CLegacyConfigParser::CLegacyConfigParser(const std::unique_ptr<CInputStream> & input)
{
	init(input);
}

void CLegacyConfigParser::init(const std::unique_ptr<CInputStream> & input)
{
	data.reset(new char[input->getSize()]);
	input->read((ui8*)data.get(), input->getSize());

	curr = data.get();
	end = curr + input->getSize();
}

std::string CLegacyConfigParser::extractQuotedPart()
{
	assert(*curr == '\"');

	curr++; // skip quote
	char * begin = curr;

	while (curr != end && *curr != '\"')
		curr++;

	return std::string(begin, curr++); //increment curr to close quote
}

std::string CLegacyConfigParser::extractQuotedString()
{
	assert(*curr == '\"');

	std::string ret;
	while (true)
	{
		ret += extractQuotedPart();

		if (curr < end && *curr == '\"') //double quote - add it to string and continue
			ret += '\"';
		else // end of string
			return ret;
	}
}

std::string CLegacyConfigParser::extractNormalString()
{
	char * begin = curr;

	while (curr < end && *curr != '\t' && *curr != '\r')//find end of string
		curr++;

	return std::string(begin, curr);
}

std::string CLegacyConfigParser::readString()
{
	if (curr >= end || *curr == '\n')
		return "";

	std::string ret;

	if (*curr == '\"')
		ret = extractQuotedString();// quoted text - find closing quote
	else
		ret = extractNormalString();//string without quotes - copy till \t or \r

	curr++;
	return ret;
}

float CLegacyConfigParser::readNumber()
{
	std::string input = readString();

	std::istringstream stream(input);

	if (input.find(',') != std::string::npos) // code to handle conversion with comma as decimal separator
		stream.imbue(std::locale(std::locale(), new LocaleWithComma));

	int result;
	if ( !(stream >> result) )
		return 0;
	return result;
}

bool CLegacyConfigParser::isNextEntryEmpty()
{
	return curr >= end || *curr == '\n' || *curr == '\r' || *curr == '\t';
}

bool CLegacyConfigParser::endLine()
{
	while (curr < end && *curr !=  '\n')
		readString();

	curr++;

	return curr < end;
}

void readToVector(std::string sourceName, std::vector<std::string> & dest)
{
	CLegacyConfigParser parser(sourceName);
	do
	{
		dest.push_back(parser.readString());
	}
	while (parser.endLine());
}

void CGeneralTextHandler::load()
{
	readToVector("DATA/VCDESC.TXT",   victoryConditions);
	readToVector("DATA/LCDESC.TXT",   lossCondtions);
	readToVector("DATA/TCOMMAND.TXT", tcommands);
	readToVector("DATA/HALLINFO.TXT", hcommands);
	readToVector("DATA/CASTINFO.TXT", fcommands);
	readToVector("DATA/ADVEVENT.TXT", advobtxt);
	readToVector("DATA/XTRAINFO.TXT", xtrainfo);
	readToVector("DATA/RESTYPES.TXT", restypes);
	readToVector("DATA/TERRNAME.TXT", terrainNames);
	readToVector("DATA/RANDSIGN.TXT", randsign);
	readToVector("DATA/ZCRGN1.TXT",   creGens);
	readToVector("DATA/CRGEN4.TXT",   creGens4);
	readToVector("DATA/OVERVIEW.TXT", overview);
	readToVector("DATA/ARRAYTXT.TXT", arraytxt);
	readToVector("DATA/PRISKILL.TXT", primarySkillNames);
	readToVector("DATA/JKTEXT.TXT",   jktexts);
	readToVector("DATA/TVRNINFO.TXT", tavernInfo);
	readToVector("DATA/TURNDUR.TXT",  turnDurations);
	readToVector("DATA/HEROSCRN.TXT", heroscrn);
	readToVector("DATA/ARTEVENT.TXT", artifEvents);
	readToVector("DATA/TENTCOLR.TXT", tentColors);
	readToVector("DATA/SKILLLEV.TXT", levels);
	readToVector("DATA/OBJNAMES.TXT", names);

	{
		CLegacyConfigParser parser("DATA/GENRLTXT.TXT");
		parser.endLine();
		do
		{
			allTexts.push_back(parser.readString());
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/ZELP.TXT");
		do
		{
			std::string first = parser.readString();
			std::string second = parser.readString();
			zelp.push_back(std::make_pair(first, second));
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/HEROSPEC.TXT");
		CLegacyConfigParser bioParser("DATA/HEROBIOS.TXT");

		//skip header
		parser.endLine();
		parser.endLine();

		do
		{
			HeroTexts texts;
			texts.bonusName  = parser.readString();
			texts.shortBonus = parser.readString();
			texts.longBonus  = parser.readString();
			texts.biography  = bioParser.readString();
			hTxts.push_back(texts);
		}
		while (parser.endLine() && bioParser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/BLDGNEUT.TXT");

		for(int i=0; i<15; i++)
		{
			std::string name  = parser.readString();
			std::string descr = parser.readString();
			parser.endLine();

			for(int j=0; j<GameConstants::F_NUMBER; j++)
			{
				buildings[j][i].first = name;
				buildings[j][i].second = descr;
			}
		}
		parser.endLine(); // silo
		parser.endLine(); // blacksmith  //unused entries
		parser.endLine(); // moat

		//shipyard with the ship
		std::string name  = parser.readString();
		std::string descr = parser.readString();
		parser.endLine();

		for(int j=0; j<GameConstants::F_NUMBER; j++)
		{
			buildings[j][20].first = name;
			buildings[j][20].second = descr;
		}

		//blacksmith
		for(int j=0; j<GameConstants::F_NUMBER; j++)
		{
			buildings[j][16].first =  parser.readString();
			buildings[j][16].second = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/BLDGSPEC.TXT");

		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			for(int build=0; build<9; build++)
			{
				buildings[town][17+build].first =  parser.readString();
				buildings[town][17+build].second = parser.readString();
				parser.endLine();
			}
			buildings[town][26].first =  parser.readString(); // Grail
			buildings[town][26].second = parser.readString();
			parser.endLine();

			buildings[town][15].first =  parser.readString(); // Resource silo
			buildings[town][15].second = parser.readString();
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/DWELLING.TXT");

		for(int town=0; town<GameConstants::F_NUMBER; town++)
		{
			for(int build=0; build<14; build++)
			{
				buildings[town][30+build].first =  parser.readString();
				buildings[town][30+build].second = parser.readString();
				parser.endLine();
			}
		}
	}
	{
		CLegacyConfigParser typeParser("DATA/TOWNTYPE.TXT");
		CLegacyConfigParser nameParser("DATA/TOWNNAME.TXT");
		do
		{
			townTypes.push_back(typeParser.readString());

			townNames.push_back(std::vector<std::string>());
			for (int i=0; i<GameConstants::NAMES_PER_TOWN; i++)
			{
				townNames.back().push_back(nameParser.readString());
				nameParser.endLine();
			}
		}
		while (typeParser.endLine());
	}
	{
		CLegacyConfigParser nameParser("DATA/MINENAME.TXT");
		CLegacyConfigParser eventParser("DATA/MINEEVNT.TXT");

		do
		{
			std::string name  = nameParser.readString();
			std::string event = eventParser.readString();
			mines.push_back(std::make_pair(name, event));
		}
		while (nameParser.endLine() && eventParser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/PLCOLORS.TXT");
		do
		{
			std::string color = parser.readString();
			colors.push_back(color);

			color[0] = toupper(color[0]);
			capColors.push_back(color);
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/SSTRAITS.TXT");

		//skip header
		parser.endLine();
		parser.endLine();

		do
		{
			skillName.push_back(parser.readString());

			skillInfoTexts.push_back(std::vector<std::string>());
			for(int j = 0; j < 3; j++)
				skillInfoTexts.back().push_back(parser.readString());
		}
		while (parser.endLine());
	}
	{
		CLegacyConfigParser parser("DATA/SEERHUT.TXT");

		//skip header
		parser.endLine();
		parser.endLine();

		for (int i = 0; i < 6; ++i)
			seerEmpty.push_back(parser.readString());

		quests.resize(10);
		for (int i = 0; i < 9; ++i) //9 types of quests
		{
			quests[i].resize(5);
			for (int j = 0; j < 5; ++j)
			{
				parser.readString(); //front description
				for (int k = 0; k < 6; ++k)
					quests[i][j].push_back(parser.readString());

				parser.endLine();
			}
		}
		quests[9].resize(1);

		for (int k = 0; k < 6; ++k) //Time limit
		{
			quests[9][0].push_back(parser.readString());
		}
		parser.endLine();

		parser.endLine(); // empty line
		parser.endLine(); // header

		for (int i = 0; i < 48; ++i)
		{
			seerNames.push_back(parser.readString());
			parser.endLine();
		}
	}
	{
		CLegacyConfigParser parser("DATA/CAMPTEXT.TXT");

		//skip header
		parser.endLine();

		std::string text;
		do
		{
			text = parser.readString();
			if (!text.empty())
				campaignMapNames.push_back(text);
		}
		while (parser.endLine() && !text.empty());

		for (size_t i=0; i<campaignMapNames.size(); i++)
		{
			do // skip empty space and header
			{
				text = parser.readString();
			}
			while (parser.endLine() && text.empty());

			campaignRegionNames.push_back(std::vector<std::string>());
			do
			{
				text = parser.readString();
				if (!text.empty())
					campaignRegionNames.back().push_back(text);
			}
			while (parser.endLine() && !text.empty());
		}
	}
	{
		CLegacyConfigParser parser("DATA/ZCREXP.TXT");
		parser.endLine();//header
		do
		{
			parser.readString(); //ignore 1st column with description
			zcrexp.push_back(parser.readString());
		}
		while (parser.endLine());
	}

	std::string buffer;
	std::ifstream ifs(CResourceHandler::get()->getResourceName(ResourceID("config/threatlevel.txt")), std::ios::binary);
	getline(ifs, buffer); //skip 1st line
	for (int i = 0; i < 13; ++i)
	{
		getline(ifs, buffer);
		threat.push_back(buffer);
	}
}

std::string CGeneralTextHandler::getTitle(const std::string & text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='{'));
	while ((text[i]!='}') && (i<text.length()))
		ret+=text[i++];
	return ret;
}

std::string CGeneralTextHandler::getDescr(const std::string & text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='}'));
	i+=2;
	while ((text[i]!='"') && (i<text.length()))
		ret+=text[i++];
	return ret;
}

CGeneralTextHandler::CGeneralTextHandler()
{

}
