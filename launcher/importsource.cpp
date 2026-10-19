/*
 * importsource.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "importsource.h"
#include "helper.h"

struct ImportSource::Lifetime
{
	QString cleanupPath;
	std::shared_ptr<void> access;

	~Lifetime()
	{
		if(cleanupPath.isEmpty())
			return;
		QFile file(cleanupPath);
		if(file.exists() && !file.remove())
			logGlobal->warn("Failed to remove downloaded import archive '%s': %s", cleanupPath.toStdString(), file.errorString().toStdString());
	}
};

ImportSource ImportSource::userFile(QString path)
{
	ImportSource source;
	source.path = std::move(path);
	source.lifetime = std::make_shared<Lifetime>();
	source.lifetime->access = Helper::acquireImportAccess(source.path);
	return source;
}

ImportSource ImportSource::downloadedArchive(QString path)
{
	ImportSource source;
	source.path = std::move(path);
	source.lifetime = std::make_shared<Lifetime>();
	source.lifetime->cleanupPath = source.path;
	return source;
}
