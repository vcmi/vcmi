#pragma once

#include "cmodlist.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

namespace ModFields
{
	enum EModFields
	{
		STATUS_ENABLED,
		STATUS_UPDATE,
		TYPE,
		NAME,
		VERSION,
		SIZE,
		AUTHOR,
		COUNT
	};
}

class CModListModel : public QAbstractTableModel, public CModList
{
	Q_OBJECT

	QVector<QString> indexToName;

	void endResetModel();
public:
	/// CModListContainer overrides
	void addRepository(QJsonObject data);
	void setLocalModList(QJsonObject data);
	void setModSettings(QJsonObject data);

	QString modIndexToName(int index) const;

	explicit CModListModel(QObject *parent = 0);
	
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	int rowCount(const QModelIndex &parent) const;
	int columnCount(const QModelIndex &parent) const;

	Qt::ItemFlags flags(const QModelIndex &index) const;
signals:
	
public slots:
	
};

class CModFilterModel : public QSortFilterProxyModel
{
	CModListModel * base;
	int filteredType;
	int filterMask;

	bool filterMatches(int modIndex) const;

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

	bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
public:
	void setTypeFilter(int filteredType, int filterMask);

	CModFilterModel(CModListModel * model, QObject *parent = 0);
};
