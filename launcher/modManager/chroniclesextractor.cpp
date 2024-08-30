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

#ifdef ENABLE_INNOEXTRACT
#include "cli/extract.hpp"
#include "setup/version.hpp"
#endif

ChroniclesExtractor::ChroniclesExtractor(QWidget *p, std::function<void(float percent)> cb) :
	parent(p), cb(cb)
{
}

bool ChroniclesExtractor::handleTempDir(bool create)
{
	if(create)
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
	}
	else
		tempDir.removeRecursively();

	return true;
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
	const std::map<int, QByteArray> chronicles = {
		{1, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Warlords of the Wasteland"), 90}},
		{2, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Conquest of the Underworld"), 92}},
		{3, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Masters of the Elements"), 86}},
		{4, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Clash of the Dragons"), 80}},
		{5, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The World Tree"), 68}},
		{6, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The Fiery Moon"), 68}},
		{7, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - Revolt of the Beastmasters"), 92}},
		{8, QByteArray{reinterpret_cast<const char*>(u"Heroes Chronicles - The Sword of Frost"), 76}}
	};
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
#ifndef ENABLE_INNOEXTRACT
		QMessageBox::critical(parent, tr("Innoextract functionality missing"), "VCMI was compiled without innoextract support, which is needed to extract chroncles!");
		return false;
#else
		::extract_options o;
		o.extract = true;

		// standard settings
		o.gog_galaxy = true;
		o.codepage = 0U;
		o.output_dir = tempDir.path().toStdString();
		o.extract_temp = true;
		o.extract_unknown = true;
		o.filenames.set_expand(true);

		o.preserve_file_times = true; // also correctly closes file -> without it: on Windows the files are not written completely

		QString errorText = "";
		try
		{
			process_file(file.toStdString(), o, [this](float progress) {
				float overallProgress = ((1.0 / float(fileCount)) * float(extractionFile)) + (progress / float(fileCount));
				if(cb)
					cb(overallProgress);
			});
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

		if(!errorText.isEmpty())
		{
			QMessageBox::critical(parent, tr("Extracting error!"), errorText);
			return false;
		}

		return true;
#endif
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

		if(!handleTempDir(true))
			continue;

		if(!extractGogInstaller(f))
			continue;

		if(!handleTempDir(false))
			continue;
	}
}