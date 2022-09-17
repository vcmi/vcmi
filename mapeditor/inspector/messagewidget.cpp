#include "messagewidget.h"
#include "ui_messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MessageWidget)
{
	ui->setupUi(this);
}

MessageWidget::~MessageWidget()
{
	delete ui;
}

void MessageWidget::setMessage(const QString & m)
{
	ui->messageEdit->setPlainText(m);
}

QString MessageWidget::getMessage() const
{
	return ui->messageEdit->toPlainText();
}

QWidget * MessageDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return new MessageWidget(parent);
}

void MessageDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	if(auto *ed = qobject_cast<MessageWidget *>(editor))
	{
		ed->setMessage(index.data().toString());
	}
	else
	{
		QStyledItemDelegate::setEditorData(editor, index);
	}
}

void MessageDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	if(auto *ed = qobject_cast<MessageWidget *>(editor))
	{
		model->setData(index, ed->getMessage());
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}
}
