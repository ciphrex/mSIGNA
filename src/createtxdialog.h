///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// createtxdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef COINVAULT_CREATETXDIALOG_H
#define COINVAULT_CREATETXDIALOG_H

class QTextEdit;

#include <QDialog>
#include <QHBoxLayout>

// TODO: move TaggedOutput to another file, remove the following dependency
#include "accountmodel.h"
#include "paymentrequest.h"

#include <CoinQ_typedefs.h>

class QComboBox;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

class TxOutLayout : public QHBoxLayout
{
    Q_OBJECT

public:
    TxOutLayout(QWidget* parent = NULL);

    void setAddress(const QString& address);
    QString getAddress() const;

    bytes_t getScript() const;

    void setValue(uint64_t value);
    uint64_t getValue() const;

    void setRecipient(const QString& recipient);
    QString getRecipient() const;

    QPushButton* getRemoveButton() const { return removeButton; }

    void removeWidgets();

private:
    QLineEdit* addressEdit;
    QLineEdit* amountEdit;
    QLineEdit* recipientEdit;
    QPushButton* removeButton;
};


class CreateTxDialog : public QDialog
{
    Q_OBJECT

public:
    CreateTxDialog(const QString& accountName, const PaymentRequest& paymentRequest = PaymentRequest(), QWidget* parent = NULL);

    QString getAccountName() const;
    uint64_t getFeeValue() const;
    std::vector<TaggedOutput> getOutputs();

    enum status_t { SAVE_ONLY, SIGN_AND_SAVE, SIGN_AND_SEND };
    status_t getStatus() const { return status; }

public slots:
    void addTxOut(const PaymentRequest& paymentRequest = PaymentRequest());

private slots:
    void removeTxOut(TxOutLayout* txOutLayout);

private:
    QComboBox* accountComboBox;
    QLineEdit* feeEdit;
    QVBoxLayout* txOutVBoxLayout;
    QVBoxLayout* mainLayout;

    std::set<TxOutLayout*> txOutLayouts;

    QPushButton* signAndSendButton;
    QPushButton* signAndSaveButton;
    QPushButton* saveButton;
    QPushButton* cancelButton;

    status_t status;
};

#endif // COINVAULT_CREATETXDIALOG_H
