/*
 * importsource.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QString>
#include <memory>

/// Copies share file access and, for downloaded archives, cleanup ownership.
/// The cleanup path is fixed even when an importer materializes another copy.
struct ImportSource
{
	QString path;

	static ImportSource userFile(QString path);
	static ImportSource downloadedArchive(QString path);

private:
	struct Lifetime;
	std::shared_ptr<Lifetime> lifetime;
};
