///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// txview.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#include "txview.h"
#include "txmodel.h"

#include <QMenu>
#include <QContextMenuEvent>

TxView::TxView(QWidget* parent)
    : QTreeView(parent), m_model(nullptr), m_menu(nullptr)
{
    setSelectionBehavior(QAbstractItemView::SelectRows);
}

void TxView::setModel(TxModel* model)
{
    QTreeView::setModel(model);
    m_model = model;
}

void TxView::updateColumns()
{
    if (!m_model) return;
    for (int i = 0; i < m_model->columnCount(); i++) { resizeColumnToContents(i); }
}

void TxView::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_menu) m_menu->exec(event->globalPos());
}

