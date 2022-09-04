#ifndef INSPECTOR_H
#define INSPECTOR_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include "../lib/int3.h"
#include "../lib/GameConstants.h"

class CGObjectInstance;
class CGTownInstance;

class Initializer
{
public:
	Initializer(CGObjectInstance *);
};

class Inspector
{
public:
	Inspector(CGObjectInstance *, QTableWidget *);

	void setProperty(const QString & key, const QVariant & value);

	void updateProperties();

protected:
	QTableWidgetItem * addProperty(int value);
	QTableWidgetItem * addProperty(const std::string & value);
	QTableWidgetItem * addProperty(const QString & value);
	QTableWidgetItem * addProperty(const int3 & value);
	QTableWidgetItem * addProperty(const PlayerColor & value);
	QTableWidgetItem * addProperty(bool value);
	QTableWidgetItem * addProperty(CGObjectInstance * value);

	void setProperty(CGTownInstance * obj, const QString & key, const QVariant & value);

	template<class T>
	void addProperty(const QString & key, const T & value, bool restricted = true)
	{
		auto * itemKey = new QTableWidgetItem(key);
		auto * itemValue = addProperty(value);
		itemKey->setFlags(Qt::NoItemFlags);
		if(restricted)
			itemValue->setFlags(Qt::NoItemFlags);

		if(table->rowCount() < row + 1)
			table->setRowCount(row + 1);
		table->setItem(row, 0, itemKey);
		table->setItem(row, 1, itemValue);
		++row;
	}

protected:
	int row = 0;
	QTableWidget * table;
	CGObjectInstance * obj;
};

class PlayerColorDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	//void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	//QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QWidget * createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private slots:
	void commitAndCloseEditor(int);
};

#endif // INSPECTOR_H
