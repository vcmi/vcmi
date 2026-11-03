/*
 * objectbrowser.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <QSortFilterProxyModel>
#include "../lib/GameConstants.h"


class ObjectBrowserProxyModel : public QSortFilterProxyModel
{
public:
	explicit ObjectBrowserProxyModel(QObject *parent = nullptr);
	
	Qt::ItemFlags flags(const QModelIndex &index) const override;
		
	QMimeData * mimeData(const QModelIndexList & indexes) const override;

	TerrainId terrain;
	QString filter;
	QString category;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
	bool filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const;
	bool lessThan(const QModelIndex & left, const QModelIndex & right) const override;
};

class ObjectBrowser : public QListView
{
public:
	ObjectBrowser(QWidget * parent);

	void setGridMode(bool grid);
	
protected:
	void startDrag(Qt::DropActions supportedActions) override;
};

class ObjectCategoryList : public QListWidget
{
public:
	ObjectCategoryList(QWidget * parent);

	void fitHeight();

protected:
	void resizeEvent(QResizeEvent * event) override;
};
