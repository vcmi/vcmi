/*
 * innoextract.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "innoextract.h"

#ifdef ENABLE_INNOEXTRACT
#include "cli/extract.hpp"
#include "setup/version.hpp"
#endif

QString Innoextract::extract(QString installer, QString outDir, std::function<void (float percent)> cb)
{
	QString errorText{};

#ifdef ENABLE_INNOEXTRACT
	::extract_options o;
	o.extract = true;

	// standard settings
	o.gog_galaxy = true;
	o.codepage = 0U;
	o.output_dir = outDir.toStdString();
	o.extract_temp = true;
	o.extract_unknown = true;
	o.filenames.set_expand(true);

	o.preserve_file_times = true; // also correctly closes file -> without it: on Windows the files are not written completely

	try
	{
		process_file(installer.toStdString(), o, cb);
	}
	catch(const std::ios_base::failure & e)
	{
		errorText = tr("Stream error while extracting files!\nerror reason: ");
		errorText += e.what();
	}
	catch(const format_error & e)
	{
		errorText = e.what();
	}
	catch(const std::runtime_error & e)
	{
		errorText = e.what();
	}
	catch(const setup::version_error &)
	{
		errorText = tr("Not a supported Inno Setup installer!");
	}
#else
	errorText = tr("VCMI was compiled without innoextract support, which is needed to extract exe files!");
#endif

	return errorText;
}

QString Innoextract::getHashError(QString exeFile, QString binFile, QString exeFileOriginal, QString binFileOriginal)
{
	enum filetype
	{
		H3_COMPLETE, CHR
	};
	struct data
	{
		filetype type;
		std::string language;
		int exeSize;
		int binSize;
		std::string exe;
		std::string bin;
	};

	struct fileinfo {
		std::string hash;
		int size = 0;
	};
	
	std::vector<data> knownHashes = {
	//	{ H3_COMPLETE, "english", 973162040,          0, "7cf1ecec73e8c2f2c2619415cd16749be5641942", ""                                         }, // setup_homm_3_complete_4.0_(10665).exe
	//	{ H3_COMPLETE, "french",        ???,          0, "7e5a737c51530a1888033d188ab0635825ee622f", ""                                         }, // setup_homm_3_complete_french_4.0_(10665).exe
		{ H3_COMPLETE, "english",    822520, 1005040617, "66646a353b06417fa12c6384405688c84a315cc1", "c624e2071f4e35386765ab044ad5860ac245b7f4" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(28740).exe
		{ H3_COMPLETE, "french",     824960,  997305870, "072f1d4466ff16444d8c7949c6530448a9c53cfa", "9b6b451d2bd2f8b4be159e62fa6d32e87ee10455" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(french)_(28740).exe
		{ H3_COMPLETE, "polish",     822288,  849286313, "74ffde00156dd5a8e237668f87213387f0dd9c7c", "2523cf9943043ae100186f89e4ebf7c28be09804" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(polish)_(28740).exe
		{ H3_COMPLETE, "russian",    821608,  980398466, "88ccae41e66da58ba4ad62024d97dfe69084f825", "58f1b3c813a1953992bba1f9855c47d01c897db8" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(russian)_(28740).exe
		{ H3_COMPLETE, "english",    820288, 1006275333, "ca68adb8c2d8c6b3afa17a595ad70c2cec062b5a", "2715e10e91919d05377d39fd879d43f9f0cb9f87" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(3.2)_gog_0.1_(77075).exe
		{ H3_COMPLETE, "french",     822688,  998540653, "fbb300eeef52f5d81a571a178723b19313e3856d", "4f4d90ff2f60968616766237664744bc54754500" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(3.2)_gog_0.1_(french)_(77075).exe
		{ H3_COMPLETE, "polish",     819904,  851750601, "a413b0b9f3d5ca3e1a57e84a42de28c67d77b1a7", "fd9fe58bcbb8b442e8cfc299d90f1d503f281d40" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(3.2)_gog_0.1_(polish)_(77075).exe
		{ H3_COMPLETE, "russian",    819416,  981633128, "e84eedf62fe2e5f9171a7e1ce6e99315a09ce41f", "49cc683395c0cf80830bfa66e42bb5dfdb7aa124" }, // setup_heroes_of_might_and_magic_3_complete_4.0_(3.2)_gog_0.1_(russian)_(77075).exe
		{ CHR,         "english", 689384280,          0, "81f6e306103cca7dd413c4476eeda65eb790e9e6", ""                                         }, // setup_heroes_chronicles_2.0.0.38.exe
		{ CHR,         "english", 485694752,          0, "44e4fc2c38261a1c2a57d5198f44493210e8fc1a", ""                                         }, // setup_heroes_chronicles_chapter1_2.1.0.42.exe
		{ CHR,         "english", 493102840,          0, "b479a3272cf4b57a6b7fc499df5eafb624dcd6de", ""                                         }, // setup_heroes_chronicles_chapter2_2.1.0.43.exe
		{ CHR,         "english", 470364128,          0, "5ad36d822e1700c9ecf93b78652900a52518146b", ""                                         }, // setup_heroes_chronicles_chapter3_2.1.0.41.exe
		{ CHR,         "english", 469211296,          0, "5deb374a2e188ed14e8f74ad1284c45e46adf760", ""                                         }, // setup_heroes_chronicles_chapter4_2.1.0.42.exe
		{ CHR,         "english", 447497560,          0, "a6daa6ed56c840f3be7ad6ad920a2f9f2439acc8", ""                                         }, // setup_heroes_chronicles_chapter5_2.1.0.42.exe
		{ CHR,         "english", 447430456,          0, "93a42dd24453f36e7020afc61bca05b8461a3f04", ""                                         }, // setup_heroes_chronicles_chapter6_2.1.0.42.exe
		{ CHR,         "english", 481583720,          0, "d74b042015f3c5b667821c5d721ac3d2fdbf43fc", ""                                         }, // setup_heroes_chronicles_chapter7_2.1.0.42.exe
		{ CHR,         "english", 462976008,          0, "9039050e88b9dabcdb3ffa74b33e6aa86a20b7d9", ""                                         }, // setup_heroes_chronicles_chapter8_2.1.0.42.exe
		{ CHR,         "english", 500308840,          0, "bc96759fcd4098a846dc83c76e9175c2cb047d66", ""                                         }, // setup_heroes_chronicles_chapter_1_-_warlords_of_the_wasteland_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 503877256,          0, "e64e09b4d4a558c4b435f45dd5a5025812d2418f", ""                                         }, // setup_heroes_chronicles_chapter_2_-_conquest_of_the_underworld_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 484422880,          0, "c2c5c596367def0835c6401bcd0b8949892c74b6", ""                                         }, // setup_heroes_chronicles_chapter_3_-_masters_of_the_elements_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 479835408,          0, "5df9e25a20a91fd51c6ae109e8a652bbe33fb85b", ""                                         }, // setup_heroes_chronicles_chapter_4_-_clash_of_the_dragons_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 460531888,          0, "d2af77493045543945bea7c1d2da35c4c8c2d8a4", ""                                         }, // setup_heroes_chronicles_chapter_5_-_the_world_tree_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 462570376,          0, "143f778c640bbe719f27a6551c6f9732766ec8d1", ""                                         }, // setup_heroes_chronicles_chapter_6_-_the_fiery_moon_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 497063264,          0, "0b2784a07417245b1f8caa3f5f128bc1e3c1a18c", ""                                         }, // setup_heroes_chronicles_chapter_7_-_revolt_of_the_beastmasters_1.0_gog_v2_(83337).exe
		{ CHR,         "english", 477021344,          0, "bd7db8f8a31f06960f8997548b7ed5fd3bf7cab2", ""                                         }, // setup_heroes_chronicles_chapter_8_-_the_sword_of_frost_1.0_gog_v2_(83337).exe
	};

	auto doHash = [](QFile f){
		fileinfo tmp;

		if(f.open(QFile::ReadOnly)) {
			QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
			if(hash.addData(&f))
				tmp.hash = hash.result().toHex().toLower().toStdString();
			tmp.size = f.size();
		}

		return tmp;
	};

	fileinfo exeInfo;
	fileinfo binInfo;
	fileinfo exeInfoOriginal;
	fileinfo binInfoOriginal;

	exeInfo = doHash(QFile(exeFile));
	if(!binFile.isEmpty())
		binInfo = doHash(QFile(binFile));
	exeInfoOriginal = doHash(QFile(exeFileOriginal));
	if(!binFileOriginal.isEmpty())
		binInfoOriginal = doHash(QFile(binFileOriginal));

	if(exeInfo.hash.empty() || (!binFile.isEmpty() && binInfo.hash.empty()))
		return QString{}; // hashing not possible -> previous error is enough

	QString hashOutput = tr("SHA1 hash of provided files:\nExe (%n bytes):\n%1", "param is hash", exeInfo.size).arg(QString::fromStdString(exeInfo.hash));
	if(!binInfo.hash.empty())
		hashOutput += tr("\nBin (%n bytes):\n%1", "param is hash", binInfo.size).arg(QString::fromStdString(binInfo.hash));

	if((!exeInfoOriginal.hash.empty() && exeInfo.hash != exeInfoOriginal.hash) || (!binInfoOriginal.hash.empty() && !binFile.isEmpty() && !binFileOriginal.isEmpty() && binInfo.hash != binInfoOriginal.hash))
		return tr("Internal copy process failed. Enough space on device?\n\n%1").arg(hashOutput);

	QString foundKnown;
	QString exeLang;
	QString binLang;
	auto find = [exeInfo, binInfo](const data & d) { return (!d.exe.empty() && d.exe == exeInfo.hash) || (!d.bin.empty() && d.bin == binInfo.hash);};
	auto it = std::find_if(knownHashes.begin(), knownHashes.end(), find);
	while(it != knownHashes.end()){
		auto lang = QString::fromStdString((*it).language);
		foundKnown += "\n" + (exeInfo.hash == (*it).exe ? tr("Exe") : tr("Bin")) + " - " + lang;
		if(exeInfo.hash == (*it).exe)
			exeLang = lang;
		else
			binLang = lang;
		it = std::find_if(++it, knownHashes.end(), find);
	}

	if(!exeLang.isEmpty() && !binLang.isEmpty() && exeLang != binLang && !binFile.isEmpty())
		return tr("Language mismatch!\n%1\n\n%2").arg(foundKnown, hashOutput);
	else if((!exeLang.isEmpty() || !binLang.isEmpty()) && !binFile.isEmpty())
		return tr("Only one file known! Maybe files are corrupted? Please download again.\n%1\n\n%2").arg(foundKnown, hashOutput);
	else if(!exeLang.isEmpty() && binFile.isEmpty())
		return QString{};
	else if(!exeLang.isEmpty() && !binFile.isEmpty() && exeLang == binLang)
		return QString{};

	return tr("Unknown files! Maybe files are corrupted? Please download again.\n\n%1").arg(hashOutput);
}
