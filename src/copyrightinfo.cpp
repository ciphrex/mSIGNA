///////////////////////////////////////////////////////////////////
//
// CoinVault
//
// copyrightinfo.cpp
//
// Copyright (c) 2013-2014 Eric Lombrozo
//
// All Rights Reserved.

#include "copyrightinfo.h"

// Definitions
const int COPYRIGHTPADDINGRIGHT = 20;
const int COPYRIGHTPADDINGBOTTOM = 10;

const QString COPYRIGHTTEXT("Copyright (c) 2013-2014 Ciphrex Corporation, All Rights Reserved");

// Accessors
int getCopyrightPaddingRight() { return COPYRIGHTPADDINGRIGHT; }
int getCopyrightPaddingBottom() { return COPYRIGHTPADDINGBOTTOM; }

const QString& getCopyrightText() { return COPYRIGHTTEXT; }

