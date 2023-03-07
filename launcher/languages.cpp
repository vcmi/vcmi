/*
 * languages.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "languages.h"

#include "../lib/CConfigHandler.h"
#include "../lib/Languages.h"

#include <QComboBox>
#include <QListWidget>

// list of language names, for generation of translations. Do not use directly, use Languages namespace instead
static const std::array<std::string, 11> languageTranslatedNamesGenerator = {
	{
		QT_TRANSLATE_NOOP("Language", "Chinese"),
		QT_TRANSLATE_NOOP("Language", "English"),
		QT_TRANSLATE_NOOP("Language", "French"),
		QT_TRANSLATE_NOOP("Language", "German"),
		QT_TRANSLATE_NOOP("Language", "Korean"),
		QT_TRANSLATE_NOOP("Language", "Polish"),
		QT_TRANSLATE_NOOP("Language", "Russian"),
		QT_TRANSLATE_NOOP("Language", "Ukrainian"),
		QT_TRANSLATE_NOOP("Language", "Other (East European)"),
		QT_TRANSLATE_NOOP("Language", "Other (Cyrillic Script)"),
		QT_TRANSLATE_NOOP("Language", "Other (West European)"),
	 }
};

static_assert(languageTranslatedNamesGenerator.size() == static_cast<size_t>(Languages::ELanguages::COUNT), "Languages array is missing a value!");

QString Languages::generateLanguageName(const Languages::Options & language)
{
	std::string activeLanguage = settings["general"]["language"].String();

	QString localizedName = QApplication::translate("Language", language.nameEnglish.c_str());
	QString nativeName = language.nameNative.c_str();

	if(activeLanguage == language.identifier)
		return nativeName;

	if(!language.hasTranslation)
		return localizedName;

	QString displayName = QString("%1 (%2)").arg(localizedName, nativeName);

	return displayName;
}

void Languages::fillLanguages(QComboBox * widget)
{
	widget->blockSignals(true); // we do not want calls caused by initialization
	widget->clear();

	std::string activeLanguage = settings["general"]["language"].String();

	for(const auto & language : Languages::getLanguageList())
	{
		QString displayName = generateLanguageName(language);
		QVariant userData = QString::fromStdString(language.identifier);

		widget->addItem(displayName, userData);
		if(activeLanguage == language.identifier)
			widget->setCurrentIndex(widget->count() - 1);
	}

	widget->blockSignals(false);
}

void Languages::fillLanguages(QListWidget * widget)
{
	widget->blockSignals(true); // we do not want calls caused by initialization
	widget->clear();

	std::string activeLanguage = settings["general"]["language"].String();

	for(const auto & language : Languages::getLanguageList())
	{
		QString displayName = generateLanguageName(language);
		QVariant userData = QString::fromStdString(language.identifier);

		widget->addItem(displayName);
		widget->item(widget->count() - 1)->setData(Qt::UserRole, userData);
		if(activeLanguage == language.identifier)
			widget->setCurrentRow(widget->count() - 1);
	}
	widget->blockSignals(false);
}
