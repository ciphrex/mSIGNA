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

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    void setModel(TxModel* model);
    #pragma clang diagnostic pop

    void setMenu(QMenu* menu) { m_menu = menu; }
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    TxModel* m_model;

    QMenu* m_menu;
};

