///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "accountview.h"
#include "accountmodel.h"

#include "txdialog.h"

#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>

AccountView::AccountView(QWidget* parent)
    : QTreeView(parent), menu(NULL)
{
}

void AccountView::updateColumns()
{
    if (!model()) return;
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }
}

void AccountView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}

