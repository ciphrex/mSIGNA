///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// paymentrequest.h
//
// Copyright (c) 2013 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <QString>

#include <stdint.h>

class QUrl;

class PaymentRequest
{
public:
    PaymentRequest() : value_(0), hasAddress_(false), hasVersion_(false), hasValue_(false), hasLabel_(false), hasMessage_(false), hasSend_(false) { }
    PaymentRequest(const QUrl& url);

    static uint64_t parseAmount(const QString& amountString);

    const QString& address() const { return address_; }
    const QString& version() const { return version_; }
    uint64_t value() const { return value_; }
    const QString& label() const { return label_; }
    const QString& message() const { return message_; }
    const QString& send() const { return send_; }

    bool hasAddress() const { return hasAddress_; }
    bool hasVersion() const { return hasVersion_; }
    bool hasValue() const { return hasValue_; }
    bool hasLabel() const { return hasLabel_; }
    bool hasMessage() const { return hasMessage_; }
    bool hasSend() const { return hasSend_; }

    QString toJson() const;

private:
    QString address_;
    QString version_;
    uint64_t value_;
    QString label_;
    QString message_;
    QString send_;

    bool hasAddress_;
    bool hasVersion_;
    bool hasValue_;
    bool hasLabel_;
    bool hasMessage_;
    bool hasSend_;
};

