/*
 * configeditordialog_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;

VCMI_LIB_NAMESPACE_END

namespace Ui {
class ConfigEditorDialog;
}

class ConfigEditorDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConfigEditorDialog(QWidget *parent = nullptr);
	~ConfigEditorDialog();
	
	static void showConfigEditorDialog();

private slots:
	void onComboBoxTextChanged(QString filename);
	void onCloseButtonClicked();
	void onSaveButtonClicked();

private:
	Ui::ConfigEditorDialog *ui;

	QString loadedText;
	QString loadedFile;

	bool askUnsavedChanges(QWidget *parent);
	QString loadFile(QString filename);
	void saveFile(QString filename, QString content);
};
