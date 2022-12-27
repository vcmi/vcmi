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
	void setDisplayList();
	void changeEvent(QEvent *event) override;

	bool isExtraResolutionsModEnabled{};

public slots:
	void fillValidResolutions(bool isExtraResolutionsModEnabled);

private slots:
	void on_comboBoxResolution_currentTextChanged(const QString & arg1);

	void on_comboBoxFullScreen_currentIndexChanged(int index);

	void on_comboBoxPlayerAI_currentIndexChanged(const QString & arg1);

	void on_comboBoxFriendlyAI_currentIndexChanged(const QString & arg1);

	void on_comboBoxNeutralAI_currentIndexChanged(const QString & arg1);

	void on_comboBoxEnemyAI_currentIndexChanged(const QString & arg1);

	void on_spinBoxNetworkPort_valueChanged(int arg1);

	void on_plainTextEditRepos_textChanged();

	void on_comboBoxEncoding_currentIndexChanged(int index);

	void on_openTempDir_clicked();

	void on_openUserDataDir_clicked();

	void on_openGameDataDir_clicked();

	void on_comboBoxShowIntro_currentIndexChanged(int index);

	void on_changeGameDataDir_clicked();

	void on_comboBoxAutoCheck_currentIndexChanged(int index);

	void on_comboBoxDisplayIndex_currentIndexChanged(int index);

	void on_comboBoxAutoSave_currentIndexChanged(int index);

	void on_updatesButton_clicked();

	void on_comboBoxLanguage_currentIndexChanged(int index);

	void on_comboBoxCursorType_currentIndexChanged(int index);

private:
	Ui::CSettingsView * ui;

	void fillValidResolutionsForScreen(int screenIndex);
};
