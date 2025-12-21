/*
 * aboutproject_moc.h, part of VCMI engine
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
class AboutProjectView;
}

class CModListView;

class AboutProjectView : public QWidget
{
	Q_OBJECT

	void changeEvent(QEvent *event) override;

	/// Hides a widget and expands second widgets to take place of first widget in layout
	void hideAndStretchWidget(QGridLayout * layout, QWidget * toHide, QWidget * toStretch);

public:
	explicit AboutProjectView(QWidget * parent = nullptr);
	~AboutProjectView() override;

private slots:
	void on_updatesButton_clicked();

	void on_openGameDataDir_clicked();

	void on_openUserDataDir_clicked();

	void on_openTempDir_clicked();

	void on_pushButtonDiscord_clicked();

	void on_pushButtonGithub_clicked();

	void on_pushButtonHomepage_clicked();

	void on_pushButtonBugreport_clicked();

	void on_pushButtonExportLogs_clicked();

	void on_openConfigDir_clicked();

private:
	std::unique_ptr<Ui::AboutProjectView> ui;
};
