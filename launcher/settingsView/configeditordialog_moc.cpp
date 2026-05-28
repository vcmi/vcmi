/*
 * configeditordialog_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "configeditordialog_moc.h"
#include "ui_configeditordialog_moc.h"

#include "../helper.h"
#include "../mainwindow_moc.h"

#include "../../lib/json/JsonNode.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/json/JsonFormatException.h"

ConfigEditorDialog::ConfigEditorDialog(QWidget *parent):
	QDialog(parent),
	ui(new Ui::ConfigEditorDialog)
{
	ui->setupUi(this);

	setWindowTitle(tr("Config editor"));
	
	setWindowModality(Qt::ApplicationModal);
#ifdef VCMI_MOBILE
	showMaximized();
#else
	show();
#endif

	QStringList files = {
		"settings.json",
		"persistentStorage.json",
		"modSettings.json",
		"keyBindingsConfig.json",
	};
	ui->comboBox->addItems(files);

	QFont font("Monospace");
	font.setStyleHint(QFont::TypeWriter);
	ui->plainTextEdit->setFont(font);
	ui->plainTextEdit->setLineWrapMode(QPlainTextEdit::LineWrapMode::NoWrap);
	ui->plainTextEdit->setPlainText(loadFile(files.first()));
	
	connect(ui->comboBox, &QComboBox::currentTextChanged, this, &ConfigEditorDialog::onComboBoxTextChanged);
	connect(ui->closeButton, &QPushButton::clicked, this, &ConfigEditorDialog::onCloseButtonClicked);
	connect(ui->saveButton, &QPushButton::clicked, this, &ConfigEditorDialog::onSaveButtonClicked);
}

ConfigEditorDialog::~ConfigEditorDialog()
{
	delete ui;
}

void ConfigEditorDialog::showConfigEditorDialog()
{
	auto * dialog = new ConfigEditorDialog();
	
	dialog->setAttribute(Qt::WA_DeleteOnClose);
}

bool ConfigEditorDialog::askUnsavedChanges(QWidget *parent)
{
	auto reply = QMessageBox::question(parent, tr("Unsaved changes"), tr("Do you want to discard changes?"), QMessageBox::Yes | QMessageBox::No);
	return reply != QMessageBox::Yes;
}

void ConfigEditorDialog::onComboBoxTextChanged(QString filename)
{
	auto inputText = ui->plainTextEdit->toPlainText();

	if(filename == loadedFile)
		return;

	if(inputText != loadedText)
	{
		if(askUnsavedChanges(this))
		{
			ui->comboBox->setCurrentText(loadedFile);
			return;
		}
	}

	ui->plainTextEdit->setPlainText(loadFile(filename));
}

void ConfigEditorDialog::onCloseButtonClicked()
{
	auto text = ui->plainTextEdit->toPlainText();
	if(text != loadedText)
	{
		if(askUnsavedChanges(this))
			return;
	}
	close();
}

void ConfigEditorDialog::onSaveButtonClicked()
{
	auto text = ui->plainTextEdit->toPlainText();
	auto filename = ui->comboBox->currentText();

	if(text == loadedText)
	{
		close();
		return;
	}

	auto stdText = text.toStdString();
	try
	{
		JsonParsingSettings jsonSettings;
		jsonSettings.strict = true;
		JsonNode node(reinterpret_cast<const std::byte*>(stdText.data()), stdText.size(), jsonSettings, filename.toStdString());
	}
	catch(const JsonFormatException& e)
	{
		QMessageBox::critical(this, tr("JSON file is not valid!"), e.what());
		return;
	}

	saveFile(filename, text);

	Helper::reLoadSettings();

	close();
}

QString ConfigEditorDialog::loadFile(QString filename)
{
	auto file = pathToQString(VCMIDirs::get().userConfigPath() / filename.toStdString());
	QFile f(file);
	if(!f.open(QFile::ReadOnly | QFile::Text))
	{
		logGlobal->error("ConfigEditor can not open config for reading!");
		return {};
	}
	loadedText = QString::fromUtf8(f.readAll());
	loadedFile = filename;

	return loadedText;
}

void ConfigEditorDialog::saveFile(QString filename, QString content)
{
	auto file = pathToQString(VCMIDirs::get().userConfigPath() / filename.toStdString());
	QFile f(file);
	if(!f.open(QFile::WriteOnly | QFile::Text))
	{
		logGlobal->error("ConfigEditor can not open config for writing!");
		return;
	}
	f.write(content.toUtf8());
}
