///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// signaturemodel.h
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <QStandardItemModel>

#include <CoinDB/Vault.h>

class SignatureModel : public QStandardItemModel
{
    Q_OBJECT

public:
    SignatureModel(QObject* parent = nullptr);
    SignatureModel(CoinDB::Vault* vault, const bytes_t& txHash, QObject* parent = nullptr);

    void setVault(CoinDB::Vault* vault);
    void setTxHash(const bytes_t& txHash);
    const bytes_t getTxHash() const { return m_txHash; }
    void update();

    unsigned int getSigsNeeded() const { return m_sigsNeeded; }

private:
    void initColumns();

    CoinDB::Vault* m_vault;
    bytes_t m_txHash;
    unsigned int m_sigsNeeded;
};

