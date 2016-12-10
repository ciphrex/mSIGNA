///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signatureview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class SignatureModel;

#include <QTreeView>

class SignatureView : public QTreeView
{
    Q_OBJECT

public:
    SignatureView(QWidget* parent = nullptr);

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
    void setModel(SignatureModel* model);
    #pragma clang diagnostic pop

    void setMenu(QMenu* menu) { m_menu = menu; }

public slots:
    void updateColumns();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    SignatureModel* m_model;
    QMenu* m_menu;
};

