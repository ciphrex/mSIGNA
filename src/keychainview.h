///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// keychainview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QMenu;

#include <QTreeView>

class KeychainView : public QTreeView
{
    Q_OBJECT

public:
    KeychainView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

