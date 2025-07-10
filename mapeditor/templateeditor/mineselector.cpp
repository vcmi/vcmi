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

auto resources = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};

MineSelector::MineSelector(std::map<TResource, ui16> & mines) :
	ui(new Ui::MineSelector),
	minesSelected(mines)
{
	ui->setupUi(this);

	setWindowTitle(tr("Mine Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->tableWidgetMines->setColumnCount(2);
	ui->tableWidgetMines->setRowCount(resources.size());
	ui->tableWidgetMines->setHorizontalHeaderLabels({tr("Resource"), tr("Mines")});
	for (int row = 0; row < resources.size(); ++row)
	{
		auto name = LIBRARY->generaltexth->translate(TextIdentifier("core.restypes", resources[row].getNum()).get());
		auto label = new QLabel(QString::fromStdString(name));
		label->setAlignment(Qt::AlignCenter);
		ui->tableWidgetMines->setCellWidget(row, 0, label);

		auto spinBox = new QSpinBox();
		spinBox->setRange(0, 100);
		spinBox->setValue(mines[resources[row]]);
		ui->tableWidgetMines->setCellWidget(row, 1, spinBox);
	}
	ui->tableWidgetMines->resizeColumnsToContents();

	show();
}

void MineSelector::showMineSelector(std::map<TResource, ui16> & mines)
{
	auto * dialog = new MineSelector(mines);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void MineSelector::on_buttonBoxResult_accepted()
{
	for (int row = 0; row < resources.size(); ++row)
		minesSelected[resources[row]] = static_cast<QSpinBox *>(ui->tableWidgetMines->cellWidget(row, 1))->value();

	close();
}

void MineSelector::on_buttonBoxResult_rejected()
{
	close();
}
