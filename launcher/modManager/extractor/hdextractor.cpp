/*
 * hdsextractor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../../StdInc.h"

#include "hdextractor.h"
#include "sd_data.h"

#include "../../lib/dds/dds.hpp"

#define BCDEC_IMPLEMENTATION
namespace bcdec {
	#include "../../lib/bcdec/bcdec.h"
}

#include "../../../lib/VCMIDirs.h"
#include "../../../lib/filesystem/Filesystem.h"
#include "../../../lib/filesystem/CArchiveLoader.h"
#include "../../../lib/filesystem/CZipSaver.h"
#include "../../../lib/filesystem/CMemoryBuffer.h"

HdExtractor::HdExtractor(QWidget *p, std::function<void(float percent)> cb) :
	parent(p), cb(cb)
{
}

bool HdExtractor::createTempDir()
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

void HdExtractor::removeTempDir()
{
	tempDir.removeRecursively();
}

void HdExtractor::installHd(QDir path)
{
	baseDir = path;

	if(!createTempDir())
		return;

	extract();

	QDir src(QDir::cleanPath(tempDir.absolutePath() + QDir::separator() + "hd-version"));
	QDir dst(pathToQString(VCMIDirs::get().userDataPath() / "Mods" / "hd-version"));
	dst.removeRecursively();
	src.rename(src.absolutePath(), dst.absolutePath());

	removeTempDir();
}

void HdExtractor::extract()
{
	std::string language = "";
	auto folderList = QDir(QDir::cleanPath(baseDir.absolutePath() + QDir::separator() + "data/LOC")).entryList(QDir::Filter::Dirs);
	for(auto lng : LANGUAGES)
		for(auto folder : folderList)
			if(lng.first == folder.toStdString())
				language = lng.first;

	auto outputDir = QDir::cleanPath(tempDir.absolutePath() + QDir::separator() + "hd-version");

	std::vector<SubModType> modTypes = {X2, X3, LOC_X2, LOC_X3};
	int count = 0;
	tbb::parallel_for_each(modTypes.begin(), modTypes.end(), [&](SubModType & t)
		{
			ModGenerator mg(outputDir, LANGUAGES[language], t);
			mg.loadFlagData(baseDir);
			if(t == X2) //only one thread needs to create
				mg.createModJson(std::nullopt, outputDir, language);

			for(auto & type : {ArchiveType::BITMAP_X2, ArchiveType::BITMAP_X3, ArchiveType::SPRITE_X2, ArchiveType::SPRITE_X3, ArchiveType::BITMAP_LOC_X2, ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3})
			{
				if(archiveTypeToSubModType(type) != t)
					continue;

				QFile fileName;
				if(vstd::contains({ArchiveType::BITMAP_LOC_X2, ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3}, type))
					fileName.setFileName(baseDir.filePath(QString("data/") + QString::fromStdString(language) + QString::fromStdString(PAKPATH[type])));
				else
					fileName.setFileName(baseDir.filePath(QString("data/") + QString::fromStdString(PAKPATH[type])));

				decodePac(fileName, [&, this](QString groupName, QString imageConfig, std::vector<QByteArray> & data){
					std::vector<QImage> sheets;
					for(auto & d : data)
						sheets.push_back(loadDds(d));
					std::vector<QString> imgNames;

					if(!mg.filter(groupName, type))
						return;

					cropImages(sheets, imageConfig, [&](QString imageName, QImage & img, int * sdOffset, bool isShadow){
						{
							const std::lock_guard<std::mutex> lock(mutex);
							count++;
							if(count % 50 == 0)
								cb(static_cast<float>(count) / static_cast<float>(FILES));
						}

						bool isSprite = vstd::contains({ArchiveType::SPRITE_X2, ArchiveType::SPRITE_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3}, type);
						if(isSprite)
							img = mg.resizeSprite(img, groupName, imageName, type, sdOffset);
						mg.addFile(groupName, imageName + (isShadow ? "-shadow" : ""), img, type);

						if(!isShadow)
						{
							bool needsBorder = vstd::contains({"CABEHE", "CADEVL", "CAELEM", "CALIZA", "CAMAGE", "CANGEL", "CAPEGS", "CBASIL", "CBDRGN", "CBDWAR", "CBEHOL", "CBGOG", "CBKNIG", "CBLORD", "CBTREE", "CBWLFR", "CCAVLR", "CCENTR", "CCERBU", "CCGORG", "CCHAMP", "CCHYDR", "CCMCOR", "CCRUSD", "CCYCLLOR", "CCYCLR", "CDDRAG", "CDEVIL", "CDGOLE", "CDRFIR", "CDRFLY", "CDWARF", "CECENT", "CEELEM", "CEFREE", "CEFRES", "CELF", "CEVEYE", "CFAMIL", "CFELEM", "CGARGO", "CGBASI", "CGDRAG", "CGENIE", "CGGOLE", "CGNOLL", "CGNOLM", "CGOBLI", "CGOG", "CGRELF", "CGREMA", "CGREMM", "CGRIFF", "CGTITA", "CHALBD", "CHARPH", "CHARPY", "CHCBOW", "CHDRGN", "CHGOBL", "CHHOUN", "CHYDRA", "CIGOLE", "CIMP", "CITROG", "CLCBOW", "CLICH", "CLTITA", "CMAGE", "CMAGOG", "CMCORE", "CMEDUQ", "CMEDUS", "CMINOK", "CMINOT", "CMONKK", "CNAGA", "CNAGAG", "CNDRGN", "CNOSFE", "COGARG", "COGMAG", "COGRE", "COHDEM", "CORCCH", "CORC", "CPEGAS", "CPFIEN", "CPFOE", "CPKMAN", "CPLICH", "CPLIZA", "CRANGL", "CRDRGN", "CRGRIF", "CROC", "CSGOLE", "CSKELE", "CSULTA", "CSWORD", "CTBIRD", "CTHDEM", "CTREE", "CTROGL", "CUNICO", "CUWLFR", "CVAMP", "CWELEM", "CWIGHT", "CWRAIT", "CWSKEL", "CWUNIC", "CWYVER", "CWYVMN", "CYBEHE", "CZEALT", "CZOMBI", "CZOMLO"}, boost::to_upper_copy(groupName.toStdString()));
							if(needsBorder)
							{
								auto imgBorder = mg.getBorder(img, qRgb(255, 255, 255), 2);
								mg.addFile(groupName, imageName + "-overlay", imgBorder, type);
							}

							int scaleFactor = vstd::contains({ArchiveType::BITMAP_X2, ArchiveType::BITMAP_LOC_X2, ArchiveType::SPRITE_X2, ArchiveType::SPRITE_LOC_X2}, type) ? 2 : 3;
							QImage flagImage = mg.addFlags(img, groupName, imageName, scaleFactor);
							if(!flagImage.isNull())
								mg.addFile(groupName, imageName + "-overlay", flagImage, type);

							if(boost::to_upper_copy(groupName.toStdString()) == "AVXPRSN0") // prision needs empty overlay
							{
								QImage tmpImg(img.width(), img.height(), QImage::Format_RGBA8888);
								tmpImg.fill(qRgba(0, 0, 0, 0));
								mg.addFile(groupName, imageName + "-overlay", tmpImg, type);
							}
						}

						if(!vstd::contains(imgNames, imageName))
							imgNames.push_back(imageName);
					});
					mg.createAnimationJson(groupName, imgNames, type);
				});
			}
		});
}

void HdExtractor::cropImages(std::vector<QImage> & images, QString imageParam, std::function<void(QString, QImage &, int[2], bool)> cb)
{
	QTextStream stream(&imageParam);
	QString line{};

	while (stream.readLineInto(&line)) {
		QStringList tmp = line.split(" ");
		if(tmp.length() > 11)
		{
			QString name = tmp[0];
			int no = tmp[1].toInt();
			int xOffsetSd = tmp[2].toInt();
			//tmp[3] -> unknown
			int yOffsetSd = tmp[4].toInt();
			//tmp[5] -> unknown
			int xOffset = tmp[6].toInt();
			int yOffset = tmp[7].toInt();
			int width = tmp[8].toInt();
			int height = tmp[9].toInt();
			int rotation = tmp[10].toInt();
			int hasShadow = tmp[11].toInt();

			QTransform transform;
			transform.rotate(90.0 * rotation);
			auto image = images[no].copy(QRect(xOffset, yOffset, width, height));
			image = image.transformed(transform);

			if(cb)
				cb(name, image, new int[2]{xOffsetSd, yOffsetSd}, false);

			if(hasShadow == 1)
			{
				int noShadow = tmp[12].toInt();
				int xOffsetShadow = tmp[13].toInt();
				int yOffsetShadow = tmp[14].toInt();
				int widthShadow = tmp[15].toInt();
				int heightShadow = tmp[16].toInt();
				int rotationShadow = tmp[17].toInt();

				QTransform transformShadow;
				transformShadow.rotate(90.0 * rotationShadow);
				auto imageShadow = images[noShadow];
				imageShadow = imageShadow.copy(QRect(xOffsetShadow, yOffsetShadow, widthShadow, heightShadow));
				imageShadow = imageShadow.transformed(transformShadow);

				if(cb)
					cb(name, imageShadow, new int[2]{xOffsetSd, yOffsetSd}, true);
			}
		}
	}
}

void HdExtractor::decodePac(QFile & file, std::function<void(QString, QString, std::vector<QByteArray> &)> cb)
{
	std::vector<QByteArray> outData;

	if(file.open(QIODevice::ReadOnly))
	{
		QDataStream in(&file);
		in.setByteOrder(QDataStream::LittleEndian);
		
		int32_t infoOffset;
		int32_t fileCount;

		file.seek(file.pos() + 4); // dummy
		in >> infoOffset;
		file.seek(infoOffset);
		in >> fileCount;

		int32_t nameOffset = file.pos();
		for(int i = 0; i < fileCount; i++)
		{
			QString name;
			int32_t offset;
			int32_t imageConfigSize;
			int32_t chunks;
			int32_t zsize;
			int32_t size;
			QString imageConfig;
			QByteArray data;
			QByteArray dataCompressed;
			std::vector<QByteArray> dataCompressedArray;

			file.seek(nameOffset);
			char tmpName[8];
			in.readRawData(tmpName, 8);
			name = QString(tmpName);
			file.seek(file.pos() + 12); // dummy
			in >> offset >> imageConfigSize >> chunks >> zsize >> size;
			std::vector<int32_t> chunkZsizeArray;
			for(int j = 0; j < chunks; j++)
			{
				int32_t chunkZsize;
				in >> chunkZsize;
				chunkZsizeArray.push_back(chunkZsize);
			}
			std::vector<int32_t> chunkSizeArray;
			for(int j = 0; j < chunks; j++)
			{
				int32_t chunkSize;
				in >> chunkSize;
				chunkSizeArray.push_back(chunkSize);
			}
			nameOffset = file.pos();

			file.seek(offset);
			char tmpImageConfig[imageConfigSize];
			in.readRawData(tmpImageConfig, imageConfigSize);
			imageConfig = QString(tmpImageConfig);
			offset += imageConfigSize;

			for(int j = 0; j < chunks; j++)
			{
				file.seek(offset);
				QByteArray tmpData;
				if(chunkZsizeArray[j] == chunkSizeArray[j])
				{
					tmpData.resize(chunkSizeArray.back());
					in.readRawData(tmpData.data(), chunkSizeArray.back());
					data += tmpData;
				}
				else
				{
					tmpData.resize(zsize);
					in.readRawData(tmpData.data(), zsize);
					dataCompressed += tmpData;
				}
				offset += chunkZsizeArray.back();
			}

			int dataCompressedLength = dataCompressed.length();
			while(dataCompressed.length() > 0)
			{
				auto tmpData = qUncompress(QByteArray(4, 0) + dataCompressed);
				if(tmpData.length() == 0)
					break;
				dataCompressedArray.push_back(tmpData);
				int len = qCompress(tmpData).length() - 4; // hack to get length of compressed zlib data
				dataCompressed.remove(0, len);
				if(dataCompressed.length() == 0 || dataCompressed.at(0) != 0x78) // check for zlib magic
					break;
			}

			if(data.length() < dataCompressedLength)
				outData = dataCompressedArray;
			else
				outData.push_back(data);

			if(cb)
				cb(name, imageConfig, outData);
		}
	}
}

QImage HdExtractor::loadDds(QByteArray & data)
{
	dds::Image image;
	if(dds::readImage((uint8_t*)data.data(), data.length(), &image) == dds::ReadResult::Success)
	{
		assert(image.format == DXGI_FORMAT_BC1_UNORM || image.format == DXGI_FORMAT_BC3_UNORM); // Heroes HD should use only this variant
		assert(image.mipmaps.size() == 1);

		auto src = image.mipmaps[0].data();
		uchar *uncompData = new uchar[image.width * image.height * 4];

		for(int i = 0; i < image.height; i += 4) {
			for(int j = 0; j < image.width; j += 4) {
				auto dst = uncompData + (i * image.width + j) * 4;
				if(image.format == DXGI_FORMAT_BC1_UNORM)
				{
					bcdec::bcdec_bc1(src, dst, image.width * 4);
					src += BCDEC_BC1_BLOCK_SIZE;
				}
				else
				{
					bcdec::bcdec_bc3(src, dst, image.width * 4);
					src += BCDEC_BC3_BLOCK_SIZE;
				}
			}
		}

		QImage img = QImage(uncompData, image.width, image.height, image.width * 4, QImage::Format::Format_RGBA8888).copy();
		delete[] uncompData;
		return img;
	}

	return QImage{};
}

HdExtractor::SubModType HdExtractor::archiveTypeToSubModType(ArchiveType v)
{
	SubModType subModType;
	if(vstd::contains({ArchiveType::BITMAP_X2, ArchiveType::SPRITE_X2}, v))
		subModType = SubModType::X2;
	else if(vstd::contains({ArchiveType::BITMAP_X3, ArchiveType::SPRITE_X3}, v))
		subModType = SubModType::X3;
	else if(vstd::contains({ArchiveType::BITMAP_LOC_X2, ArchiveType::SPRITE_LOC_X2}, v))
		subModType = SubModType::LOC_X2;
	else if(vstd::contains({ArchiveType::BITMAP_LOC_X3, ArchiveType::SPRITE_LOC_X3}, v))
		subModType = SubModType::LOC_X3;

	return subModType;
};

HdExtractor::ModGenerator::ModGenerator(QDir outputDir, std::string language, SubModType subModType) :
	language(language), sdData(getSdData())
{
	std::string suffix = (language.empty() || subModType == SubModType::X2 || subModType == SubModType::X3) ? "" : "_" + language;
	QDir modPath = QDir(outputDir.filePath(QString("mods/") + QString::fromStdString(SUBMODNAMES[subModType] + suffix)));
	QFileInfo zipFile(modPath.absolutePath() + QString("/content.zip"));
	QDir(zipFile.absolutePath()).mkpath(".");
	std::shared_ptr<CIOApi> io(new CDefaultIOApi());
	saver = std::make_shared<CZipSaver>(io, zipFile.absoluteFilePath().toStdString());

	createModJson(subModType, modPath, language);
}

void HdExtractor::ModGenerator::addFile(QString groupName, QString imageName, QImage & img, ArchiveType type)
{
	auto subModType = archiveTypeToSubModType(type);
	bool isSprite = vstd::contains({ArchiveType::SPRITE_X2, ArchiveType::SPRITE_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3}, type);
	std::string folder = isSprite ? "sprites" : "data";
	folder += vstd::contains({SubModType::X2, SubModType::LOC_X2}, subModType) ? "2x/" : "3x/";
	if(isSprite)
		folder += groupName.toStdString() + "/";

	QByteArray imgData;
	QBuffer buffer(&imgData);
	buffer.open(QIODevice::WriteOnly);
	img.save(&buffer, "png", 50);

	auto stream = saver->addFile(folder + imageName.toStdString() + ".png");
	stream->write(reinterpret_cast<const ui8 *>(imgData.data()), imgData.size());
}

void HdExtractor::ModGenerator::createModJson(std::optional<SubModType> submodType, QDir path, std::string language)
{
	if (auto result = submodType) {
		std::string scale = (*result == SubModType::X2 || *result == SubModType::LOC_X2) ? "2" : "3";
		bool isTranslation = (*result == SubModType::LOC_X2 || *result == SubModType::LOC_X3);
		QJsonObject mod;
		if(isTranslation)
		{
			mod = QJsonObject({
				{ "modType", "Translation" },
				{ "name", QString::fromStdString("HD Localisation (" + language + ") (x" + scale + ")") },
				{ "description", QString::fromStdString("Translated Resources (x" + scale + ")") },
				{ "author", "Ubisoft" },
				{ "version", "1.0" },
				{ "contact", "vcmi.eu" },
				{ "language", QString::fromStdString(language) },
			});
		}
		else
		{
			mod = QJsonObject({
				{ "modType", "Graphical" },
				{ "name", QString::fromStdString("HD (x" + scale + ")") },
				{ "description", QString::fromStdString("Resources (x" + scale + ")") },
				{ "author", "Ubisoft" },
				{ "version", "1.0" },
				{ "contact", "vcmi.eu" },
			});
		}
		
		QFile jsonFile(path.filePath("mod.json"));
		jsonFile.open(QFile::WriteOnly);
		jsonFile.write(QJsonDocument(mod).toJson());
	}
	else
	{
		QJsonObject mod
		{
			{ "modType", "Graphical" },
			{ "name", "Heroes HD (official)" },
			{ "description", "Extracted resources from official Heroes HD to make it usable on VCMI" },
			{ "author", "Ubisoft" },
			{ "version", "1.0" },
			{ "contact", "vcmi.eu" },
		};
		
		QFile jsonFile(path.filePath("mod.json"));
		jsonFile.open(QFile::WriteOnly);
		jsonFile.write(QJsonDocument(mod).toJson());
	}
}

inline void swap(QJsonValueRef v1, QJsonValueRef v2)
{
	QJsonValue temp(v1);
	v1 = QJsonValue(v2);
	v2 = temp;
};

void HdExtractor::ModGenerator::createAnimationJson(QString name, std::vector<QString> files, ArchiveType type)
{
	auto subModType = archiveTypeToSubModType(type);
	bool isSprite = vstd::contains({ArchiveType::SPRITE_X2, ArchiveType::SPRITE_X3, ArchiveType::SPRITE_LOC_X2, ArchiveType::SPRITE_LOC_X3}, type);
	if(!isSprite)
		return;

	std::sort(files.begin(), files.end());

	QJsonArray entries;
	for(auto & file : files)
	{
		if(sdData.count(boost::to_upper_copy(name.toStdString())))
		{
			for(auto & sdFrame : sdData[name.toStdString()])
			{
				if(boost::to_upper_copy(file.toStdString()) == sdFrame[0])
					entries.push_back(QJsonObject{
						{ "group", QString::fromStdString(sdFrame[1]).toInt() },
						{ "frame", QString::fromStdString(sdFrame[2]).toInt() },
						{ "file", file + ".png" },
					});
			}
		}
	}

	std::sort(entries.begin(), entries.end(), [](const QJsonValue &v1, const QJsonValue &v2) {
		if(v1.toObject()["group"].toInt() == v2.toObject()["group"].toInt())
			return v1.toObject()["frame"].toInt() < v2.toObject()["frame"].toInt();
		return v1.toObject()["group"].toInt() < v2.toObject()["group"].toInt();
	});


	QJsonObject mod
	{
		{ "basepath", name + "/" },
		{ "images", entries },
	};

	auto data = QJsonDocument(mod).toJson();
	auto stream = saver->addFile((vstd::contains({SubModType::X2, SubModType::LOC_X2}, subModType) ? "sprites2x/" : "sprites3x/") + name.toStdString() + ".json");
	stream->write(reinterpret_cast<const ui8 *>(data.data()), data.size());
}

QImage HdExtractor::ModGenerator::resizeSprite(QImage & img, QString groupName, QString imageName, ArchiveType type, int * sdOffsets)
{
	auto subModType = archiveTypeToSubModType(type);
	int scaleFactor = vstd::contains({SubModType::X2, SubModType::LOC_X2}, subModType) ? 2 : 3;

	if(sdData.count(boost::to_upper_copy(groupName.toStdString())))
	{
		for(auto & sdFrame : sdData[groupName.toStdString()])
		{
			if(boost::to_upper_copy(imageName.toStdString()) == sdFrame[0])
			{
				int fullWidth = std::stoi(sdFrame[4]) * scaleFactor;
				int fullHeight = std::stoi(sdFrame[5]) * scaleFactor;
				//int width = std::stoi(sdFrame[6]);
				//int height = std::stoi(sdFrame[7]);
				int leftMargin = std::stoi(sdFrame[8]);
				int topMargin = std::stoi(sdFrame[9]);

				QImage tmpImg(fullWidth, fullHeight, QImage::Format_RGBA8888);
				tmpImg.fill(qRgba(0, 0, 0, 0));
				QPainter p(&tmpImg);
				p.drawImage((leftMargin - sdOffsets[0]) * scaleFactor, (topMargin - sdOffsets[1]) * scaleFactor, img);

				return tmpImg;
			}
		}
	}
	return QImage{};
}

bool HdExtractor::ModGenerator::filter(QString groupName, ArchiveType type)
{
	// skip mainmenu (RoE)
	if(vstd::contains({"MAINMENU", "GAMSELBK", "GSELPOP1", "SCSELBCK", "LOADGAME", "NEWGAME", "LOADBAR"}, boost::to_upper_copy(groupName.toStdString())))
		return false;

	// skip menu buttons (RoE)
	if(vstd::contains({"MMENUNG", "MMENULG", "MMENUHS", "MMENUCR", "MMENUQT", "GTSINGL", "GTMULTI", "GTCAMPN", "GTTUTOR", "GTBACK", "GTSINGL", "GTMULTI", "GTCAMPN", "GTTUTOR", "GTBACK"}, boost::to_upper_copy(groupName.toStdString())))
		return false;
	
	// skip dialogbox - coloring not supported yet by vcmi
	if(vstd::contains({"DIALGBOX"}, boost::to_upper_copy(groupName.toStdString())))
		return false;

	// skip water + rivers special handling - paletteAnimation - not supported yet by vcmi
	if(vstd::contains({"WATRTL", "LAVATL", "CLRRVR", "MUDRVR", "LAVRVR"}, boost::to_upper_copy(groupName.toStdString())))
		return false;

	return true;
}

QImage HdExtractor::ModGenerator::getBorder(QImage & img, QRgb c, int window)
{
    QImage tmpImg(img.size(), img.format());
	auto getAlpha = [](int x, int y, const QImage & image, int window)
	{
		int max = 0;
		int min = 255;
		for (int i = std::max(x - window, 0), maxI = std::min(x + window, image.width()); i < maxI; i++)
		{
			for (int j = std::max(y - window, 0), maxJ = std::min(y + window, image.height()); j < maxJ; j++)
			{
				max = std::max(max, image.pixelColor(i, j).alpha());
				min = std::min(min, image.pixelColor(i, j).alpha());
			}
		}
		return max - min;
	};

	tbb::parallel_for(tbb::blocked_range<size_t>(0, tmpImg.width()), [&](const tbb::blocked_range<size_t> & r)
		{
			for (int x = r.begin(), width = r.end(); x < width; x++)
			{
				for (int y = 0, height = tmpImg.height(); y < height; y++)
				{
					QColor color(qRed(c), qGreen(c), qBlue(c), getAlpha(x, y, img, window));
					tmpImg.setPixelColor(x, y, color);
				}
			}
		});

	return tmpImg;
}

QImage HdExtractor::ModGenerator::addFlags(QImage & img, QString groupName, QString imageName, int factor)
{
	if(!flagData.count(imageName.toUpper().toStdString()))
		return QImage{};

	auto data = flagData[imageName.toUpper().toStdString()];

	QImage tmpImg(img.size(), img.format());
	tmpImg.fill(qRgba(0, 0, 0, 0));
	QPainter p(&tmpImg);

	for(int i = 0; i < std::stoi(data[0]); i++)
	{
		auto flag = factor == 2 ? flag2x : flag3x;
		if(std::stoi(data[3 + i * 3]) == 1)
			flag = flag.mirrored(true, false);
		p.drawImage(std::stoi(data[1 + i * 3]) * factor, std::stoi(data[2 + i * 3]) * factor, flag);
	}

	return tmpImg;
}

void HdExtractor::ModGenerator::loadFlagData(QDir path)
{
	std::map<std::string, std::vector<std::string>> data;

	QFile file(path.filePath("data/spriteFlagsInfo.txt"));
	file.open(QIODevice::ReadOnly);
	QTextStream in(&file);
	while(!in.atEnd()) {
		QString line = in.readLine();
		QStringList col = line.split(" ");
		col.takeLast(); // last is empty
		auto name = col.takeFirst();
		std::vector<std::string> tmp;
		std::transform(col.begin(), col.end(), std::back_inserter(tmp), [](const QString &v){ return v.toStdString(); });
		data[name.toStdString()] = tmp;
	}
	file.close();

	auto brighten = [](QImage img)
	{
		QImage tmpImg(img.size(), img.format());
		for (int x = 0, width = tmpImg.width(); x < width; x++)
		{
			for (int y = 0, height = tmpImg.height(); y < height; y++)
			{
				auto c = img.pixelColor(x, y);
				QColor colorNew(std::min(static_cast<int>(c.red() * 2.45), 255), std::min(static_cast<int>(c.green() * 2.45), 255), std::min(static_cast<int>(c.blue() * 2.45), 255), c.alpha());
				tmpImg.setPixelColor(x, y, colorNew);
			}
		}
		return tmpImg;
	};

	flag2x = QImage(path.filePath("data/flags/flag_grey_x2.png"));
	flag3x = QImage(path.filePath("data/flags/flag_grey.png"));

	flag2x = brighten(flag2x);
	flag3x = brighten(flag3x);
	flagData = data;
}