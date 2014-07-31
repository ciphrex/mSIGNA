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
    : QTreeView(parent), m_model(nullptr)
{
}

void SignatureView::setModel(SignatureModel* model)
{
    QTreeView::setModel(model);
    m_model = model;
}

void SignatureView::update()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    QTreeView::update();
}

void SignatureView::contextMenuEvent(QContextMenuEvent* /*event*/)
{
/*
    QMenu menu(this);
    if (exportPrivateAction)    menu.addAction(exportPrivateAction);
    if (exportPublicAction)     menu.addAction(exportPublicAction);
    if (!menu.isEmpty())        menu.exec(event->globalPos());
*/
}
