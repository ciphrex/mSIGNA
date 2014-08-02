///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signatureactions.h
//
// Copyright (c) 20132014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QObject>

class QAction;
class QMenu;

class QString;

class SignatureModel;
class SignatureView;

namespace CoinDB
{
    class SynchedVault;
}

class SignatureActions : public QObject
{
    Q_OBJECT

public:
    SignatureActions(CoinDB::SynchedVault& synchedVault, SignatureModel& signatureModel, SignatureView& signatureView);

    QMenu* getMenu() const { return menu; }

//    void updateVaultStatus();

signals:
    void error(const QString& message);

private slots:
    void updateCurrentKeychain(const QModelIndex& current, const QModelIndex& previous);
    void addSignature();

private:
    void createActions();
    void createMenus();

    CoinDB::SynchedVault& m_synchedVault;

    SignatureModel& m_signatureModel;
    SignatureView& m_signatureView;

    QString currentKeychain;

    QAction* addSignatureAction;

    QMenu* menu;
};

