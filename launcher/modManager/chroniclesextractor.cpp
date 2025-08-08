/*
 * chroniclesextractor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "chroniclesextractor.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/CArchiveLoader.h"

#include "../innoextract.h"
#include "../helper.h"

ChroniclesExtractor::ChroniclesExtractor(QWidget *p, std::function<void(float percent)> cb) :
	parent(p), cb(cb)
{
}

bool ChroniclesExtractor::createTempDir()
{
	tempDir = QDir(pathToQString(VCMIDirs::get().userDataPath()));
	if(tempDir.cd("tmp"))
	{
		tempDir.removeRecursively(); // remove if already exists (e.g. previous run)
		tempDir.cdUp();
	}
	tempDir.mkdir("tmp");
	if(!tempDir.cd("tmp"))
		return false; // should not happen - but avoid deleting wrong folder in any case

	return true;
}

void ChroniclesExtractor::removeTempDir()
{
	tempDir.removeRecursively();
}

std::vector<int> ChroniclesExtractor::getChronicleNo()
{
	// supports "All in one" and seperate installers
	QStringList appDirCandidates = tempDir.entryList({"app"}, QDir::Filter::Dirs);
	std::vector<int> tmp;

	if (!appDirCandidates.empty())
	{
		QDir appDir = tempDir.filePath(appDirCandidates.front());

		for (size_t i = 1; i < chronicles.size(); ++i)
		{
			QString chronicleName = chronicles.at(i);
			QStringList chroniclesDirCandidates = appDir.entryList({chronicleName}, QDir::Filter::Dirs);

			if (!chroniclesDirCandidates.empty())
				tmp.push_back(i);
		}
	}
	if(tmp.empty())
		QMessageBox::critical(parent, tr("Invalid file selected"), tr("You have to select a Heroes Chronicles installer file!"));
	return tmp;
}

bool ChroniclesExtractor::extractGogInstaller(QString file)
{
	QString errorText = Innoextract::extract(file, tempDir.path(), [this](float progress) {
		float overallProgress = ((1.0 / static_cast<float>(fileCount)) * static_cast<float>(extractionFile)) + (progress / static_cast<float>(fileCount));
		if(cb)
			cb(overallProgress);
	});

	if(!errorText.isEmpty())
	{
		logGlobal->error("Gog chronicles installer extraction failure! Reason: %s", errorText.toStdString());

		QString hashError = Innoextract::getHashError(file, {}, {}, {});
		QMessageBox::critical(parent, tr("Extracting error!"), errorText);
		if(!hashError.isEmpty())
		{
			logGlobal->error("Hash error: %s", hashError.toStdString());
			QMessageBox::critical(parent, tr("Hash error!"), hashError, QMessageBox::Ok, QMessageBox::Ok);
		}
		return false;
	}

	return true;
}

void ChroniclesExtractor::createBaseMod() const
{
	QDir dir(pathToQString(VCMIDirs::get().userDataPath() / "Mods"));
	dir.mkdir("chronicles");
	dir.cd("chronicles");
	dir.mkdir("Mods");

	QJsonObject mod
	{
		{ "modType", "Expansion" },
		{ "name", tr("Heroes Chronicles") },
		{ "description", tr("Heroes Chronicles") },
		{ "author", "New World Computing, 3DO" },
		{ "version", "1.0" },
		{ "contact", "vcmi.eu" },
		{ "heroes", QJsonArray({"config/portraitsChronicles.json"}) },
		{ "settings", QJsonObject({{"mapFormat", QJsonObject({{"chronicles", QJsonObject({{
			{"supported", true},
			{"portraits", QJsonObject({
				{"portraitTarnumBarbarian", 163},
				{"portraitTarnumKnight", 164},
				{"portraitTarnumWizard", 165},
				{"portraitTarnumRanger", 166},
				{"portraitTarnumOverlord", 167},
				{"portraitTarnumBeastmaster", 168},
			})},
		}})}})}})},
	};

	QFile jsonFile(dir.filePath("mod.json"));
	jsonFile.open(QFile::WriteOnly);
	jsonFile.write(QJsonDocument(mod).toJson());

	for(auto & dataPath : VCMIDirs::get().dataPaths())
	{
		auto file = pathToQString(dataPath / "config" / "heroes" / "portraitsChronicles.json");
		auto destFolder = VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "content" / "config";
		auto destFile = pathToQString(destFolder / "portraitsChronicles.json");
		if(QFile::exists(file))
		{
			QDir().mkpath(pathToQString(destFolder));
			QFile::remove(destFile);
			QFile::copy(file, destFile);
		}
	}
}

void ChroniclesExtractor::createChronicleMod(int no)
{
	QDir dir(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "Mods" / ("chronicles_" + std::to_string(no))));
	dir.removeRecursively();
	dir.mkpath(".");

	QString tmpChronicles = chronicles.at(no);

	QJsonObject mod
	{
		{ "modType", "Expansion" },
		{ "name", QString("%1 - %2").arg(no).arg(tmpChronicles) },
		{ "description", tr("Heroes Chronicles %1 - %2").arg(no).arg(tmpChronicles) },
		{ "author", "New World Computing, 3DO" },
		{ "version", "1.0" },
		{ "contact", "vcmi.eu" },
	};
	
	QFile jsonFile(dir.filePath("mod.json"));
	jsonFile.open(QFile::WriteOnly);
	jsonFile.write(QJsonDocument(mod).toJson());

	dir.cd("content");
	
	extractFiles(no);
}

void ChroniclesExtractor::extractFiles(int no) const
{
	QString tmpChronicles = chronicles.at(no);

	std::string chroniclesDir = "chronicles_" + std::to_string(no);
	QDir tmpDir = tempDir.filePath(tempDir.entryList({"app"}, QDir::Filter::Dirs).front());

	if(!tmpDir.entryList({"data"}, QDir::Filter::Dirs).size()) // gog installer V2 has data and other folders outside "app" folder
		tmpDir.cdUp();

	tmpDir.setPath(tmpDir.filePath(tmpDir.entryList({QString(tmpChronicles)}, QDir::Filter::Dirs).front()));
	tmpDir.setPath(tmpDir.filePath(tmpDir.entryList({"data"}, QDir::Filter::Dirs).front()));
	auto basePath = VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "Mods" / chroniclesDir / "content";
	QDir outDirDataPortraits(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "content" / "Data"));
	QDir outDirData(pathToQString(basePath / "Data" / chroniclesDir));
	QDir outDirSprites(pathToQString(basePath / "Sprites" / chroniclesDir));
	QDir outDirVideo(pathToQString(basePath / "Video" / chroniclesDir));
	QDir outDirSounds(pathToQString(basePath / "Sounds" / chroniclesDir));
	QDir outDirMaps(pathToQString(basePath / "Maps" / "Chronicles"));

	auto extract = [](QDir scrDir, QDir dest, QString file, std::vector<std::string> files = {}){
		if(scrDir.entryList({file}).isEmpty())
			return; // file does not exists (needed for "All in one" installer)

		CArchiveLoader archive("", scrDir.filePath(scrDir.entryList({file}).front()).toStdString(), false);
		for(auto & entry : archive.getEntries())
			if(files.empty())
				archive.extractToFolder(dest.absolutePath().toStdString(), "", entry.second, true);
			else
			{
				for(const auto & item : files)
					if(boost::algorithm::to_lower_copy(entry.second.name).find(boost::algorithm::to_lower_copy(item)) != std::string::npos)
						archive.extractToFolder(dest.absolutePath().toStdString(), "", entry.second, true);
			}
	};

	extract(tmpDir, outDirData, "xBitmap.lod");
	extract(tmpDir, outDirData, "xlBitmap.lod");
	extract(tmpDir, outDirSprites, "xSprite.lod");
	extract(tmpDir, outDirSprites, "xlSprite.lod");
	extract(tmpDir, outDirVideo, "xVideo.vid");
	extract(tmpDir, outDirSounds, "xSound.snd");

	tmpDir.cdUp();
	if(tmpDir.entryList({"maps"}, QDir::Filter::Dirs).size()) // special case for "The World Tree": the map is in the "Maps" folder instead of inside the lod
	{
		QDir tmpDirMaps = tmpDir.filePath(tmpDir.entryList({"maps"}, QDir::Filter::Dirs).front());
		for(const auto & entry : tmpDirMaps.entryList())
			QFile(tmpDirMaps.filePath(entry)).copy(outDirData.filePath(entry));
	}

	tmpDir.cdUp();
	QDir tmpDirData = tmpDir.filePath(tmpDir.entryList({"data"}, QDir::Filter::Dirs).front());
	auto tarnumPortraits = std::vector<std::string>{"HPS137", "HPS138", "HPS139", "HPS140", "HPS141", "HPS142", "HPL137", "HPL138", "HPL139", "HPL140", "HPL141", "HPL142"};
	extract(tmpDirData, outDirDataPortraits, "bitmap.lod", tarnumPortraits);
	extract(tmpDirData, outDirData, "lbitmap.lod", std::vector<std::string>{"INTRORIM"});

	// special case - "All in one" installer
	{
		tmpDir.cdUp();
		auto mapping = std::map<std::string, int>{{ {"Intro", 1}, {"Intr2", 2}, {"Intr3", 3}, {"Intr4", 4}, {"Intro5", 7}, {"Intro6", 8} }};
		std::vector<std::string> videoFiles;
		for(auto & elem : mapping)
			for(const auto & ending : {".bik", ".smk"})
				videoFiles.push_back(elem.first + ending);
		extract(tmpDirData, tmpDir, "Hchron.vid", videoFiles);
		for(auto & ending : {".bik", ".smk"})
		{
			if(!vstd::reverseMap(mapping).count(no))
				continue;
			auto srcName = vstd::reverseMap(mapping).at(no);
			auto dstName = (no == 7 || no == 8) ? srcName : "Intro";
			QFile::copy(tmpDir.filePath(QString::fromStdString(srcName + ending)), outDirVideo.filePath(QString::fromStdString(dstName + ending)));
		}
	}

	if(!outDirMaps.exists())
		outDirMaps.mkpath(".");
	QString campaignFileName = "Hc" + QString::number(no) + "_Main.h3c";
	QFile(outDirData.filePath(outDirData.entryList({"Main.h3c"}).front())).copy(outDirMaps.filePath(campaignFileName));
}

void ChroniclesExtractor::installChronicles(QStringList exe)
{
	logGlobal->info("Installing Chronicles");

	extractionFile = -1;
	fileCount = exe.size();
	for(QString f : exe)
	{
		extractionFile++;

		logGlobal->info("Creating temporary directory");
		if(!createTempDir())
			continue;
		
		// FIXME: this is required at the moment for Android (and possibly iOS)
		// Incoming file names are in content URI form, e.g. content://media/internal/chronicles.exe
		// Qt can handle those like it does regular files
		// however, innoextract fails to open such files
		// so make a copy in directory to which vcmi always has full access and operate on it
		QString filepath = tempDir.filePath("chr.exe");
		logGlobal->info("Copying offline installer from '%s' to '%s'", f.toStdString(), filepath.toStdString());

		Helper::performNativeCopy(f, filepath);
		QFile file(filepath);

		logGlobal->info("Extracting offline installer");
		if(!extractGogInstaller(filepath))
			continue;

		logGlobal->info("Detecting Chronicles");
		auto chronicleNo = getChronicleNo();
		if(chronicleNo.empty())
			continue;

		logGlobal->info("Creating base Chronicle mod");
		createBaseMod();

		for(const auto & no : chronicleNo)
		{
			logGlobal->info("Creating Chronicle mod (%i)", no);
			createChronicleMod(no);
		}

		logGlobal->info("Removing temporary directory");
		removeTempDir();
	}

	logGlobal->info("Chronicles installed");
}
