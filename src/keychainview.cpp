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

void KeychainView::updateColumns()
{
    if (!model()) return;
    for (int i = 0; i < model()->columnCount(); i++) { resizeColumnToContents(i); }
}

void KeychainView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}
