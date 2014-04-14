///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// paymentrequest.cpp
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#include "paymentrequest.h"
#include "numberformats.h"

#include <CoinCore/Base58Check.h>

#include <CoinQ/CoinQ_typedefs.h>

#include <QUrl>
#include <QUrlQuery>
#include <QStringList>
#include <QObject>

// TODO: Allow different coin parameters
const QString VALID_URL_SCHEME("bitcoin");
const unsigned char VALID_ADDRESS_VERSIONS[] = { 0x00, 0x05 };
const int VALID_VERSION_COUNT = sizeof(VALID_ADDRESS_VERSIONS)/sizeof(unsigned char);
const char* BASE58_CHARS = BITCOIN_BASE58_CHARS;
const unsigned int ADDRESS_DATA_SIZE = 20;

PaymentRequest::PaymentRequest(const QUrl& url)
{
    if (url.scheme().compare(VALID_URL_SCHEME, Qt::CaseInsensitive) != 0) {
        throw std::runtime_error(QObject::tr("Invalid payment request URL scheme").toStdString());
    }

    QString path = url.path();
    address_ = path.section(';', 0, 0);
    hasAddress_ = !address_.isEmpty();
    if (!hasAddress_) {
        throw std::runtime_error(QObject::tr("Payment address is missing").toStdString());
    }

    QStringList versionParts = path.section(';', 1).split('=');
    hasVersion_ = versionParts.count() != 1;
    if (hasVersion_) {
        if (versionParts.count() != 2 || versionParts[0] != "version") {
            throw std::runtime_error(QObject::tr("Payment version is invalid").toStdString());
        }
        version_ = versionParts[1];
    }

    bytes_t payload;
    unsigned int addressVersion;
    if (!fromBase58Check(address_.toStdString(), payload, addressVersion, BASE58_CHARS) || payload.size() != ADDRESS_DATA_SIZE ||
        [&]() { for (int i = 0; i < VALID_VERSION_COUNT; i++) { if (addressVersion == VALID_ADDRESS_VERSIONS[i]) return false; } return true; }()) {

        throw std::runtime_error(QObject::tr("Invalid payment address").toStdString());
    }

    QUrlQuery query(url);

    hasValue_ = query.hasQueryItem("amount");
    if (hasValue_) value_ = parseAmount(query.queryItemValue("amount"));

    hasLabel_ = query.hasQueryItem("label");
    if (hasLabel_) label_ = query.queryItemValue("label");

    hasMessage_ = query.hasQueryItem("message");
    if (hasMessage_) message_ = query.queryItemValue("message");

    hasSend_ = query.hasQueryItem("send");
    if (hasSend_) send_ = query.queryItemValue("send");
}

uint64_t PaymentRequest::parseAmount(const QString& amountString)
{
    // TODO: handle small amounts correctly
    QString mantissa;
    int exponent;
    if (amountString.contains('X')) {
        QString exp = amountString.section('X', 1);
        if (exp.size() != 1 || exp[0] < '0' || exp[0] > '8') {
            throw std::runtime_error(QObject::tr("Invalid payment address").toStdString());
        }
        mantissa = amountString.section('X', 0, 0);
        exponent = exp[0].unicode() - '0';
    }
    else {
        mantissa = amountString;
        exponent = 8;
    }
    uint64_t value = btcStringToSatoshis(mantissa.toStdString());
    for (int i = 8; i > exponent; i--) { value /= 10; }
    return value;
}

QString PaymentRequest::toJson() const
{
    bool first = true;
    QString json("{");
    if (hasAddress_) {
        first = false;
        json += "\"address\":\"" + address_ + "\"";
    }
    if (hasVersion_) {
        if (first)  { first = false; }
        else        { json += ",";   }
        json += "\"version\":\"" + version_ + "\"";
    }
    if (hasValue_) {
        if (first)  { first = false; }
        else        { json += ",";   }
        json += "\"value\":\"" + QString::number(value_) + "\"";
    }
    if (hasLabel_) {
        if (first)  { first = false; }
        else        { json += ",";   }
        json += "\"label\":\"" + label_ + "\"";
    }
    if (hasMessage_) {
        if (first)  { first = false; }
        else        { json += ",";   }
        json += "\"message\":\"" + message_ + "\"";
    }
    if (hasSend_) {
        if (first)  { first = false; }
        else        { json += ",";   }
        json += "\"send\":\"" + send_ + "\"";
    }
    json += "}";
    return json;
}

