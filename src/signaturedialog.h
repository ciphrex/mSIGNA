///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signaturedialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinCore/typedefs.h>

namespace CoinDB { class SynchedVault; }

class SignatureModel;
class SignatureView;
class SignatureActions;

class QLabel;

#include <QDialog>

class SignatureDialog : public QDialog
{
    Q_OBJECT

public:
    SignatureDialog(CoinDB::SynchedVault& synchedVault, const bytes_t& txHash, QWidget* parent = nullptr);
    ~SignatureDialog();

    SignatureModel* getModel() const { return m_model; }
    SignatureView* getView() const { return m_view; }

public slots:
    void updateTx();
    void updateKeychains();

signals:
    void keychainsUpdated();
    void txUpdated();

private:
    CoinDB::SynchedVault& m_synchedVault;

    SignatureModel* m_model;
    SignatureView* m_view;
    SignatureActions* m_actions;

    QLabel* m_sigsNeededLabel;

    void updateCaption();
};

