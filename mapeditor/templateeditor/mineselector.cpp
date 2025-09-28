/*
 * mineselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "mineselector.h"
#include "ui_mineselector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/entities/ResourceTypeHandler.h"

auto resourcesToShow = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS}; //todo: configurable resource support

MineSelector::MineSelector(std::map<GameResID, ui16> & mines) :
	ui(new Ui::MineSelector),
	minesSelected(mines)
{
	ui->setupUi(this);

	setWindowTitle(tr("Mine Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->tableWidgetMines->setColumnCount(2);
	ui->tableWidgetMines->setRowCount(resourcesToShow.size());
	ui->tableWidgetMines->setHorizontalHeaderLabels({tr("Resource"), tr("Mines")});
	for (int row = 0; row < resourcesToShow.size(); ++row)
	{
		auto name = resourcesToShow[row].toResource()->getNameTranslated();
		auto label = new QLabel(QString::fromStdString(name));
		label->setAlignment(Qt::AlignCenter);
		ui->tableWidgetMines->setCellWidget(row, 0, label);

		auto spinBox = new QSpinBox();
		spinBox->setRange(0, 100);
		spinBox->setValue(mines[resourcesToShow[row]]);
		ui->tableWidgetMines->setCellWidget(row, 1, spinBox);
	}
	ui->tableWidgetMines->resizeColumnsToContents();

	show();
}

void MineSelector::showMineSelector(std::map<GameResID, ui16> & mines)
{
	auto * dialog = new MineSelector(mines);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void MineSelector::on_buttonBoxResult_accepted()
{
	for (int row = 0; row < resourcesToShow.size(); ++row)
		minesSelected[resourcesToShow[row]] = static_cast<QSpinBox *>(ui->tableWidgetMines->cellWidget(row, 1))->value();

	close();
}

void MineSelector::on_buttonBoxResult_rejected()
{
	close();
}
