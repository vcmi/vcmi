/*
 * csettingsview_moc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../StdInc.h"

namespace Ui
{
class CSettingsView;
}

class CSettingsView : public QWidget
{
	Q_OBJECT

public:
	explicit CSettingsView(QWidget * parent = 0);
	~CSettingsView();

	void loadSettings();
	void loadTranslation();
	void setDisplayList();
	void changeEvent(QEvent *event) override;
	void showEvent(QShowEvent * event) override;

public slots:
	void fillValidResolutions();

private slots:
	void on_comboBoxResolution_currentTextChanged(const QString & arg1);
	void on_comboBoxFullScreen_currentIndexChanged(int index);
	void on_comboBoxPlayerAI_currentTextChanged(const QString & arg1);
	void on_comboBoxFriendlyAI_currentTextChanged(const QString & arg1);
	void on_comboBoxNeutralAI_currentTextChanged(const QString & arg1);
	void on_comboBoxEnemyAI_currentTextChanged(const QString & arg1);
	void on_spinBoxNetworkPort_valueChanged(int arg1);
	void on_openTempDir_clicked();
	void on_openUserDataDir_clicked();
	void on_openGameDataDir_clicked();
	void on_comboBoxShowIntro_currentIndexChanged(int index);
	void on_comboBoxAutoCheck_currentIndexChanged(int index);
	void on_comboBoxDisplayIndex_currentIndexChanged(int index);
	void on_comboBoxAutoSave_currentIndexChanged(int index);
	void on_updatesButton_clicked();
	void on_comboBoxLanguage_currentIndexChanged(int index);
	void on_comboBoxCursorType_currentIndexChanged(int index);
	void on_listWidgetSettings_currentRowChanged(int currentRow);
	void on_pushButtonTranslation_clicked();

	void on_comboBoxLanguageBase_currentIndexChanged(int index);

	void on_checkBoxRepositoryDefault_stateChanged(int arg1);

	void on_checkBoxRepositoryExtra_stateChanged(int arg1);

	void on_lineEditRepositoryExtra_textEdited(const QString &arg1);

	void on_spinBoxInterfaceScaling_valueChanged(int arg1);

private:
	Ui::CSettingsView * ui;

	void fillValidResolutionsForScreen(int screenIndex);
	void fillValidScalingRange();
	QSize getPreferredRenderingResolution();
};
