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
#include "../lib/CGeneralTextHandler.h"

#include <QComboBox>
#include <QListWidget>

// list of language names, for generation of translations. Do not use directly, use Languages namespace instead
static const std::array<std::string, 19> languageTranslatedNamesGenerator = {
	{
		QT_TRANSLATE_NOOP("Language", "Czech"),
		QT_TRANSLATE_NOOP("Language", "Chinese"),
		QT_TRANSLATE_NOOP("Language", "English"),
		QT_TRANSLATE_NOOP("Language", "Finnish"),
		QT_TRANSLATE_NOOP("Language", "French"),
		QT_TRANSLATE_NOOP("Language", "German"),
		QT_TRANSLATE_NOOP("Language", "Hungarian"),
		QT_TRANSLATE_NOOP("Language", "Italian"),
		QT_TRANSLATE_NOOP("Language", "Korean"),
		QT_TRANSLATE_NOOP("Language", "Polish"),
		QT_TRANSLATE_NOOP("Language", "Portuguese"),
		QT_TRANSLATE_NOOP("Language", "Russian"),
		QT_TRANSLATE_NOOP("Language", "Spanish"),
		QT_TRANSLATE_NOOP("Language", "Swedish"),
		QT_TRANSLATE_NOOP("Language", "Turkish"),
		QT_TRANSLATE_NOOP("Language", "Ukrainian"),
		QT_TRANSLATE_NOOP("Language", "Other (East European)"),
		QT_TRANSLATE_NOOP("Language", "Other (Cyrillic Script)"),
		QT_TRANSLATE_NOOP("Language", "Other (West European)"),
	 }
};

static_assert(languageTranslatedNamesGenerator.size() == static_cast<size_t>(Languages::ELanguages::COUNT), "Languages array is missing a value!");

QString Languages::getHeroesDataLanguage()
{
	CGeneralTextHandler::detectInstallParameters();

	QString language = QString::fromStdString(settings["session"]["language"].String());
	double deviation = settings["session"]["languageDeviation"].Float();

	if(deviation > 0.1)
		return QString();
	return language;
}

QString generateAutodetectedLanguageName(QString const & language)
{
	std::string languageNameEnglish = Languages::getLanguageOptions(language.toStdString()).nameEnglish;
	QString languageName = QApplication::translate( "Language", languageNameEnglish.c_str());
	QString itemName = QApplication::translate("Language", "Auto (%1)").arg(languageName);

	return itemName;
}

QString Languages::generateLanguageName(const Languages::Options & language)
{
	std::string activeLanguage = settings["general"]["language"].String();

	QString localizedName = QApplication::translate("Language", language.nameEnglish.c_str());
	QString nativeName = language.nameNative.c_str();

	if(!language.hasTranslation)
		return localizedName;

	if(activeLanguage == language.identifier)
		return nativeName;

	QString displayName = QString("%1 (%2)").arg(localizedName, nativeName);

	return displayName;
}

void Languages::fillLanguages(QComboBox * widget, bool includeAll)
{
	widget->blockSignals(true); // we do not want calls caused by initialization
	widget->clear();

	std::string activeLanguage = includeAll ?
		settings["general"]["gameDataLanguage"].String():
		settings["general"]["language"].String();

	if (includeAll)
	{
		QString language = getHeroesDataLanguage();
		if (!language.isEmpty())
			widget->addItem(generateAutodetectedLanguageName(language), QString("auto"));

		if (activeLanguage == "auto")
			widget->setCurrentIndex(0);
	}

	for(const auto & language : Languages::getLanguageList())
	{
		if (!language.hasTranslation && !includeAll)
			continue;

		QString displayName = generateLanguageName(language);
		QVariant userData = QString::fromStdString(language.identifier);

		widget->addItem(displayName, userData);
		if(activeLanguage == language.identifier)
			widget->setCurrentIndex(widget->count() - 1);
	}

	widget->blockSignals(false);
}

void Languages::fillLanguages(QListWidget * widget, bool includeAll)
{
	widget->blockSignals(true); // we do not want calls caused by initialization
	widget->clear();

	std::string activeLanguage = includeAll ?
		settings["general"]["gameDataLanguage"].String():
		settings["general"]["language"].String();

	if (includeAll)
	{
		QString language = getHeroesDataLanguage();
		if (!language.isEmpty())
		{
			widget->addItem(generateAutodetectedLanguageName(language));
			widget->item(widget->count() - 1)->setData(Qt::UserRole, QString("auto"));

			if (activeLanguage == "auto")
				widget->setCurrentRow(0);
		}
	}

	for(const auto & language : Languages::getLanguageList())
	{
		if (!language.hasTranslation && !includeAll)
			continue;

		QString displayName = generateLanguageName(language);
		QVariant userData = QString::fromStdString(language.identifier);

		widget->addItem(displayName);
		widget->item(widget->count() - 1)->setData(Qt::UserRole, userData);
		if(activeLanguage == language.identifier)
			widget->setCurrentRow(widget->count() - 1);
	}
	widget->blockSignals(false);
}
