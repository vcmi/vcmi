#ifndef OBJECTBROWSER_H
#define OBJECTBROWSER_H

#include <QSortFilterProxyModel>
#include "../lib/Terrain.h"

class ObjectBrowser : public QSortFilterProxyModel
{
public:
	explicit ObjectBrowser(QObject *parent = nullptr);

	Terrain terrain;
	QString filter;

protected:
	bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const override;
	bool filterAcceptsRowText(int source_row, const QModelIndex &source_parent) const;
};

#endif // OBJECTBROWSER_H
