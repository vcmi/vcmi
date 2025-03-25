/*
 * PlayerSelectionDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QDialog>
#include <QComboBox>
#include <QCheckBox>
#include <QDialogButtonBox>
#include "../source/lib/constants/EntityIdentifiers.h"

/// Dialog shown when a hero cannot be placed as NEUTRAL.
/// Allows the user to select a valid player via checkboxes,
/// or using the existing keyboard shortcuts from MainWindow's player QActions.
class PlayerSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSelectionDialog(QWidget * parent = nullptr);
	PlayerColor getSelectedPlayer() const;

private slots:
	void onCheckboxToggled(bool checked);

private:
	std::vector<QCheckBox *> checkboxes;
	PlayerColor selectedPlayer;

	QFont font;
	QVBoxLayout mainLayout;
	QVBoxLayout checkboxLayout;

	void setupDialogComponents();
	void addCheckbox(QAction * checkboxAction, PlayerColor player, bool isEnabled, bool * isDefault);

};
