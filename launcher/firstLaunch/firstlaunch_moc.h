/*
 * firstlaunch_moc.h, part of VCMI engine
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
class FirstLaunchView;
}

class FirstLaunchView : public QWidget
{
	Q_OBJECT

	void changeEvent(QEvent *event);
public:
	explicit FirstLaunchView(QWidget * parent = 0);
	~FirstLaunchView();

public slots:

private slots:

	void on_buttonTabLanguage_clicked();

	void on_buttonTabHeroesData_clicked();

	void on_buttonTabModPreset_clicked();

	void on_buttonTabFinish_clicked();

	void on_listWidgetLanguage_currentRowChanged(int currentRow);

private:
	Ui::FirstLaunchView * ui;

};
