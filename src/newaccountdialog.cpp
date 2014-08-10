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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QDateTimeEdit>
#include <QCalendarWidget>

#include <stdexcept>

NewAccountDialog::NewAccountDialog(const QList<QString>& keychainNames, QWidget* parent)
    : QDialog(parent)
{
    if (keychainNames.isEmpty()) {
        throw std::runtime_error("Names list cannot be empty.");
    }

    this->keychainNames = keychainNames;
    qSort(this->keychainNames.begin(), this->keychainNames.end());

    // Buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);

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

    // Key Chain Names
    minSigComboBox = new QComboBox();
    minSigLineEdit = new QLineEdit();
    minSigLineEdit->setAlignment(Qt::AlignRight);
    minSigComboBox->setLineEdit(minSigLineEdit);

    QLabel* keychainLabel = new QLabel();
    keychainLabel->setText(tr("Key Chains:"));

    QString keychainText;
    for (int i = 0; i < keychainNames.size(); i++) {
        if (i != 0) { keychainText += ", "; }
        keychainText += keychainNames.at(i);
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

