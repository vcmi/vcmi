/*
 * PlayerSelectionDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "PlayerSelectionDialog.h"
#include "../lib/mapping/CMap.h"
#include "mainwindow.h"

#include <QRadioButton>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QAction>
#include <QLabel>


PlayerSelectionDialog::PlayerSelectionDialog(MainWindow * mainWindow)
	: QDialog(mainWindow), selectedPlayer(PlayerColor::NEUTRAL)
{
	setupDialogComponents();

	int maxPlayers = 0;
	if(mainWindow && mainWindow->controller.map())
		maxPlayers = mainWindow->controller.map()->players.size();

	for(int i = 0; i < maxPlayers; ++i)
	{
		PlayerColor player(i);
		addRadioButton(mainWindow->getActionPlayer(player), player);
	}
}

PlayerColor PlayerSelectionDialog::getSelectedPlayer() const
{
	return selectedPlayer;
}

void PlayerSelectionDialog::setupDialogComponents()
{
	setWindowTitle(tr("Select Player"));
	setFixedWidth(dialogWidth);
	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	font.setPointSize(10);
	setFont(font);

	buttonGroup = new QButtonGroup(this);
	buttonGroup->setExclusive(true);

	QLabel * errorLabel = new QLabel(tr("Hero cannot be created as NEUTRAL"), this);
	font.setBold(true);
	errorLabel->setFont(font);
	errorLabel->setWordWrap(true);
	mainLayout.addWidget(errorLabel);

	QLabel * instructionLabel = new QLabel(tr("Switch to one of the available players:"), this);
	font.setBold(false);
	instructionLabel->setFont(font);
	instructionLabel->setWordWrap(true);
	mainLayout.addWidget(instructionLabel);

	QWidget * radioContainer = new QWidget(this);
	radioContainer->setLayout(& radioButtonsLayout);
	mainLayout.addWidget(radioContainer);

	QDialogButtonBox * box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	mainLayout.addWidget(box);

	setLayout(& mainLayout);
}

void PlayerSelectionDialog::addRadioButton(QAction * action, PlayerColor player)
{
	auto * radioButton = new QRadioButton(action->text(), this);
	radioButton->setEnabled(action->isEnabled());
	// Select first available player by default
	if(buttonGroup->buttons().isEmpty() && radioButton->isEnabled())
	{
		radioButton->setChecked(true);
		selectedPlayer = player;
	}

	radioButton->setToolTip(tr("Shortcut: %1").arg(action->shortcut().toString()));
	buttonGroup->addButton(radioButton, player.getNum());
	radioButtonsLayout.addWidget(radioButton);

	connect(radioButton, &QRadioButton::clicked, this, [this, player]()
		{
			selectedPlayer = player;
		});

	addAction(action);
	connect(action, &QAction::triggered, this, [radioButton]()
		{
			if(radioButton->isEnabled())
				radioButton->click();
		});
}
