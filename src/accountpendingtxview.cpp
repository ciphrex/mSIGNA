///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// accountpendingtxview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txview.h"
#include "txmodel.h"

#include <QMenu>
#include <QContextMenuEvent>

TxView::TxView(QWidget* parent)
    : QTreeView(parent), accountHistoryModel(NULL)
{
}

void TxView::setModel(TxModel* model)
{
    QTreeView::setModel(model);
    accountHistoryModel = model;
}

void TxView::update()
{
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }
}

void TxView::contextMenuEvent(QContextMenuEvent* /*event*/)
{
/*
    QMenu menu(this);
    if (exportPrivateAction)    menu.addAction(exportPrivateAction);
    if (exportPublicAction)     menu.addAction(exportPublicAction);
    if (!menu.isEmpty())        menu.exec(event->globalPos());
*/
}
