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

int ChroniclesExtractor::getChronicleNo(QFile & file)
{
	if(!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::critical(parent, tr("File cannot opened"), file.errorString());
		return 0;
	}

	QByteArray magic{"MZ"};
	QByteArray magicFile = file.read(magic.length());
	if(!magicFile.startsWith(magic))
	{
		QMessageBox::critical(parent, tr("Invalid file selected"), tr("You have to select an gog installer file!"));
		return 0;
	}

	QByteArray dataBegin = file.read(1'000'000);
	int chronicle = 0;
	for (const auto& kv : chronicles) {
		if(dataBegin.contains(kv.second))
		{
			chronicle = kv.first;
			break;
		}
	}
	if(!chronicle)
	{
		QMessageBox::critical(parent, tr("Invalid file selected"), tr("You have to select an chronicle installer file!"));
		return 0;
	}
	return chronicle;
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
		QMessageBox::critical(parent, tr("Extracting error!"), errorText);
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
		{ "author", "3DO" },
		{ "version", "1.0" },
		{ "contact", "vcmi.eu" },
	};

	QFile jsonFile(dir.filePath("mod.json"));
    jsonFile.open(QFile::WriteOnly);
    jsonFile.write(QJsonDocument(mod).toJson());
}

void ChroniclesExtractor::createChronicleMod(int no)
{
	QDir dir(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "Mods" / ("chronicles_" + std::to_string(no))));
	dir.removeRecursively();
	dir.mkpath(".");

	QByteArray tmpChronicles = chronicles.at(no);
	tmpChronicles.replace('\0', "");

	QJsonObject mod
	{
		{ "modType", "Expansion" },
		{ "name", QString::number(no) + " - " + QString(tmpChronicles) },
		{ "description", tr("Heroes Chronicles") + " - " + QString::number(no) + " - " + QString(tmpChronicles) },
		{ "author", "3DO" },
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
	QByteArray tmpChronicles = chronicles.at(no);
	tmpChronicles.replace('\0', "");

	std::string chroniclesDir = "chronicles_" + std::to_string(no);
	QDir tmpDir = tempDir.filePath(tempDir.entryList({"app"}, QDir::Filter::Dirs).front());
	tmpDir.setPath(tmpDir.filePath(tmpDir.entryList({QString(tmpChronicles)}, QDir::Filter::Dirs).front()));
	tmpDir.setPath(tmpDir.filePath(tmpDir.entryList({"data"}, QDir::Filter::Dirs).front()));
	auto basePath = VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "Mods" / chroniclesDir / "content";
	QDir outDirDataPortraits(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "chronicles" / "content" / "Data"));
	QDir outDirData(pathToQString(basePath / "Data" / chroniclesDir));
	QDir outDirSprites(pathToQString(basePath / "Sprites" / chroniclesDir));
	QDir outDirVideo(pathToQString(basePath / "Video" / chroniclesDir));
	QDir outDirSounds(pathToQString(basePath / "Sounds" / chroniclesDir));
	QDir outDirMaps(pathToQString(basePath / "Maps"));

	auto extract = [](QDir scrDir, QDir dest, QString file, std::vector<std::string> files = {}){
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

	if(!outDirMaps.exists())
		outDirMaps.mkpath(".");
	QString campaignFileName = "Hc" + QString::number(no) + "_Main.h3c";
	QFile(outDirData.filePath(outDirData.entryList({"Main.h3c"}).front())).copy(outDirMaps.filePath(campaignFileName));
}

void ChroniclesExtractor::installChronicles(QStringList exe)
{
	extractionFile = -1;
	fileCount = exe.size();
	for(QString f : exe)
	{
		extractionFile++;
		QFile file(f);

		int chronicleNo = getChronicleNo(file);
		if(!chronicleNo)
			continue;

		if(!createTempDir())
			continue;

		if(!extractGogInstaller(f))
			continue;
		
		createBaseMod();
		createChronicleMod(chronicleNo);

		removeTempDir();
	}
}
