#pragma once
#include "StdInc.h"
#include "lib/mapObjects/CRewardableObject.h"
#include <QWidget>
#include "lib/rewardable/Reward.h"

namespace Ui {
class RewardWidget;
}

class RewardWidget : public QWidget
{
	Q_OBJECT

public:

	explicit RewardWidget(CMap & m, ObjectInstanceID id, QWidget *parent);
	~RewardWidget();

	void loadReward(Rewardable::Reward * reward);
	void showData(Rewardable::Reward * reward);
	void commit();
	void clearData();
	void disableRemoveObjectOption();
	bool isRewardLoaded() const;

private slots:

	void on_rCreatureAdd_clicked();

	void on_rCreatureRemove_clicked();

	void on_castSpellCheck_toggled(bool checked);
	void on_bonusAdd_clicked();
	void on_bonusRemove_clicked();

private:

	void onCreatureAdd(QTableWidget * listWidget, QComboBox * comboWidget, QSpinBox * spinWidget);
	void printRewardInformation();

	Ui::RewardWidget *ui;
	ObjectInstanceID id;
	Rewardable::Reward * reward = nullptr;
	CMap & map;
};