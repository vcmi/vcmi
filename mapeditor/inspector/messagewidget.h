/*
 * messagewidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <QDialog>

namespace Ui {
class MessageWidget;
}

class MessageWidget : public QDialog
{
	Q_OBJECT

public:
	explicit MessageWidget(QWidget *parent = nullptr);
	~MessageWidget();
	
	void setMessage(const QString &);
	QString getMessage() const;

private:
	Ui::MessageWidget *ui;
};


class MessageDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;
	
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

