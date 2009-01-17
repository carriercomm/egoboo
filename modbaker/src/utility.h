//---------------------------------------------------------------------
// ModBaker - a module editor for Egoboo
//---------------------------------------------------------------------
// Copyright (C) 2009 Tobias Gall
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//---------------------------------------------------------------------
#ifndef utility_h
#define utility_h
//---------------------------------------------------------------------
//-
//-   Utility functions for filesystem access
//-
//---------------------------------------------------------------------

#include "global.h"

#include <iostream>
#include <fstream>
using namespace std;

void fgoto_colon(ifstream &);
bool fgoto_colon_yesno(ifstream &);
void fgoto_colon_buffer(ifstream &, string, size_t );
bool fgoto_colon_yesno_buffer(ifstream &, string, size_t  );


int fget_int(ifstream &);
int fget_next_int(ifstream &);

float fget_float(ifstream &);
float fget_next_float(ifstream &);
#endif
