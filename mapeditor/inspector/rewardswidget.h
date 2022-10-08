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
 
/*
 ui32 gainedExp;
 si32 manaDiff; //amount of gained / lost mana
 si32 moraleDiff; //morale modifier
 si32 luckDiff; //luck modifier
 TResources resources;//gained / lost resources
 std::vector<si32> primskills;//gained / lost prim skills
 std::vector<SecondarySkill> abilities; //gained abilities
 std::vector<si32> abilityLevels; //levels of gained abilities
 std::vector<ArtifactID> artifacts; //gained artifacts
 std::vector<SpellID> spells; //gained spells
 CCreatureSet creatures; //gained creatures
 */

const std::array<std::string, 10> rewardTypes{"Experience", "Mana", "Morale", "Luck", "Resource", "Primary skill", "Secondary skill", "Artifact", "Spell", "Creature"};

class RewardsWidget : public QDialog
{
	Q_OBJECT

public:
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
	void addReward(int typeId, int listId, int amount);
	QList<QString> getListForType(int typeId);
	
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
