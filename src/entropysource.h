///////////////////////////////////////////////////////////////////////////////
//
// CoinVault
//
// entropysource.h
//
// Copyright (c) 2014 Eric Lombrozo
//
// All Rights Reserved.

#pragma once

#include <CoinCore/typedefs.h>

class QWidget;

void seedEntropySource(bool reseed = false, bool showDialog = false, QWidget* parent = nullptr);
secure_bytes_t getRandomBytes(int n, QWidget* parent = nullptr);
void joinEntropyThread();

