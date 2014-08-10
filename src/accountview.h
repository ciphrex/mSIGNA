///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

class QMenu;

#include <QTreeView>

class AccountView : public QTreeView
{
    Q_OBJECT

public:
    AccountView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }
    void updateColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

