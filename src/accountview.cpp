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

void AccountView::update()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void AccountView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}
