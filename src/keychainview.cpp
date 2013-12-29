///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainview.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "keychainview.h"

#include <QMenu>
#include <QContextMenuEvent>

KeychainView::KeychainView(QWidget* parent)
    : QTreeView(parent), menu(NULL)
{
}

void KeychainView::update()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

void KeychainView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}
