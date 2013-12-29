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
    : QTreeView(parent), accountHistoryModel(NULL), menu(NULL)
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

void TxView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}

