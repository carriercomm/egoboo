/*******************************************************************************
*  EDITDRAW.H                                                                  *
*	- Draws anithing what's 3D        	                                       *
*									                                           *
*   Copyright (C) 2010  Paul Mueller <pmtech@swissonline.ch>                   *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

#ifndef _EDITDRAW_H_
#define _EDITDRAW_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "editor.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITDRAW_OBJ_TILE   0x01    /* 	==> First always the tile       */
#define EDITDRAW_OBJ_CHAR   0x02    /* Characters (MD2)                 */
#define EDITDRAW_OBJ_PART   0x03    /* Particles (can be transparent)   */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

COMMAND_T *editdrawInitData(void);
void editdrawFreeData(void);
void editdrawChooseFanType(int type_no, int x, int y);
void editdraw3DView(MESH_T *mesh, int chosen_fan, int chosen_fan_type);
void editdraw2DMap(MESH_T *mesh, int x, int y, int w, int h, int chosen_fan);
void editdraw2DTex(int x, int y, int w, int h, unsigned char tx_no, char tx_big); 

#endif  /* _EDITDRAW_H_ */