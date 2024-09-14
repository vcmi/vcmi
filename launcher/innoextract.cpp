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
