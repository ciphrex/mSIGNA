///////////////////////////////////////////////////////////////////////////////
//
// mSIGNA
//
// entropysource.h
//
// Copyright (c) 2014 Eric Lombrozo
// Copyright (c) 2011-2016 Ciphrex Corp.
//
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
//

#pragma once

#include <CoinCore/typedefs.h>

class QWidget;

void seedEntropySource(bool reseed = false, bool showDialog = false, QWidget* parent = nullptr);
secure_bytes_t getRandomBytes(int n, QWidget* parent = nullptr);
void joinEntropyThread();

