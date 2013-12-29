///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txoutview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_TXOUTVIEW_H
#define COINVAULT_TXOUTVIEW_H

#include <QTreeView>

class TxOutView : public QTreeView
{
    Q_OBJECT

public:
    TxOutView(QWidget* parent = NULL);

    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
};

#endif // COINVAULT_TXOUTVIEW_H
