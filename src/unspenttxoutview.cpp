///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// unspenttxoutview.cpp
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#include "unspenttxoutview.h"
#include "unspenttxoutmodel.h"

#include <QMenu>
#include <QContextMenuEvent>

UnspentTxOutView::UnspentTxOutView(QWidget* parent)
    : QTreeView(parent), model(nullptr), menu(nullptr)
{
    setSelectionMode(UnspentTxOutView::MultiSelection);
}

void UnspentTxOutView::setModel(UnspentTxOutModel* model)
{
    QTreeView::setModel(model);
    this->model = model;
}

void UnspentTxOutView::update()
{
    QTreeView::update();
    for (int i = 0; i < model->columnCount(); i++) { resizeColumnToContents(i); }
}

void UnspentTxOutView::contextMenuEvent(QContextMenuEvent* event)
{
    if (menu) menu->exec(event->globalPos());
}

