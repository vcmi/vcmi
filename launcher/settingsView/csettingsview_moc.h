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

namespace Ui
{
class CSettingsView;
}

class MainWindow;

class CSettingsView : public QWidget
{
	Q_OBJECT

	MainWindow * getMainWindow();

public:
	explicit CSettingsView(QWidget * parent = nullptr);
	~CSettingsView();

	void loadSettings();
	void loadToggleButtonSettings();
	void loadTranslation();
	void setDisplayList();
	void changeEvent(QEvent *event) override;
	void showEvent(QShowEvent * event) override;

	void setCheckbuttonState(QToolButton * button, bool checked);
	void updateCheckbuttonText(QToolButton * button);

public slots:
	void fillValidResolutions();

private slots:
	void on_comboBoxResolution_currentTextChanged(const QString & arg1);
	void on_comboBoxFullScreen_currentIndexChanged(int index);
	void on_comboBoxFriendlyAI_currentTextChanged(const QString & arg1);
	void on_comboBoxNeutralAI_currentTextChanged(const QString & arg1);
	void on_comboBoxEnemyAI_currentTextChanged(const QString & arg1);
	void on_spinBoxNetworkPort_valueChanged(int arg1);
	void on_buttonShowIntro_toggled(bool value);
	void on_buttonAllowPortrait_toggled(bool value);
	void on_buttonAutoCheck_toggled(bool value);
	void on_comboBoxDisplayIndex_currentIndexChanged(int index);
	void on_buttonAutoSave_toggled(bool value);
	void on_comboBoxLanguage_currentIndexChanged(int index);
	void on_buttonCursorType_toggled(bool value);
	void on_pushButtonTranslation_clicked();
	void on_pushButtonResetTutorialTouchscreen_clicked();
	void on_buttonRepositoryDefault_toggled(bool value);
	void on_buttonRepositoryExtra_toggled(bool value);
	void on_lineEditRepositoryExtra_textEdited(const QString &arg1);
	void on_spinBoxInterfaceScaling_valueChanged(int arg1);
	void on_refreshRepositoriesButton_clicked();
	void on_buttonConfigEditor_clicked();
	void on_spinBoxFramerateLimit_valueChanged(int arg1);
	void on_buttonVSync_toggled(bool value);
	void on_comboBoxEnemyPlayerAI_currentTextChanged(const QString &arg1);
	void on_comboBoxAlliedPlayerAI_currentTextChanged(const QString &arg1);
	void on_buttonAutoSavePrefix_toggled(bool value);
	void on_spinBoxAutoSaveLimit_valueChanged(int arg1);
	void on_lineEditAutoSavePrefix_textEdited(const QString &arg1);
	void on_sliderReservedArea_valueChanged(int arg1);
	void on_comboBoxRendererType_currentTextChanged(const QString &arg1);

	void on_buttonIgnoreSslErrors_clicked(bool checked);
	void on_comboBoxUpscalingFilter_currentIndexChanged(int index);
	void on_comboBoxDownscalingFilter_currentIndexChanged(int index);
	void on_sliderMusicVolume_valueChanged(int value);
	void on_sliderSoundVolume_valueChanged(int value);
	void on_buttonRelativeCursorMode_toggled(bool value);
	void on_sliderRelativeCursorSpeed_valueChanged(int value);
	void on_buttonHapticFeedback_toggled(bool value);
	void on_sliderLongTouchDuration_valueChanged(int value);
	void on_slideToleranceDistanceMouse_valueChanged(int value);
	void on_sliderToleranceDistanceTouch_valueChanged(int value);
	void on_sliderToleranceDistanceController_valueChanged(int value);
	void on_lineEditGameLobbyHost_textChanged(const QString &arg1);
	void on_spinBoxNetworkPortLobby_valueChanged(int arg1);
	void on_sliderControllerSticksAcceleration_valueChanged(int value);
	void on_sliderControllerSticksSensitivity_valueChanged(int value);
	void on_sliderScalingFont_valueChanged(int value);
	void on_buttonFontAuto_clicked(bool checked);
	void on_buttonFontScalable_clicked(bool checked);
	void on_buttonFontOriginal_clicked(bool checked);

	void on_buttonValidationOff_clicked(bool checked);
	void on_buttonValidationBasic_clicked(bool checked);
	void on_buttonValidationFull_clicked(bool checked);
	void on_sliderScalingCursor_valueChanged(int value);
	void on_buttonScalingAuto_toggled(bool checked);
	void on_buttonHandleBackRightMouseButton_toggled(bool checked);
	void on_buttonIgnoreMuteSwitch_toggled(bool checked);

private:
	Ui::CSettingsView * ui;

	void fillValidRenderers();
	void fillValidResolutionsForScreen(int screenIndex);
	void fillValidScalingRange();
	QSize getPreferredRenderingResolution();
};
