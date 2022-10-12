/*
 * rewardswidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>
#include "../lib/mapObjects/CGPandoraBox.h"
#include "../lib/mapping/CMap.h"

namespace Ui {
class RewardsWidget;
}

const std::array<std::string, 10> rewardTypes{"Experience", "Mana", "Morale", "Luck", "Resource", "Primary skill", "Secondary skill", "Artifact", "Spell", "Creature"};

class RewardsWidget : public QDialog
{
	Q_OBJECT

public:
	enum RewardType
	{
		EXPERIENCE = 0, MANA, MORALE, LUCK, RESOURCE, PRIMARY_SKILL, SECONDARY_SKILL, ARTIFACT, SPELL, CREATURE
	};
	
	explicit RewardsWidget(const CMap &, CGPandoraBox &, QWidget *parent = nullptr);
	~RewardsWidget();
	
	void obtainData();
	bool commitChanges();

private slots:
	void on_rewardType_activated(int index);

	void on_rewardList_activated(int index);

	void on_buttonAdd_clicked();

	void on_buttonRemove_clicked();

	void on_buttonClear_clicked();

	void on_rewardsTable_itemSelectionChanged();

private:
	void addReward(RewardType typeId, int listId, int amount);
	QList<QString> getListForType(RewardType typeId);
	
	Ui::RewardsWidget *ui;
	CGPandoraBox * pandora;
	const CMap & map;
	int rewards = 0;
};

class RewardsPandoraDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	
	RewardsPandoraDelegate(const CMap &, CGPandoraBox &);
	
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	
private:
	CGPandoraBox & pandora;
	const CMap & map;
};
