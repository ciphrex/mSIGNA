///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signatureview.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

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

public slots:
    void update();

protected:
    void contextMenuEvent(QContextMenuEvent* event);

private:
    SignatureModel* m_model;
};

