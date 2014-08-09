///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signatureview.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "signatureview.h"
#include "signaturemodel.h"

#include <QMenu>
#include <QContextMenuEvent>

SignatureView::SignatureView(QWidget* parent)
    : QTreeView(parent), m_model(nullptr), m_menu(nullptr)
{
}

void SignatureView::setModel(SignatureModel* model)
{
    QTreeView::setModel(model);
    m_model = model;
}

void SignatureView::updateColumns()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    QTreeView::update();
}

void SignatureView::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_menu) m_menu->exec(event->globalPos());
}

