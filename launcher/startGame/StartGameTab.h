#ifndef STARTGAMETAB_H
#define STARTGAMETAB_H

#include <QWidget>

namespace Ui
{
class StartGameTab;
}

enum class EGameUpdateStatus : int8_t
{
	NOT_CHECKED,
	NO_UPDATE,
	UPDATE_AVAILABLE
};

enum ETranslationStatus
{
	NOT_AVAILABLE, // translation for this language was not found in mod list. Could also happen if player is offline or disabled repository checkout
	NOT_INSTALLLED, // translation mod found, but it is not installed
	DISABLED, // translation mod found, and installed, but toggled off
	ACTIVE // translation mod active OR game is already in specified language (e.g. English H3 for players with English language)
};

class MainWindow;

class StartGameTab : public QWidget
{
	Q_OBJECT

	MainWindow * getMainWindow();

	void refreshState();

	void refreshUpdateStatus(EGameUpdateStatus status);
	void refreshTranslation(ETranslationStatus status);
	void refreshMods();
	void refreshGameData();

public:
	explicit StartGameTab(QWidget * parent = nullptr);
	~StartGameTab();

private slots:
	void on_buttonGameStart_clicked();

	void on_buttonOpenChangelog_clicked();

	void on_buttonOpenDownloads_clicked();

	void on_buttonUpdateCheck_clicked();

	void on_buttonGameEditor_clicked();

	void on_buttonImportFiles_clicked();

private:
	Ui::StartGameTab * ui;
};

#endif // STARTGAMETAB_H
