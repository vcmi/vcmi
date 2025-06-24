/*
 * treasureselector.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "treasureselector.h"
#include "ui_treasureselector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/rmg/CRmgTemplate.h"

TreasureSelector::TreasureSelector(std::vector<CTreasureInfo> & treasures) :
	ui(new Ui::TreasureSelector),
	treasures(treasures)
{
	ui->setupUi(this);

	setWindowTitle(tr("Treasure Selector"));
	
	setWindowModality(Qt::ApplicationModal);

	ui->tableWidgetTreasures->setColumnCount(4);
	ui->tableWidgetTreasures->setRowCount(treasures.size() + 1);
	ui->tableWidgetTreasures->setHorizontalHeaderLabels({tr("Min"), tr("Max"), tr("Density"), tr("Action")});

	auto addRow = [this](int min, int max, int density, int row){
		auto spinBoxMin = new QSpinBox();
        spinBoxMin->setRange(0, 1000000);
        spinBoxMin->setValue(min);
		ui->tableWidgetTreasures->setCellWidget(row, 0, spinBoxMin);

		auto spinBoxMax = new QSpinBox();
        spinBoxMax->setRange(0, 1000000);
        spinBoxMax->setValue(max);
		ui->tableWidgetTreasures->setCellWidget(row, 1, spinBoxMax);

		auto spinBoxDensity = new QSpinBox();
        spinBoxDensity->setRange(0, 1000);
        spinBoxDensity->setValue(density);
		ui->tableWidgetTreasures->setCellWidget(row, 2, spinBoxDensity);

		auto deleteButton = new QPushButton("Delete");
		ui->tableWidgetTreasures->setCellWidget(row, 3, deleteButton);
		connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
			for (int r = 0; r < ui->tableWidgetTreasures->rowCount(); ++r) {
				if (ui->tableWidgetTreasures->cellWidget(r, 3) == deleteButton) {
					ui->tableWidgetTreasures->removeRow(r);
					break;
				}
			}
		});
	};

	for (int row = 0; row < treasures.size(); ++row)
		addRow(treasures[row].min, treasures[row].max, treasures[row].density, row);

	auto addButton = new QPushButton("Add");
	ui->tableWidgetTreasures->setCellWidget(ui->tableWidgetTreasures->rowCount() - 1, 3, addButton);
	connect(addButton, &QPushButton::clicked, this, [this, addRow, treasures]() {
		ui->tableWidgetTreasures->insertRow(ui->tableWidgetTreasures->rowCount() - 1);
		addRow(0, 0, 0, ui->tableWidgetTreasures->rowCount() - 2);
	});

	ui->tableWidgetTreasures->resizeColumnsToContents();

	show();
}

void TreasureSelector::showTreasureSelector(std::vector<CTreasureInfo> & treasures)
{
	auto * dialog = new TreasureSelector(treasures);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->exec();
}

void TreasureSelector::on_buttonBoxResult_accepted()
{
	treasures.clear();
	for (int row = 0; row < ui->tableWidgetTreasures->rowCount() - 1; ++row)
	{
		CTreasureInfo info;
		info.min = static_cast<QSpinBox *>(ui->tableWidgetTreasures->cellWidget(row, 0))->value();
		info.max = static_cast<QSpinBox *>(ui->tableWidgetTreasures->cellWidget(row, 1))->value();
		info.density = static_cast<QSpinBox *>(ui->tableWidgetTreasures->cellWidget(row, 2))->value();
		treasures.push_back(info);
	}

    close();
}

void TreasureSelector::on_buttonBoxResult_rejected()
{
    close();
}