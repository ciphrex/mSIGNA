///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// newaccountdialog.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "newaccountdialog.h"

#include <QtAlgorithms>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QCalendarWidget>

#include <stdexcept>

NewAccountDialog::NewAccountDialog(const QList<QString>& allKeychains, const QList<QString>& selectedKeychains, QWidget* parent)
    : QDialog(parent)
{
    if (allKeychains.isEmpty()) {
        throw std::runtime_error("Names list cannot be empty.");
    }

    keychainSet = selectedKeychains.toSet();

    QList<QString> keychains = allKeychains;
    qSort(keychains.begin(), keychains.end());

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // Account Name
    QLabel* nameLabel = new QLabel();
    nameLabel->setText(tr("Account Name:"));
    nameEdit = new QLineEdit();

    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->setSizeConstraint(QLayout::SetNoConstraint);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);

    // Keychain List Widget
    QLabel* selectionLabel = new QLabel(tr("Select keychains:"));
    keychainListWidget = new QListWidget();
    for (auto& keychain: keychains)
    {
        QCheckBox* checkBox = new QCheckBox(keychain);
        checkBox->setCheckState(selectedKeychains.count(keychain) ? Qt::Checked : Qt::Unchecked);
        connect(checkBox, &QCheckBox::stateChanged, [=](int state) { updateSelection(keychain, state); });
        QListWidgetItem* item = new QListWidgetItem();
        keychainListWidget->addItem(item);
        keychainListWidget->setItemWidget(item, checkBox);
    }

    // Keychain Names
    minSigComboBox = new QComboBox();
    minSigLineEdit = new QLineEdit();
    minSigLineEdit->setAlignment(Qt::AlignRight);
    minSigComboBox->setLineEdit(minSigLineEdit);

    QLabel* keychainLabel = new QLabel();
    keychainLabel->setText(tr("Keychains:"));

    QString keychainText;
    for (int i = 0; i < keychains.size(); i++) {
        if (i != 0) { keychainText += ", "; }
        keychainText += keychains.at(i);
        minSigComboBox->addItem(QString::number(i + 1));        
    }
    keychainEdit = new QLineEdit();
    keychainEdit->setReadOnly(true);
    keychainEdit->setText(keychainText);

    QHBoxLayout* keychainLayout = new QHBoxLayout();
    keychainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    keychainLayout->addWidget(keychainLabel);
    keychainLayout->addWidget(keychainEdit);

    // Minimum Signatures
    QLabel *minSigLabel = new QLabel();
    minSigLabel->setText(tr("Minimum Signatures:"));

    QHBoxLayout* minSigLayout = new QHBoxLayout();
    minSigLayout->setSizeConstraint(QLayout::SetNoConstraint);
    minSigLayout->addWidget(minSigLabel);
    minSigLayout->addWidget(minSigComboBox);

    // Creation Time
    QDateTime localDateTime = QDateTime::currentDateTime();
    QLabel* creationTimeLabel = new QLabel(tr("Creation Time ") + "(" + localDateTime.timeZoneAbbreviation() + "):");
    creationTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    creationTimeEdit->setDisplayFormat("yyyy.MM.dd hh:mm:ss");
    creationTimeEdit->setCalendarPopup(true);
    calendarWidget = new QCalendarWidget(this);
    creationTimeEdit->setCalendarWidget(calendarWidget);

    QHBoxLayout* creationTimeLayout = new QHBoxLayout();
    creationTimeLayout->setSizeConstraint(QLayout::SetNoConstraint);
    creationTimeLayout->addWidget(creationTimeLabel);
    creationTimeLayout->addWidget(creationTimeEdit);

    // Main Layout 
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
    mainLayout->addLayout(nameLayout);
    mainLayout->addWidget(selectionLabel);
    mainLayout->addWidget(keychainListWidget);
    mainLayout->addLayout(keychainLayout);
    mainLayout->addLayout(minSigLayout);
    mainLayout->addLayout(creationTimeLayout);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);
}

NewAccountDialog::~NewAccountDialog()
{
    delete calendarWidget;
}

QString NewAccountDialog::getName() const
{
    return nameEdit->text();
}

int NewAccountDialog::getMinSigs() const
{
    return minSigComboBox->currentIndex() + 1;
}

qint64 NewAccountDialog::getCreationTime() const
{
    return creationTimeEdit->dateTime().toMSecsSinceEpoch();
}

void NewAccountDialog::updateSelection(const QString& keychain, int state)
{
    if (state == Qt::Checked)
    {
        keychainSet.insert(keychain);
    }
    else
    {
        keychainSet.remove(keychain);
    }

    updateMinSigs();
    updateEnabled();
}

void NewAccountDialog::updateMinSigs()
{
}

void NewAccountDialog::updateEnabled()
{
}

