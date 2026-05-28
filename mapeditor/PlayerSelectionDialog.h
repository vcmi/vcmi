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
#include <QVBoxLayout>
#include "../lib/constants/EntityIdentifiers.h"

VCMI_LIB_USING_NAMESPACE

class QRadioButton;
class QButtonGroup;
class EditorMainWindow;

/// Dialog shown when a hero cannot be placed as NEUTRAL.
/// Allows the user to select a valid player via checkboxes,
/// or using the existing keyboard shortcuts from EditorMainWindow's player QActions.
class PlayerSelectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSelectionDialog(EditorMainWindow * mainWindow);
	PlayerColor getSelectedPlayer() const;

private:
	const int dialogWidth = 320;

	QButtonGroup * buttonGroup;
	PlayerColor selectedPlayer;

	QFont font;
	QVBoxLayout mainLayout;
	QVBoxLayout radioButtonsLayout;

	void setupDialogComponents();
	void addRadioButton(QAction * action, PlayerColor player);

};
