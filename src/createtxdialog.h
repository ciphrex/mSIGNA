///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// createtxdialog.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

class UnspentTxOutModel;
class UnspentTxOutView;

class QTextEdit;

#include <QDialog>
#include <QHBoxLayout>

// TODO: move TaggedOutput to another file, remove the following dependency
#include "accountmodel.h"
#include "paymentrequest.h"

#include <CoinQ/CoinQ_typedefs.h>

class QComboBox;
class QLineEdit;
class QCheckBox;
class QPushButton;
class QVBoxLayout;
class QItemSelection;

class TxOutLayout : public QHBoxLayout
{
    Q_OBJECT

public:
    TxOutLayout(uint64_t currencyDivisor, const QString& currencySymbol, uint64_t maxCurrencyValue, unsigned int maxCurrencyDecimals, QWidget* parent = nullptr);

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
    unsigned char base58_versions[2];

    QLineEdit* addressEdit;
    QLineEdit* amountEdit;
    QLineEdit* recipientEdit;
    QPushButton* removeButton;

    uint64_t currencyDivisor;
    QString currencySymbol;
    uint64_t currencyMax;
    unsigned int currencyDecimals;
};

class CoinControlWidget : public QWidget
{
    Q_OBJECT

public:
    CoinControlWidget(CoinDB::Vault* vault, const QString& accountName, QWidget* parent = nullptr);

    std::vector<unsigned long> getInputTxOutIds() const;

    void updateAll();

    // Windows repaint workaround
    void refreshView();

public slots:
    void updateTotal(const QItemSelection& selected, const QItemSelection& deselected);

private:
    UnspentTxOutModel* model;
    UnspentTxOutView* view;
    QLineEdit* totalEdit;
};

class CreateTxDialog : public QDialog
{
    Q_OBJECT

public:
    CreateTxDialog(const QString& accountName, const PaymentRequest& paymentRequest = PaymentRequest(), QWidget* parent = nullptr);
    CreateTxDialog(CoinDB::Vault* vault, const QString& accountName, const PaymentRequest& paymentRequest = PaymentRequest(), QWidget* parent = nullptr);

    QString getAccountName() const;
    uint64_t getFeeValue() const;
    std::vector<unsigned long> getInputTxOutIds() const;
    std::vector<std::shared_ptr<CoinDB::TxOut>> getTxOuts();
    std::vector<TaggedOutput> getOutputs();

    enum status_t { SAVE, SIGN };
    status_t getStatus() const { return status; }

public slots:
    void addTxOut(const PaymentRequest& paymentRequest = PaymentRequest());

private slots:
    void switchCoinControl(int state);
    void removeTxOut(TxOutLayout* txOutLayout);
    void setRemoveEnabled(bool enabled = true);

private:
    QComboBox* accountComboBox;
    QLineEdit* feeEdit;
    QCheckBox* coinControlCheckBox;
    CoinControlWidget* coinControlWidget;
    QVBoxLayout* txOutVBoxLayout;
    QVBoxLayout* mainLayout;

    std::set<TxOutLayout*> txOutLayouts;

    QPushButton* signButton;
    QPushButton* saveButton;
    QPushButton* cancelButton;

    status_t status;

    uint64_t currencyDivisor;
    QString currencySymbol;
    uint64_t currencyMax;
    unsigned int currencyDecimals;
    uint64_t defaultFee;
};

