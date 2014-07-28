///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// txoutview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

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

