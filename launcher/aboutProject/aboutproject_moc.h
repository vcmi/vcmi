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
public:
	explicit AboutProjectView(QWidget * parent = nullptr);

public slots:

private slots:


	void on_updatesButton_clicked();

	void on_openGameDataDir_clicked();

	void on_openUserDataDir_clicked();

	void on_openTempDir_clicked();

	void on_pushButtonDiscord_clicked();

	void on_pushButtonSlack_clicked();

	void on_pushButtonGithub_clicked();

	void on_pushButtonHomepage_clicked();

	void on_pushButtonBugreport_clicked();

private:
	Ui::AboutProjectView * ui;

};
