#pragma once

#include "StdInc.h"

namespace Ui {
	class CSettingsView;
}

class CSettingsView : public QWidget
{
	Q_OBJECT

public:
	explicit CSettingsView(QWidget *parent = 0);
	~CSettingsView();

	void loadSettings();

private slots:
	void on_comboBoxResolution_currentIndexChanged(const QString &arg1);

	void on_comboBoxFullScreen_currentIndexChanged(int index);

	void on_comboBoxPlayerAI_currentIndexChanged(const QString &arg1);

	void on_comboBoxNeutralAI_currentIndexChanged(const QString &arg1);

	void on_spinBoxNetworkPort_valueChanged(int arg1);

	void on_plainTextEditRepos_textChanged();

	void on_comboBoxEncoding_currentIndexChanged(int index);

	void on_openTempDir_clicked();

	void on_openUserDataDir_clicked();

	void on_openGameDataDir_clicked();

	void on_comboBoxShowIntro_currentIndexChanged(int index);

	void on_changeGameDataDir_clicked();

	void on_comboBoxAutoCheck_currentIndexChanged(int index);

private:
	Ui::CSettingsView *ui;
};
