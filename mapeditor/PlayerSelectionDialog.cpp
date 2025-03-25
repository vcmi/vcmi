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
#include "mainwindow.h"
#include "../lib/mapping/CMap.h"

PlayerSelectionDialog::PlayerSelectionDialog(QWidget * parent)
	: QDialog(parent), selectedPlayer(PlayerColor::NEUTRAL)
{
	auto main = qobject_cast<MainWindow *>(parent);
	assert(main);

	setupDialogComponents();

	int maxPlayers = 0;
	if(main && main->controller.map())
		maxPlayers = main->controller.map()->players.size();
	
	bool defaultIsChecked = false;

	for(int i = 0; i < maxPlayers; ++i)
	{
		PlayerColor player(i);
		QAction * action = main->getActionPlayer(player);

		addCheckbox(action, player, main->controller.map()->players.at(i).canAnyonePlay(), & defaultIsChecked);
	}
}

void PlayerSelectionDialog::onCheckboxToggled(bool checked)
{
	if(!checked)
		return;

	QCheckBox * senderCheckBox = qobject_cast<QCheckBox *>(sender());
	if(!senderCheckBox)
		return;

	for(int i = 0; i < checkboxes.size(); ++i)
	{
		QCheckBox * cb = checkboxes[i];
		if(cb == senderCheckBox)
		{
			selectedPlayer = PlayerColor(i);
		}
		else
		{
			cb->blockSignals(true);
			cb->setChecked(false);
			cb->blockSignals(false);
		}
	}
}


PlayerColor PlayerSelectionDialog::getSelectedPlayer() const
{
	return selectedPlayer;
}

void PlayerSelectionDialog::setupDialogComponents()
{
	setWindowTitle(tr("Select Player"));
	setFixedSize(sizeHint());
	setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
	font.setPointSize(10);
	setFont(font);

	mainLayout.setSpacing(10);
	mainLayout.setContentsMargins(20, 20, 80, 20);
	checkboxLayout.setContentsMargins(0, 10, 0, 20);
	mainLayout.addSpacing(4);

	QLabel * errorLabel = new QLabel(tr("Hero cannot be created as NEUTRAL"), this);
	font.setBold(true);
	errorLabel->setFont(font);
	mainLayout.addWidget(errorLabel);

	QLabel * instructionLabel = new QLabel(tr("Switch to one of the available players:"), this);
	font.setBold(false);
	instructionLabel->setFont(font);
	mainLayout.addWidget(instructionLabel);

	QWidget * checkboxContainer = new QWidget(this);
	checkboxContainer->setLayout(& checkboxLayout);
	mainLayout.addWidget(checkboxContainer);

	QDialogButtonBox * box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	mainLayout.addWidget(box);

	setLayout(& mainLayout);
}

void PlayerSelectionDialog::addCheckbox(QAction * checkboxAction, PlayerColor player, bool isEnabled, bool * isDefault)
{
	QHBoxLayout * rowLayout = new QHBoxLayout();
	auto * checkbox = new QCheckBox(checkboxAction->text(), this);
	
	QLabel * shortcutLabel = new QLabel(checkboxAction->shortcut().toString(), this);
	QFont shortcutFont = font;
	shortcutFont.setPointSize(9);
	shortcutFont.setItalic(true);
	shortcutLabel->setFont(shortcutFont);
	shortcutLabel->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

	QWidget * checkboxContainer = new QWidget(this);
	QHBoxLayout * cbLayout = new QHBoxLayout(checkboxContainer);
	cbLayout->setContentsMargins(0, 0, 0, 0);
	cbLayout->addWidget(checkbox, 0, Qt::AlignCenter);

	rowLayout->addWidget(checkboxContainer, 1);
	rowLayout->addWidget(shortcutLabel, 1);

	checkbox->setEnabled(isEnabled);
	if(checkbox->isEnabled() && !*isDefault)
	{
		checkbox->setChecked(true);
		selectedPlayer = player;
		*isDefault = true;
	}

	checkboxLayout.addLayout(rowLayout);

	connect(checkbox, &QCheckBox::clicked, this, [=]()
		{
			selectedPlayer = player;

			// Radio-style logic: uncheck other boxes
			for(auto * box : findChildren<QCheckBox *>())
			{
				if(box != checkbox)
					box->setChecked(false);
			}
		});


	// Add action to the dialog for shortcut support
	addAction(checkboxAction);

	// Connect action trigger to simulate checkbox click
	connect(checkboxAction, &QAction::triggered, this, [=]()
		{
			if(checkbox->isEnabled())
				checkbox->click();
		});
}
