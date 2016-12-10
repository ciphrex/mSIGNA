///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// signaturedialog.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

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
    void error(const QString& error);
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

