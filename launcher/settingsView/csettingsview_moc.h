#pragma once

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

	void on_comboBoxEnableMods_currentIndexChanged(int index);

	void on_spinBoxNetworkPort_valueChanged(int arg1);

	void on_plainTextEditRepos_textChanged();

private:
	Ui::CSettingsView *ui;
};
