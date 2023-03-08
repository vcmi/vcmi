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

#include "mainwindow_moc.h"

#include "../languages.h"
#include "../../lib/CConfigHandler.h"

FirstLaunchView::FirstLaunchView(QWidget * parent)
	: QWidget(parent), ui(new Ui::FirstLaunchView)
{
	ui->setupUi(this);

	Languages::fillLanguages(ui->listWidgetLanguage);
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

void FirstLaunchView::on_listWidgetLanguage_currentRowChanged(int currentRow)
{
	Settings node = settings.write["general"]["language"];
	QString selectedLanguage = ui->listWidgetLanguage->item(currentRow)->data(Qt::UserRole).toString();
	node->String() = selectedLanguage.toStdString();

	if(auto * mainWindow = dynamic_cast<MainWindow *>(qApp->activeWindow()))
		mainWindow->updateTranslation();
}

void FirstLaunchView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange)
	{
		ui->retranslateUi(this);
		Languages::fillLanguages(ui->listWidgetLanguage);
	}
	QWidget::changeEvent(event);
}
