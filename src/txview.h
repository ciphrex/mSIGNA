///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_TXVIEW_H
#define COINVAULT_TXVIEW_H

class TxModel;

#include <QTreeView>

class TxView : public QTreeView
{
    Q_OBJECT

public:
    TxView(QWidget* parent = NULL);

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    void setModel(TxModel* model);
    #pragma clang diagnostic pop

    void setMenu(QMenu* menu) { this->menu = menu; }
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    TxModel* accountHistoryModel;

    QMenu* menu;
};

#endif // COINVAULT_TXVIEW_H
