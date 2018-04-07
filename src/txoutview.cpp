///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txoutview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txoutview.h"

#include <QMenu>
#include <QContextMenuEvent>

#include <QMessageBox>

TxOutView::TxOutView(QWidget* parent)
    : QTreeView(parent)
{
}

void TxOutView::update()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void TxOutView::contextMenuEvent(QContextMenuEvent* /*event*/)
{
/*
    QMenu menu(this);
    if (exportPrivateAction)    menu.addAction(exportPrivateAction);
    if (exportPublicAction)     menu.addAction(exportPublicAction);
    if (!menu.isEmpty())        menu.exec(event->globalPos());
*/
}
