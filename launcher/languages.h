/*
 * languages.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class QString;
class QComboBox;
class QListWidget;

namespace Languages
{
	struct Options;

	QString generateLanguageName(const Languages::Options & identifier);
	QString getHeroesDataLanguage();

	/// if includeAll is set, all languages including encoding placeholders will be included into widget
	void fillLanguages(QComboBox * widget, bool includeAll);
	void fillLanguages(QListWidget * widget, bool includeAll);

}
