/*
 * herosspellwidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "artifactwidget.h"
#include "ui_artifactwidget.h"
#include "inspector.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/artifact/CArtifactFittingSet.h"

ArtifactWidget::ArtifactWidget(CArtifactFittingSet & fittingSet, QWidget * parent) :
	QDialog(parent),
	ui(new Ui::ArtifactWidget),
	fittingSet(fittingSet)
{
	ui->setupUi(this);

	connect(ui->saveButton, &QPushButton::clicked, this, [this]() 
	{
		saveArtifact(ui->artifact->currentData().toInt(), ArtifactPosition(ui->possiblePositions->currentData().toInt()));
		close();
	});
	connect(ui->cancelButton, &QPushButton::clicked, this, &ArtifactWidget::close);
	connect(ui->possiblePositions, static_cast<void(QComboBox::*) (int)> (&QComboBox::currentIndexChanged), this, &ArtifactWidget::fillArtifacts);
	
	std::vector<ArtifactPosition> possiblePositions;
	for(const auto & slot : ArtifactUtils::allWornSlots())
	{
		if(fittingSet.isPositionFree(slot))
		{
			ui->possiblePositions->addItem(QString::fromStdString(NArtifactPosition::namesHero[slot.num]), slot.num);
		}
	}
	ui->possiblePositions->addItem(QString::fromStdString(NArtifactPosition::backpack), ArtifactPosition::BACKPACK_START);
	fillArtifacts();


}

void ArtifactWidget::fillArtifacts()
{
	ui->artifact->clear();
	auto currentSlot = ui->possiblePositions->currentData().toInt();
	for (const auto& art : LIBRARY->arth->getDefaultAllowed())
	{
		auto artifact = art.toArtifact();
		// forbid spell scroll for now as require special handling
		if (artifact->canBePutAt(&fittingSet, currentSlot, true) && artifact->getId() != ArtifactID::SPELL_SCROLL) {
			ui->artifact->addItem(QString::fromStdString(artifact->getNameTranslated()), QVariant::fromValue(artifact->getIndex()));
		}
	}
}

ArtifactWidget::~ArtifactWidget()
{
	delete ui;
}
