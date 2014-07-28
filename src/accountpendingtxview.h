///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

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

