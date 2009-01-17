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
#ifndef global_h
#define global_h
//---------------------------------------------------------------------
//-
//-   This file contains global defined stuff
//-
//---------------------------------------------------------------------
#include <vector>

using namespace std;

#include "edit.h"
#include "renderer.h"
#include "mesh.h"
#include "frustum.h"

// Mouse / input stuff
// Implemented in controls.cpp
extern int g_mouse_x;
extern int g_mouse_y;

extern float g_mouse_gl_x;
extern float g_mouse_gl_y;
extern float g_mouse_gl_z;

// Selection
// Implemented in edit.cpp
extern int g_nearest_vertex;
extern c_selection g_selection;

// Global subsystems
// Implmented in renderer.cpp, renderer_misc.cpp, mesh.cpp, frustum.cpp
extern c_renderer g_renderer;
extern c_mesh     g_mesh;
extern c_frustum  g_frustum;                // TODO: Move to g_renderer
extern vector<c_tile_definition> g_tiledict;

// Misc stuff
// Implmented in global.cpp
extern float calculate_distance(vect3, vect3); // TODO: Move to graphic stuff
extern void Quit();
#endif
