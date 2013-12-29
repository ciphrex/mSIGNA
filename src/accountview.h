///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// accountview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTVIEW_H
#define COINVAULT_ACCOUNTVIEW_H

class QMenu;

#include <QTreeView>

class AccountView : public QTreeView
{
    Q_OBJECT

public:
    AccountView(QWidget* parent = NULL);

    void setMenu(QMenu* menu) { this->menu = menu; }
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    QMenu* menu;
};

#endif // COINVAULT_ACCOUNTVIEW_H
