///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// keychainview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "keychainview.h"

#include <QMenu>
#include <QContextMenuEvent>

KeychainView::KeychainView(QWidget* parent)
    : QTreeView(parent), menu(NULL)
{
}

QList<QString> KeychainView::getAllKeychains() const
{
    QList<QString> keychainNames;
    if (model())
    {
        for (int i = 0; i < model()->rowCount(); i++)
        {
            keychainNames << model()->data(model()->index(i, 0)).toString();
        }
    }
    return keychainNames;
}

QList<QString> KeychainView::getSelectedKeychains() const
{
    QList<QString> keychainNames;
    if (model())
    {
        QModelIndexList indexes = selectionModel()->selectedRows(0);

        for (auto& index: indexes)
        {
            keychainNames << model()->data(index).toString();
        }
    }
    return keychainNames;
}

void KeychainView::updateColumns()
{
    if (!model()) return;
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }
}

void KeychainView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}
