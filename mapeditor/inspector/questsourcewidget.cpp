/*
 * questsourcewidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../helper.h"
#include "questsourcewidget.h"
#include "ui_questsourcewidget.h"
#include "../mapcontroller.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/constants/StringConstants.h"
#include "../../lib/mapping/CMap.h"

#include <rewardable/Reward.h>
#include "../translator.h"
#include "segments/rewardwidget.h"
#include "segments/questwidget.h"

QuestSourceWidget::QuestSourceWidget(MapController & _controller, QuestSource & questSource, QWidget *parent) :
	QDialog(parent),
	controller(_controller),
	questSource(questSource),
	ui(new Ui::QuestWidget)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	ui->setupUi(this);
	Helper::decorateDialog(this);

	rewardWidget = std::make_unique<RewardWidget>(*controller.map(), questSource.id, this);
	rewardWidget->disableRemoveObjectOption();
	ui->rewardContainer->layout()->addWidget(rewardWidget.get());
	disableReward(true);

	questWidget = std::make_unique<QuestWidget>(controller,  this);
	questWidget->showMapAction([this](){this->hide();});
	questWidget->returnToWidgetAction([this](){this->show();});
	//single quests are always before repeatable quests, so we need to reorder the quest list
	QObject::connect(questWidget.get(), &QuestWidget::repeatableClicked, [this]()
	{
		commitChanges();
		prepareQuestsList(selectedQuest);
	});
	ui->questContainer->layout()->addWidget(questWidget.get());
	disableQuest(true);

	if (!questSource.allQuests().empty())
		selectedQuest = questSource.allQuestsEditor()[0];
}

QuestSourceWidget::~QuestSourceWidget()
{
	delete ui;
}

void QuestSourceWidget::loadQuestData()
{
	if (!selectedQuest)
	{
		disableReward(true);
		rewardWidget->clearData();
		disableQuest(true);
		questWidget->clearData();

		ui->rewardButton->setDisabled(true);
		ui->moveUpButton->setDisabled(true);
		ui->moveDownButton->setDisabled(true);
		ui->textsWidget->hide();
		return;
	}
	else
	{
		questWidget->loadQuest(selectedQuest.get());
		disableQuest(false);
		ui->textsWidget->show();
		ui->rewardButton->setDisabled(false);
		ui->moveUpButton->setDisabled(false);
		ui->moveDownButton->setDisabled(false);
		if (selectedQuest->reward)
		{
			rewardWidget->loadReward(&(selectedQuest->reward.value().reward));
			disableReward(false);
		}
		else
		{
			disableReward(true);
			rewardWidget->clearData();
		}
	}

	ui->firstVisitText->clear();
	ui->firstVisitText->append(QString::fromStdString(selectedQuest->firstVisitText.toString(&Translator::instance())));

	ui->nextVisitText->clear();
	ui->nextVisitText->append(QString::fromStdString(selectedQuest->nextVisitText.toString(&Translator::instance())));

	ui->completedText->clear();
	ui->completedText->append(QString::fromStdString(selectedQuest->completedText.toString(&Translator::instance())));
	questDataLoaded = true;

}

void QuestSourceWidget::obtainData()
{
	prepareQuestsList();
	selectQuest(0);
}

void QuestSourceWidget::prepareQuestsList(const std::shared_ptr<Quest> & questToSelect)
{
	auto & allQuests = questSource.allQuestsEditor();
	ui->questsList->clear();

	std::ranges::stable_sort(allQuests, [](auto & a, auto & b)
					 {
						 return !a->repeatedQuest && b->repeatedQuest;
					 });

	setTranslationIdentifiers();	//translation identifiers depend on the order

	for(int i = 0; i< allQuests.size(); ++i)
	{
		ui->questsList->addItem(tr("%1 quest on position %2")
									.arg(tr(allQuests[i]->repeatedQuest ? "Repeated" : "One time"))
									.arg(i));
		if (questToSelect && questSource.allQuests()[i] == questToSelect)
		{
			ui->questsList->setCurrentRow(i);
			loadQuestData();
		}
	}
	if (ui->questsList->count() == 0)
		selectedQuest = nullptr;
}

void QuestSourceWidget::setTranslationIdentifiers()
{
	auto & allQuests = questSource.allQuestsEditor();
	std::vector<std::string> fv;
	std::vector<std::string> nv;
	std::vector<std::string> com;
	for(int i = 0; i< allQuests.size(); ++i) {
		fv.push_back(allQuests[i]->firstVisitText.toString(&Translator::instance()));
		nv.push_back(allQuests[i]->nextVisitText.toString(&Translator::instance()));
		com.push_back(allQuests[i]->completedText.toString(&Translator::instance()));
	}

	for(int i = 0; i< allQuests.size(); ++i) {
		setTranslation(allQuests[i]->firstVisitText, TextIdentifier("quest", questSource.instanceName, i, "firstVisit"), fv[i]);
		setTranslation(allQuests[i]->nextVisitText, TextIdentifier("quest", questSource.instanceName, i, "nextVisit"), nv[i]);
		setTranslation(allQuests[i]->completedText, TextIdentifier("quest", questSource.instanceName, i, "completed"), com[i]);
	}
}

void QuestSourceWidget::setTranslation(MetaString & metastring, const TextIdentifier & identifier, const std::string & translation)
{
	metastring = MetaString::createFromTextID(mapRegisterLocalizedString(
		"map", *controller.map(), identifier, translation));
	if(translation.empty())
		metastring.clear();
}

void QuestSourceWidget::selectQuest(int index)
{
	if (questSource.allQuests().size() > index)
	{
		ui->questsList->setCurrentRow(index);
		selectedQuest = questSource.allQuestsEditor()[index];
	}
	loadQuestData();
}

bool QuestSourceWidget::commitChanges()
{
	if (!selectedQuest)
		return false;

	if(selectedQuest->reward)
		selectedQuest->reward->visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
	rewardWidget->commit();
	questWidget->commit();

	auto index = std::find(questSource.allQuests().begin(), questSource.allQuests().end(), selectedQuest) - questSource.allQuests().begin();
	setTranslation(selectedQuest->firstVisitText, TextIdentifier("quest", questSource.instanceName, index, "firstVisit"), ui->firstVisitText->toPlainText().toStdString());
	setTranslation(selectedQuest->nextVisitText, TextIdentifier("quest", questSource.instanceName, index, "nextVisit"), ui->nextVisitText->toPlainText().toStdString());
	setTranslation(selectedQuest->completedText, TextIdentifier("quest", questSource.instanceName, index, "completed"), ui->completedText->toPlainText().toStdString());
	
	return true;
}

void QuestSourceWidget::shiftSelectedQuest(int shift)
{
	auto & quests = questSource.allQuestsEditor();
	auto currentPosition = vstd::find_pos(quests, selectedQuest);
	if (currentPosition != -1 && currentPosition + shift < quests.size() && currentPosition + shift >= 0)
	{
		auto it = quests.begin() + currentPosition;
		if((*it)->repeatedQuest == (*(it + shift))->repeatedQuest)
			iter_swap(it, it + shift);
	}
	prepareQuestsList(selectedQuest);
}

void QuestSourceWidget::disableReward(bool disabled)
{
		ui->rewardContainer->setDisabled(disabled);
		ui->rewardButton->setText(disabled ? tr("Add reward") : tr("Remove reward"));
}

void QuestSourceWidget::disableQuest(bool disabled)
{
	ui->questContainer->setDisabled(disabled);
}

void QuestSourceWidget::on_addQuestButton_clicked()
{
	questSource.addQuest();
	prepareQuestsList();
	selectQuest(ui->questsList->count() - 1);
}

void QuestSourceWidget::on_rewardButton_clicked()
{
	if (selectedQuest)
	{
		if (!selectedQuest->reward)
		{
			selectedQuest->reward = Rewardable::VisitInfo();
			rewardWidget->loadReward(&(selectedQuest->reward.value().reward));
			disableReward(false);
		}
		else
		{
			selectedQuest->reward = std::nullopt;
			rewardWidget->clearData();
			disableReward(true);
		}
	}
}

void QuestSourceWidget::on_moveUpButton_clicked()
{
	shiftSelectedQuest(-1);
}

void QuestSourceWidget::on_moveDownButton_clicked()
{
	shiftSelectedQuest(1);
}

void QuestSourceWidget::on_deleteQuestButton_clicked()
{
	if (questSource.allQuests().empty())
		return;
	selectedQuest = nullptr;
	questWidget->clearData();
	questSource.allQuestsEditor().erase(questSource.allQuests().begin() + ui->questsList->currentRow());
	prepareQuestsList();
	selectQuest(0);
}

void QuestSourceWidget::on_questsList_currentRowChanged(int row)
{
	if (!questDataLoaded || row == -1)
		return;

	commitChanges();
	questDataLoaded = false;
	selectQuest(row);
}

QuestSourceDelegate::QuestSourceDelegate(MapController & c, QuestSource & questSource): BaseInspectorItemDelegate(), controller(c), questSource(questSource)
{
}

QWidget * QuestSourceDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	return new QuestSourceWidget(controller, questSource, parent);
}

void QuestSourceDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestSourceWidget *>(editor))
	{
		ed->obtainData();
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void QuestSourceDelegate::setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const
{
	if(auto *ed = qobject_cast<QuestSourceWidget *>(editor))
	{
		ed->commitChanges();
		updateModelData(model, index);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}

bool QuestSourceDelegate::eventFilter(QObject * object, QEvent * event)
{
	if(auto * ed = qobject_cast<QuestSourceWidget *>(object))
	{
		if(event->type() == QEvent::Hide || event->type() == QEvent::FocusOut)
			return false;
		if(event->type() == QEvent::Close)
		{
			commitData(ed);
			closeEditor(ed);
			return true;
		}
	}
	return QStyledItemDelegate::eventFilter(object, event);
}
