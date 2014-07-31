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

#include <CoinDB/Vault.h>

class SignatureModel;
class SignatureView;

class QLabel;

#include <QDialog>

class SignatureDialog : public QDialog
{
    Q_OBJECT

public:
    SignatureDialog(CoinDB::Vault* vault, const bytes_t& txHash, QWidget* parent = nullptr);

private:
    SignatureModel* m_model;
    SignatureView* m_view;

    QLabel* m_sigsNeededLabel;
};

