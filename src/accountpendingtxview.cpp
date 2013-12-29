///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

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
