///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txview.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_ACCOUNTHISTORYVIEW_H
#define COINVAULT_ACCOUNTHISTORYVIEW_H

class TxModel;

#include <QTreeView>

class TxView : public QTreeView
{
    Q_OBJECT

public:
    TxView(QWidget* parent = NULL);

    void setModel(TxModel* model);
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    TxModel* accountHistoryModel;
};

#endif // COINVAULT_ACCOUNTHISTORYVIEW_H
