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
#include <QMap>
#include "baseinspectoritemdelegate.h"
#include "../../lib/constants/EntityIdentifiers.h"

namespace Ui {
	class PlayerSelectionWidget;
}

class PlayerSelectionWidget : public QDialog
{
	Q_OBJECT

public:
	explicit PlayerSelectionWidget(const std::set<PlayerColor>& selectcurrentSelectioned, QWidget* parent = nullptr);
	~PlayerSelectionWidget();
	std::set<PlayerColor> selectedPlayers() const;
	void setSelectedPlayers(const std::set<PlayerColor>& players);

signals:
	void editingFinished();  
	void cancelEditing();   

private:
	Ui::PlayerSelectionWidget *ui;
	QMap<PlayerColor, QCheckBox*> colorCheckboxes;

	void populateCheckboxes();
};

class PlayerSelectionDelegate : public BaseInspectorItemDelegate
{
	Q_OBJECT
public:
	using BaseInspectorItemDelegate::BaseInspectorItemDelegate;

	PlayerSelectionDelegate(std::set<PlayerColor>&);

	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
private:
	std::set<PlayerColor> & colors;
};
