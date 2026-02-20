/*
 * baseinspectoritemdelegate.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <QStyledItemDelegate>
#include <QAbstractItemModel>

class BaseInspectorItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    virtual void updateModelData(QAbstractItemModel * model, const QModelIndex & index) const {};
    void setModelTextData(QAbstractItemModel * model, const QModelIndex & index, const QStringList & list) const
    {
        QString text = list.join("\n");
        model->setItemData(index, {
            {Qt::DisplayRole, QVariant{text}},
            {Qt::ToolTipRole, QVariant{text}}
        });
    };
};
