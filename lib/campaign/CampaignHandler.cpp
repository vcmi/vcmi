/*
 * CCampaignHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CampaignHandler.h"

#include "CampaignState.h"
#include "CampaignRegionsHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CCompressedStream.h"
#include "../filesystem/CMemoryStream.h"
#include "../filesystem/CBinaryReader.h"
#include "../filesystem/CZipLoader.h"
#include "../GameLibrary.h"
#include "../mapping/CMapHeader.h"
#include "../mapping/CMapService.h"
#include "../mapping/MapFormatSettings.h"
#include "../modding/CModHandler.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/TextOperations.h"

VCMI_LIB_NAMESPACE_BEGIN

void CampaignHandler::readCampaign(Campaign * ret, const std::vector<ui8> & input, const std::string & filename, const std::string & modName, const std::string & encoding)
{
	if (input.front() < uint8_t(' ')) // binary format
	{
		CMemoryStream stream(input.data(), input.size());
		CBinaryReader reader(&stream);

		readHeaderFromMemory(*ret, reader, filename, modName, encoding);
		ret->overrideCampaign();

		for(int g = 0; g < ret->numberOfScenarios; ++g)
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromMemory(reader, *ret);
		}
		ret->overrideCampaignScenarios();
	}
	else // text format (json)
	{
		JsonNode jsonCampaign(reinterpret_cast<const std::byte*>(input.data()), input.size(), filename);
		readHeaderFromJson(*ret, jsonCampaign, filename, modName, encoding);

		for(auto & scenario : jsonCampaign["scenarios"].Vector())
		{
			auto scenarioID = static_cast<CampaignScenarioID>(ret->scenarios.size());
			ret->scenarios[scenarioID] = readScenarioFromJson(scenario);
		}
	}
}

std::unique_ptr<Campaign> CampaignHandler::getHeader( const std::string & name)
{
	ResourcePath resourceID(name, EResType::CAMPAIGN);
	std::string modName = LIBRARY->modh->findResourceOrigin(resourceID);
	std::string encoding = LIBRARY->modh->findResourceEncoding(resourceID);
	
	auto ret = std::make_unique<Campaign>();
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);
	std::vector<ui8> cmpgn = getFile(std::move(fileStream), name, true)[0];

	readCampaign(ret.get(), cmpgn, resourceID.getName(), modName, encoding);

	return ret;
}

std::shared_ptr<CampaignState> CampaignHandler::getCampaign( const std::string & name )
{
	ResourcePath resourceID(name, EResType::CAMPAIGN);
	const std::string & modName = LIBRARY->modh->findResourceOrigin(resourceID);
	const std::string & encoding = LIBRARY->modh->findResourceEncoding(resourceID);
	
	auto ret = std::make_unique<CampaignState>();
	
	auto fileStream = CResourceHandler::get(modName)->load(resourceID);

	std::vector<std::vector<ui8>> files = getFile(std::move(fileStream), name, false);

	readCampaign(ret.get(), files[0], resourceID.getName(), modName, encoding);

	//first entry is campaign header. start loop from 1
	for(int scenarioIndex = 0, fileIndex = 1; fileIndex < files.size() && scenarioIndex < ret->numberOfScenarios; scenarioIndex++)
	{
		auto scenarioID = static_cast<CampaignScenarioID>(scenarioIndex);

		if(!ret->scenarios[scenarioID].isNotVoid()) //skip void scenarios
			continue;

		std::string scenarioName = resourceID.getName();
		boost::to_lower(scenarioName);
		scenarioName += ':' + std::to_string(fileIndex - 1);

		//set map piece appropriately, convert vector to string
		ret->mapPieces[scenarioID] = files[fileIndex];

		auto hdr = ret->getMapHeader(scenarioID);
		ret->scenarios[scenarioID].scenarioName = hdr->name;
		fileIndex++;
	}

	return ret;
}

std::string CampaignHandler::readLocalizedString(CampaignHeader & target, CBinaryReader & reader, const std::string & filename, const std::string & modName, const std::string & encoding, const std::string & identifier)
{
	const std::string & input = TextOperations::toUnicode(reader.readBaseString(), encoding);

	return readLocalizedString(target, input, filename, modName, identifier);
}

std::string CampaignHandler::readLocalizedString(CampaignHeader & target, const std::string & text, const std::string & filename, const std::string & modName, const std::string & identifier)
{
	TextIdentifier stringID( "campaign", TextOperations::convertMapName(filename), identifier);

	if (text.empty())
		return "";

	target.getTexts().registerString(modName, stringID, text);
	return stringID.get();
}

void CampaignHandler::readHeaderFromJson(CampaignHeader & ret, JsonNode & reader, const std::string & filename, const std::string & modName, const std::string & encoding)
{
	ret.version = static_cast<CampaignVersion>(reader["version"].Integer());
	if(ret.version < CampaignVersion::VCMI_MIN || ret.version > CampaignVersion::VCMI_MAX)
	{
		logGlobal->info("VCMP Loading: Unsupported campaign %s version %d", filename, static_cast<int>(ret.version));
		return;
	}
	
	ret.version = CampaignVersion::VCMI;
	ret.campaignRegions = CampaignRegions(reader["regions"]);
	ret.numberOfScenarios = reader["scenarios"].Vector().size();
	ret.name.appendTextID(readLocalizedString(ret, reader["name"].String(), filename, modName, "name"));
	ret.description.appendTextID(readLocalizedString(ret, reader["description"].String(), filename, modName, "description"));
	ret.author.appendRawString(reader["author"].String());
	ret.authorContact.appendRawString(reader["authorContact"].String());
	ret.campaignVersion.appendRawString(reader["campaignVersion"].String());
	ret.creationDateTime = reader["creationDateTime"].Integer();
	ret.difficultyChosenByPlayer = reader["allowDifficultySelection"].Bool();
	ret.music = AudioPath::fromJson(reader["music"]);
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
	ret.loadingBackground = ImagePath::fromJson(reader["loadingBackground"]);
	ret.videoRim = ImagePath::fromJson(reader["videoRim"]);
	ret.introVideo = VideoPath::fromJson(reader["introVideo"]);
	ret.outroVideo = VideoPath::fromJson(reader["outroVideo"]);
}

JsonNode CampaignHandler::writeHeaderToJson(CampaignHeader & header)
{
	JsonNode node;
	node["version"].Integer() = static_cast<ui64>(CampaignVersion::VCMI);
	node["regions"] = header.campaignRegions.toJson();
	node["name"].String() = header.name.toString();
	node["description"].String() = header.description.toString();
	node["author"].String() = header.author.toString();
	node["authorContact"].String() = header.authorContact.toString();
	node["campaignVersion"].String() = header.campaignVersion.toString();
	node["creationDateTime"].Integer() = header.creationDateTime;
	node["allowDifficultySelection"].Bool() = header.difficultyChosenByPlayer;
	node["music"].String() = header.music.getName();
	node["loadingBackground"].String() = header.loadingBackground.getName();
	node["videoRim"].String() = header.videoRim.getName();
	node["introVideo"].String() = header.introVideo.getName();
	node["outroVideo"].String() = header.outroVideo.getName();
	return node;
}

CampaignScenario CampaignHandler::readScenarioFromJson(JsonNode & reader)
{
	auto prologEpilogReader = [](JsonNode & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = !identifier.isNull();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = VideoPath::fromJson(identifier["video"]);
			ret.prologMusic = AudioPath::fromJson(identifier["music"]);
			ret.prologVoice = AudioPath::fromJson(identifier["voice"]);
			ret.prologText.jsonDeserialize(identifier["text"]);
		}
		return ret;
	};

	CampaignScenario ret;
	ret.mapName = reader["map"].String();
	for(auto & g : reader["preconditions"].Vector())
		ret.preconditionRegions.insert(static_cast<CampaignScenarioID>(g.Integer()));

	ret.regionColor = reader["color"].Integer();
	ret.difficulty = reader["difficulty"].Integer();
	ret.regionText.jsonDeserialize(reader["regionText"]);
	ret.prolog = prologEpilogReader(reader["prolog"]);
	ret.epilog = prologEpilogReader(reader["epilog"]);

	ret.travelOptions = readScenarioTravelFromJson(reader);

	return ret;
}

JsonNode CampaignHandler::writeScenarioToJson(const CampaignScenario & scenario)
{
	auto prologEpilogWriter = [](const CampaignScenarioPrologEpilog & elem) -> JsonNode
	{
		JsonNode node;
		if(elem.hasPrologEpilog)
		{
			node["video"].String() = elem.prologVideo.getName();
			node["music"].String() = elem.prologMusic.getName();
			node["voice"].String() = elem.prologVoice.getName();
			node["text"].String() = elem.prologText.toString();
		}
		return node;
	};

	JsonNode node;
	node["map"].String() = scenario.mapName;
	for(auto & g : scenario.preconditionRegions)
		node["preconditions"].Vector().push_back(JsonNode(g.getNum()));
	node["color"].Integer() = scenario.regionColor;
	node["difficulty"].Integer() = scenario.difficulty;
	node["regionText"].String() = scenario.regionText.toString();
	node["prolog"] = prologEpilogWriter(scenario.prolog);
	node["epilog"] = prologEpilogWriter(scenario.epilog);

	writeScenarioTravelToJson(node, scenario.travelOptions);

	return node;
}

static const std::map<std::string, CampaignStartOptions> startOptionsMap = {
	{"none", CampaignStartOptions::NONE},
	{"bonus", CampaignStartOptions::START_BONUS},
	{"crossover", CampaignStartOptions::HERO_CROSSOVER},
	{"hero", CampaignStartOptions::HERO_OPTIONS}
};

CampaignTravel CampaignHandler::readScenarioTravelFromJson(JsonNode & reader)
{
	CampaignTravel ret;
	
	for(auto & k : reader["heroKeeps"].Vector())
	{
		if(k.String() == "experience") ret.whatHeroKeeps.experience = true;
		if(k.String() == "primarySkills") ret.whatHeroKeeps.primarySkills = true;
		if(k.String() == "secondarySkills") ret.whatHeroKeeps.secondarySkills = true;
		if(k.String() == "spells") ret.whatHeroKeeps.spells = true;
		if(k.String() == "artifacts") ret.whatHeroKeeps.artifacts = true;
	}
	
	for(auto & k : reader["keepCreatures"].Vector())
	{
		if(auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "creature", k.String()))
			ret.monstersKeptByHero.insert(CreatureID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepCreatures contains unresolved identifier %s", k.String());
	}
	for(auto & k : reader["keepArtifacts"].Vector())
	{
		if(auto identifier = LIBRARY->identifiers()->getIdentifier(ModScope::scopeMap(), "artifact", k.String()))
			ret.artifactsKeptByHero.insert(ArtifactID(identifier.value()));
		else
			logGlobal->warn("VCMP Loading: keepArtifacts contains unresolved identifier %s", k.String());
	}

	ret.startOptions = startOptionsMap.at(reader["startOptions"].String());

	if (!reader["playerColor"].isNull())
		ret.playerColor = PlayerColor(PlayerColor::decode(reader["playerColor"].String()));

	for(auto & bjson : reader["bonuses"].Vector())
	{
		try {
			ret.bonusesToChoose.emplace_back(bjson, ret.startOptions);
		}
		catch (const std::exception &)
		{
			logGlobal->error("Failed to parse campaign bonus: %s", bjson.toCompactString());
			throw;
		}
	}

	return ret;
}

void CampaignHandler::writeScenarioTravelToJson(JsonNode & node, const CampaignTravel & travel)
{
	if(travel.whatHeroKeeps.experience)
		node["heroKeeps"].Vector().push_back(JsonNode("experience"));
	if(travel.whatHeroKeeps.primarySkills)
		node["heroKeeps"].Vector().push_back(JsonNode("primarySkills"));
	if(travel.whatHeroKeeps.secondarySkills)
		node["heroKeeps"].Vector().push_back(JsonNode("secondarySkills"));
	if(travel.whatHeroKeeps.spells)
		node["heroKeeps"].Vector().push_back(JsonNode("spells"));
	if(travel.whatHeroKeeps.artifacts)
		node["heroKeeps"].Vector().push_back(JsonNode("artifacts"));
	for(const auto & c : travel.monstersKeptByHero)
		node["keepCreatures"].Vector().push_back(JsonNode(CreatureID::encode(c)));
	for(const auto & a : travel.artifactsKeptByHero)
		node["keepArtifacts"].Vector().push_back(JsonNode(ArtifactID::encode(a)));
	
	node["startOptions"].String() = vstd::reverseMap(startOptionsMap)[travel.startOptions];

	if (travel.playerColor.isValidPlayer())
		node["playerColor"].String() = PlayerColor::encode(travel.playerColor.getNum());

	for (const auto & bonus : travel.bonusesToChoose)
		node["bonuses"].Vector().push_back(bonus.toJson());
}

void CampaignHandler::readHeaderFromMemory( CampaignHeader & ret, CBinaryReader & reader, const std::string & filename, const std::string & modName, const std::string & encoding )
{
	ret.version = static_cast<CampaignVersion>(reader.readUInt32());

	if (ret.version == CampaignVersion::HotA)
	{
		int32_t formatVersion = reader.readInt32();

		if (formatVersion == 2)
		{
			int hotaVersionMajor = reader.readUInt32();
			int hotaVersionMinor = reader.readUInt32();
			int hotaVersionPatch = reader.readUInt32();
			logGlobal->trace("Loading HotA campaign, version %d.%d.%d", hotaVersionMajor, hotaVersionMinor, hotaVersionPatch);

			bool forceMatchingVersion = reader.readBool();
			if (forceMatchingVersion)
				logGlobal->warn("Map '%s': This map is forced to use specific hota version!", filename);
		}

		[[maybe_unused]] int32_t unknownB = reader.readInt8();
		[[maybe_unused]] int32_t unknownC = reader.readInt32();
		ret.numberOfScenarios = reader.readInt32();

		assert(unknownB == 1);
		assert(unknownC == 0);
		assert(ret.numberOfScenarios <= 8);
	}

	const auto & mapping = LIBRARY->mapFormat->getMapping(ret.version);

	CampaignRegionID campaignMapId(reader.readUInt8());
	if(ret.version != CampaignVersion::Chr)
	{
		ret.campaignRegions = *LIBRARY->campaignRegions->getByIndex(mapping.remap(campaignMapId));
		if(ret.version != CampaignVersion::HotA)
			ret.numberOfScenarios = ret.campaignRegions.regionsCount();
	}
	ret.name.appendTextID(readLocalizedString(ret, reader, filename, modName, encoding, "name"));
	ret.description.appendTextID(readLocalizedString(ret, reader, filename, modName, encoding, "description"));
	ret.author.appendRawString("");
	ret.authorContact.appendRawString("");
	ret.campaignVersion.appendRawString("");
	ret.creationDateTime = 0;
	if (ret.version > CampaignVersion::RoE)
		ret.difficultyChosenByPlayer = reader.readBool();
	else
		ret.difficultyChosenByPlayer = false;

	ret.music = mapping.remapCampaignMusic(reader.readUInt8());
	logGlobal->trace("Campaign %s: map %d (%d scenarios), music theme: %s", filename, campaignMapId.getNum(), ret.numberOfScenarios, ret.music.getOriginalName());
	ret.filename = filename;
	ret.modName = modName;
	ret.encoding = encoding;
}

CampaignScenario CampaignHandler::readScenarioFromMemory( CBinaryReader & reader, CampaignHeader & header)
{
	const auto & mapping = LIBRARY->mapFormat->getMapping(header.version);

	auto prologEpilogReader = [&](const std::string & identifier) -> CampaignScenarioPrologEpilog
	{
		CampaignScenarioPrologEpilog ret;
		ret.hasPrologEpilog = reader.readBool();
		if(ret.hasPrologEpilog)
		{
			ret.prologVideo = mapping.remapCampaignVideo(reader.readUInt8());
			ret.prologMusic = mapping.remapCampaignMusic(reader.readUInt8());
			logGlobal->trace("Campaign %s, scenario %s: music theme: %s, video: %s", header.filename, identifier, ret.prologMusic.getOriginalName(), ret.prologVideo.getOriginalName());
			ret.prologText.appendTextID(readLocalizedString(header, reader, header.filename, header.modName, header.encoding, identifier));
		}
		return ret;
	};

	CampaignScenario ret;
	ret.mapName = reader.readBaseString();
	reader.readUInt32(); //packedMapSize - not used
	if(header.numberOfScenarios > 8) //unholy alliance
	{
		ret.loadPreconditionRegions(reader.readUInt16());
	}
	else
	{
		ret.loadPreconditionRegions(reader.readUInt8());
	}
	ret.regionColor = reader.readUInt8();
	ret.difficulty = reader.readUInt8();
	assert(ret.difficulty < 5);
	ret.regionText.appendTextID(readLocalizedString(header, reader, header.filename, header.modName, header.encoding, ret.mapName + ".region"));
	ret.prolog = prologEpilogReader(ret.mapName + ".prolog");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".prolog2");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".prolog3");
	ret.epilog = prologEpilogReader(ret.mapName + ".epilog");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".epilog2");
	if (header.version == CampaignVersion::HotA)
		prologEpilogReader(ret.mapName + ".epilog3");

	ret.travelOptions = readScenarioTravelFromMemory(reader, header.version);

	return ret;
}

template<typename Identifier>
static void readContainer(std::set<Identifier> & container, CBinaryReader & reader, const MapIdentifiersH3M & remapper, int sizeBytes)
{
	for(int iId = 0, byte = 0; iId < sizeBytes * 8; ++iId)
	{
		if(iId % 8 == 0)
			byte = reader.readUInt8();
		if(byte & (1 << iId % 8))
			container.insert(remapper.remap(Identifier(iId)));
	}
}

CampaignTravel CampaignHandler::readScenarioTravelFromMemory(CBinaryReader & reader, CampaignVersion version )
{
	CampaignTravel ret;

	ui8 whatHeroKeeps = reader.readUInt8();
	ret.whatHeroKeeps.experience = whatHeroKeeps & 1;
	ret.whatHeroKeeps.primarySkills = whatHeroKeeps & 2;
	ret.whatHeroKeeps.secondarySkills = whatHeroKeeps & 4;
	ret.whatHeroKeeps.spells = whatHeroKeeps & 8;
	ret.whatHeroKeeps.artifacts = whatHeroKeeps & 16;

	const auto & mapping = LIBRARY->mapFormat->getMapping(version);
	
	if (version == CampaignVersion::HotA)
	{
		readContainer(ret.monstersKeptByHero, reader, mapping, 24);
		readContainer(ret.artifactsKeptByHero, reader, mapping, 21);
	}
	else
	{
		readContainer(ret.monstersKeptByHero, reader, mapping, 19);
		readContainer(ret.artifactsKeptByHero, reader, mapping, version < CampaignVersion::SoD ? 17 : 18);
	}

	ret.startOptions = static_cast<CampaignStartOptions>(reader.readUInt8());

	if (ret.startOptions == CampaignStartOptions::START_BONUS)
		ret.playerColor.setNum(reader.readUInt8());

	if (ret.startOptions != CampaignStartOptions::NONE)
	{
		ui8 numOfBonuses = reader.readUInt8();
		for (int g=0; g<numOfBonuses; ++g)
			ret.bonusesToChoose.emplace_back(reader, mapping, ret.startOptions);
	}

	return ret;
}

std::vector< std::vector<ui8> > CampaignHandler::getFile(std::unique_ptr<CInputStream> file, const std::string & filename, bool headerOnly)
{
	std::array<ui8, 2> magic;
	file->read(magic.data(), magic.size());
	file->seek(0);

	std::vector< std::vector<ui8> > ret;

	static const std::array<ui8, 2> zipHeaderMagic{0x50, 0x4B};
	if (magic == zipHeaderMagic) // ZIP archive - assume VCMP format
	{
		CInputStream * buffer(file.get());
		auto ioApi = std::make_shared<CProxyROIOApi>(buffer);
		CZipLoader loader("", "_", ioApi);

		// load header
		JsonPath jsonPath = JsonPath::builtin(VCMP_HEADER_FILE_NAME);
		if(!loader.existsResource(jsonPath))
			throw std::runtime_error(jsonPath.getName() + " not found in " + filename);
		auto data = loader.load(jsonPath)->readAll();
		ret.emplace_back(data.first.get(), data.first.get() + data.second);

		if(headerOnly)
			return ret;

		// load scenarios
		JsonNode header(reinterpret_cast<const std::byte*>(data.first.get()), data.second, VCMP_HEADER_FILE_NAME);
		for(auto scenario : header["scenarios"].Vector())
		{
			ResourcePath mapPath(scenario["map"].String(), EResType::MAP);
			if(!loader.existsResource(mapPath))
				throw std::runtime_error(mapPath.getName() + " not found in " + filename);
			auto data = loader.load(mapPath)->readAll();
			ret.emplace_back(data.first.get(), data.first.get() + data.second);
		}

		return ret;
	}
	else // H3C
	{
		CCompressedStream stream(std::move(file), true);

		try
		{
			do
			{
				std::vector<ui8> block(stream.getSize());
				stream.read(block.data(), block.size());
				ret.push_back(block);
				ret.back().shrink_to_fit();
			}
			while (!headerOnly && stream.getNextBlock());
		}
		catch (const DecompressionException & e)
		{
			// Some campaigns in French version from gog.com have trailing garbage bytes
			// For example, slayer.h3c consist from 5 parts: header + 4 maps
			// However file also contains ~100 "extra" bytes after those 5 parts are decompressed that do not represent gzip stream
			// leading to exception "Incorrect header check"
			// Since H3 handles these files correctly, simply log this as warning and proceed
			logGlobal->warn("Failed to read file %s. Encountered error during decompression: %s", filename, e.what());
		}

		return ret;
	}
}

VCMI_LIB_NAMESPACE_END
