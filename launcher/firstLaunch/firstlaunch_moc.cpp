/*
 * firstlaunch_moc.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "firstlaunch_moc.h"
#include "ui_firstlaunch_moc.h"

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent), ui(new Ui::FirstLaunchView)
{
	ui->setupUi(this);
}

FirstLaunchView::~FirstLaunchView()
{

}

void FirstLaunchView::on_buttonTabLanguage_clicked()
{
	ui->installerTabs->setCurrentIndex(0);
}


void FirstLaunchView::on_buttonTabHeroesData_clicked()
{
	ui->installerTabs->setCurrentIndex(1);
}


void FirstLaunchView::on_buttonTabModPreset_clicked()
{
	ui->installerTabs->setCurrentIndex(2);
}


void FirstLaunchView::on_buttonTabFinish_clicked()
{
	ui->installerTabs->setCurrentIndex(3);
}

