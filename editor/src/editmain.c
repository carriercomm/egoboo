/*******************************************************************************
*  EDITMAIN.C                                                                  *
*	- Main edit functions for map and other things      	                   *
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

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <stdio.h>
#include <memory.h>
#include <math.h>

#include "editor.h"             /* Global needed definitions    */
#include "editfile.h"           /* Load and save map files      */
#include "editdraw.h"           /* Draw anything                */


#include "editmain.h"           /* My own header                */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_MAX_COMMAND 30

#define EDITMAIN_MAX_MAPSIZE    64
#define EDITMAIN_DEFAULT_TILE   ((char)54)
#define EDITMAIN_TOP_TILE       ((char)63)  /* Black texture    */
#define EDITMAIN_WALL_HEIGHT    192
#define EDITMAIN_TILEDIV        128         /* Size of tile     */

/* --------- Info for preset tiles ------- */
#define EDITMAIN_PRESET_FLOOR   0
#define EDITMAIN_PRESET_TOP     1
#define EDITMAIN_PRESET_WALL    2
#define EDITMAIN_PRESET_EDGEO   3
#define EDITMAIN_PRESET_EDGEI   4
#define EDITMAIN_PRESET_MAX     5

#define EDITMAIN_PRESET_TFLOOR  0
#define EDITMAIN_PRESET_TTOP    1
#define EDITMAIN_PRESET_TWALL   8
#define EDITMAIN_PRESET_TEDGEO 16
#define EDITMAIN_PRESET_TEDGEI 19

#define EDITMAIN_NORTH  0x00
#define EDITMAIN_EAST   0x01
#define EDITMAIN_SOUTH  0x02
#define EDITMAIN_WEST   0x03

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int x, y;

} EDITMAIN_XY;

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

static MESH_T Mesh;
static COMMAND_T *pCommands;
static EDITMAIN_STATE_T EditState;
static SPAWN_OBJECT_T   SpawnObjects[EDITMAIN_MAXSPAWN + 2];
static EDITOR_PASSAGE_T Passages[EDITMAIN_MAXPASSAGE + 2];

/* --- Definition of preset tiels for 'simple' mode -- */
static FANDATA_T PresetTiles[] = {
    {  EDITMAIN_DEFAULT_TILE, 0, 0,  0 },                          /* Floor    */
    {  EDITMAIN_TOP_TILE,     0, (MPDFX_WALL | MPDFX_IMPASS), 1 }, /* Top      */
    /* Walls, x/y values are rotated, if needed */
    {  64 + 10, 0, (MPDFX_WALL | MPDFX_IMPASS), 8  },   /* Wall north */
    {  64 + 1,  0, (MPDFX_WALL | MPDFX_IMPASS), 16 },   /* Outer edge north/east */
    {  64 + 3,  0, (MPDFX_WALL | MPDFX_IMPASS), 19 },   /* Inner edge north/west */
    { 0 }
};

/* ------ Data for checking of adjacent tiles -------- */
static EDITMAIN_XY AdjacentXY[8] = {

    {  0, -1 }, { +1, -1 }, { +1,  0 }, { +1, +1 },
    {  0, +1 }, { -1, +1 }, { -1,  0 }, { -1, -1 }

};

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editmainGetAdjacent
 * Description:
 *     Creates a list of fans which are adjacent to given 'fan'. 
 *     If a fan is of map, the field is filled by a value of -1.
 *     The list starts from North', clockwise.  
 * Input:
 *     mesh*:      Pointer on mesh with info about map size
 *     adjacent *: Where to return the list of fan-positions 
 * Output: 
 *     Number of adjacent tiles 
 */
static int editmainGetAdjacent(MESH_T *mesh, int fan, int adjacent[8])
{

    int dir, adj_pos;
    int num_adj;
    EDITMAIN_XY src_xy, dest_xy;
    
    
    num_adj = 0;            /* Count them */
    for (dir = 0; dir < 8; dir++) {
    
        adj_pos = -1;       /* Assume invalid */

        src_xy.x = fan % mesh -> tiles_x;
        src_xy.y = fan / mesh -> tiles_x;
        
        dest_xy.x = src_xy.x + AdjacentXY[dir].x;
        dest_xy.y = src_xy.y + AdjacentXY[dir].y;
        
        if ((dest_xy.x >= 0) && (dest_xy.x < mesh -> tiles_x)
            && (dest_xy.y >= 0) && (dest_xy.y < mesh -> tiles_y)) {

            adj_pos = (dest_xy.y * mesh -> tiles_x) + dest_xy.x;
            num_adj++;

        }

        adjacent[dir] = adj_pos;       /* Starting north */

    }

    return num_adj;

}

/*
 * Name:
 *     editmainFanAdd
 * Description:
 *     This functionsa adds a new fan to the actual map, if there are enough
 *     vertices left for it
 * Input:
 *     mesh*: Pointer on mesh to handle
 *     fan:   Number of fan to allocate vertices for
 *     x, y:  Position of fan
 *     zadd:  Elevation to add to given default z-value
 * Output:
 *    Fan could be added, yes/no
 */
static int editmainFanAdd(MESH_T *mesh, int fan, int x, int y, int zadd)
{

    COMMAND_T *ft;
    int cnt;
    int vertex;


    ft = &pCommands[mesh -> fan[fan].type];

    if (mesh -> numfreevert >= ft -> numvertices)
    {

        vertex = mesh -> numvert;

        mesh -> vrtstart[fan] = vertex;

        for (cnt = 0; cnt < ft -> numvertices; cnt++) {

            mesh -> vrtx[vertex] = x + ft -> vtx[cnt].x;
            mesh -> vrty[vertex] = y + ft -> vtx[cnt].y;
            mesh -> vrtz[vertex] = zadd + ft -> vtx[cnt].z;
            vertex++;

        }

        mesh -> numvert     += ft -> numvertices;   /* Actual number of vertices used   */
        mesh -> numfreevert -= ft -> numvertices;   /* Vertices left for edit           */

        return 1;

    }

    return 0;
}

/*
 * Name:
 *     editmainDoFanUpdate
 * Description:
 *     This function updates the fan at given 'fan 'with the
 *     info in 'new_ft'.
 *     If the type has changed, the vertice data is updated.
 * Input:
 *     mesh *:    Pointer on mesh to handle
 *     fan:       Number of fan to allocate vertices for
 *     x, y:      Position of fan
 *     new_fan *: Pointer on description of new fan data
 *     new_ft *:  Pointer on dat to create new fan off
 * Output:
 *    Fan could be updated, yes/no
 */
static int editmainDoFanUpdate(MESH_T *mesh, int fan, int x, int y, FANDATA_T *new_fan,
                               COMMAND_T *new_ft)
{

    COMMAND_T *act_ft;
    FANDATA_T *act_fan;
    int cnt;
    int vrt_diff, vertex;


    act_fan = &mesh -> fan[fan];

    act_ft  = &pCommands[act_fan -> type];

    /* Do an update on the 'static' data of the fan */
    act_fan -> tx_no    = new_fan -> tx_no;
    act_fan -> tx_flags = new_fan -> tx_flags;
    act_fan -> fx       = new_fan -> fx;

    if (act_fan -> type == new_fan -> type) {

        return 1;       /* No vertices to change */

    }

    vrt_diff = new_ft -> numvertices - act_ft -> numvertices;

    if (0 == vrt_diff) {
        /* Same number of vertices, only overwrite is needed */
        /* Set the new type of fan */
        act_fan -> type = new_fan -> type;

    }
    else {
        /* Number of vertices has changed */
        /* Solve this by handling a it with insert/delete as in a string */
        /* 1. Get number of vertices of existing fan    */
        /* 1. Remove old fan (count it's vertices)      */
        /* 2. Replace it by fan with new type  ?        */
        /* Set new fan starts for all fans from here, if number of vertices has
       changed */
       return 0;
    }

    /* Fill in the vertex values from type definition */
    vertex = mesh -> vrtstart[fan];
    for (cnt = 0; cnt < new_ft -> numvertices; cnt++) {
        /* Replace actual values by new values */
        mesh -> vrtx[vertex] = x + new_ft -> vtx[cnt].x;
        mesh -> vrty[vertex] = y + new_ft -> vtx[cnt].y;
        mesh -> vrtz[vertex] = new_ft -> vtx[cnt].z;
        vertex++;
    }

    return 1;

}

/*
 * Name:
 *     editmainFanTypeRotate
 * Description:
 *     For 'simple' mode. Rotates the given fan type
 *     Adjusts the adjacent tiles accordingly, using the 'default' fan set.
 * Input:
 *     src *:  Fan type to rotate
 *     dest *: Rotated result
 *     dir:    Into which direction to rotate
 */
static void editmainFanTypeRotate(COMMAND_T *src, COMMAND_T *dest, char dir)
{

    int rottable[8] = {  0, 1, 1, 0, 0, -1, -1, 0, };

    int cnt;


    /* Get copy of chosen type */
    memcpy(dest, src, sizeof(COMMAND_T));

    if (dir != EDITMAIN_NORTH) {

        for (cnt = 0; dest -> numvertices; cnt++) {

            /* First translate to have rotation center in middle of fan square */
            dest -> vtx[cnt].x -= 64.0;
            dest -> vtx[cnt].y -= 64.0;
            /* And now rotate it */
            dest -> vtx[cnt].x *= rottable[dir * 2];
            dest -> vtx[cnt].y *= rottable[(dir * 2) + 1];
            /* And move it back to start position */
            dest -> vtx[cnt].x += 64.0;
            dest -> vtx[cnt].y += 64.0;

        }
    
    }      
    
    /* Otherwise no rotation is needed */
}

/*
 * Name:
 *     editmainSetFanSimple
 * Description:
 *     For 'simple' mode. Sets a tile, if possible .
 *     Adjusts the adjacent tiles accordingly, using the 'default' fan set. 
 * Input:
 *     mesh *:   Pointer on mesh to handle 
 *     fan_no:   Where to place the tile on map
 *     x, y:     Position on map
 *     is_floor: Is it a floor yes/no
 * Output:
 *     Success yes/no
 */
static int editmainSetFanSimple(MESH_T *mesh, int fan_no, int x, int y, char is_floor)
{

    static int adj_xy[] = { +0, -128, +128, -128, +128, +0, +128, +128,
                            +0, +128, -128, +128, -128 + 0, -128, -128 };
    COMMAND_T fan_type_rot;
    char t1, t2, t3, rotdir;
    int adjacent[8];
    int num_adj, dir, dir2, adjfan_no;


    num_adj = editmainGetAdjacent(&Mesh, fan_no, adjacent);
    if (num_adj < 8) {
        /* There moust be at least one tile left to edge of map */
        return 0;
    }


    if (is_floor) {
        if (mesh -> fan[fan_no].type == EDITMAIN_PRESET_FLOOR) {
            /* No change at all */
            return 1;
            
        }
        /* Change me to floor */
        editmainDoFanUpdate(mesh, fan_no, x, y,
                            &PresetTiles[EDITMAIN_PRESET_FLOOR],
                            &pCommands[EDITMAIN_PRESET_TFLOOR]);

        for (dir = 0; dir < 8; dir += 2) {
            dir2 = ((dir + 2) & 0x07);
            rotdir = ((dir >> 1)^0x03);
            
            t1 = mesh -> fan[adjacent[dir]].type;
            t2 = mesh -> fan[adjacent[(dir + 2) & 0x07]].type;

            if (t1 == EDITMAIN_PRESET_TTOP) {
                editmainFanTypeRotate(&pCommands[EDITMAIN_PRESET_TTOP], &fan_type_rot, rotdir);
                editmainDoFanUpdate(mesh, adjacent[dir],
                                    x + adj_xy[(dir * 2)], 
                                    y + adj_xy[(dir * 2) + 1],
                                    &PresetTiles[EDITMAIN_PRESET_TOP],
                                    &fan_type_rot);
            }
            if (t2 == EDITMAIN_PRESET_TTOP) {
                editmainFanTypeRotate(&pCommands[EDITMAIN_PRESET_TWALL], &fan_type_rot, rotdir);
                editmainDoFanUpdate(mesh, adjacent[(dir + 2) & 0x07], 
                                    x + adj_xy[(((dir + 2) & 0x07) * 2)], 
                                    y + adj_xy[(((dir + 2) & 0x07) * 2) + 1],
                                    &PresetTiles[EDITMAIN_PRESET_WALL],
                                    &fan_type_rot);
            }
            if (t1 == EDITMAIN_PRESET_TTOP && t2 == EDITMAIN_PRESET_TTOP) {
                editmainFanTypeRotate(&pCommands[EDITMAIN_PRESET_TEDGEI], &fan_type_rot, rotdir);
                editmainDoFanUpdate(mesh, adjacent[dir + 1], 
                                    x + adj_xy[((dir + 1) * 2)], 
                                    y + adj_xy[((dir + 1) * 2) + 1],
                                    &PresetTiles[EDITMAIN_PRESET_EDGEI],
                                    &fan_type_rot);
            }
        }
    }
    else {
         if (mesh -> fan[fan_no].type == EDITMAIN_PRESET_TTOP) {
            /* No change at all */
            return 1;
            
        }
        /* Change me to solid */
        /* My shape depens on the surrounding tiles */
        /*
        editmainDoFanUpdate(mesh, fan_no, x, y, &PresetTiles[EDITMAIN_PRESET_TOP]);
        */

        for (dir = 0; dir < 8; dir += 2) {
            /* TODO: Adjust walls surrounding this fan, if needed */
            dir2 = ((dir + 2) & 0x07);
            rotdir = (dir >> 1)^0x03;
            
            t1 = mesh -> fan[adjacent[dir]].type;
            t2 = mesh -> fan[adjacent[dir + 1]].type;
            t3 = mesh -> fan[adjacent[dir2]].type;          

            if (t1 == EDITMAIN_PRESET_TFLOOR) {
                if (t3 == EDITMAIN_PRESET_TFLOOR) {
                    /*
                    editmainFanTypeRotate(&PresetTiles[EDITMAIN_PRESET_EDGEO], &fan_type_rot, rotdir);
                    editmainDoFanUpdate(mesh, fan_no, x, y, &fan_type_rot);
                    */
                }
                else if (t3 == EDITMAIN_PRESET_TWALL) {
                    /* TODO: Adjust walls in direction */
                }
            }
            if (t3 == EDITMAIN_PRESET_TFLOOR) {
                /* TODO: Rotate wall to correct direction */
                /*
                editmainDoFanUpdate(mesh, fan_no, x, y, &PresetTiles[EDITMAIN_PRESET_WALL]);
                */
            }
            if (t1 == EDITMAIN_PRESET_TFLOOR && t3 == EDITMAIN_PRESET_TFLOOR) {
                /* TODO: Rotate wall to correct direction */
                /*
                editmainDoFanUpdate(mesh, fan_no, x, y, &PresetTiles[EDITMAIN_PRESET_EDGEO]);
                */
            }
        }

    }

    return 0;

}

/*
 * Name:
 *     editmainCompleteMapData
 * Description:
 *     Completes loaded / generated map data with additional values needed
 *     for work.
 *       - Set number of fans
 *       - Set size of complete mesh
 *       - Set number of first vertex for fans
 * Input:
 *     mesh*: Pointer on mesh to handle
 * Output:
 *    None
 */
void editmainCompleteMapData(MESH_T *mesh)
{

    int fan_no, vertex_no;


    mesh -> numfan  = mesh -> tiles_x * mesh -> tiles_y;

    mesh -> edgex = (mesh -> tiles_x * EDITMAIN_TILEDIV) - 1;
    mesh -> edgey = (mesh -> tiles_y * EDITMAIN_TILEDIV) - 1;
    mesh -> edgez = 180 * 16;
    
    mesh -> watershift = 3;
    if (mesh -> tiles_x > 16)   mesh -> watershift++;
    if (mesh -> tiles_x > 32)   mesh -> watershift++;
    if (mesh -> tiles_x > 64)   mesh -> watershift++;
    if (mesh -> tiles_x > 128)  mesh -> watershift++;
    if (mesh -> tiles_x > 256)  mesh -> watershift++;

    /* Now set the number of first vertex for each fan */
    for (fan_no = 0, vertex_no = 0; fan_no < mesh -> numfan; fan_no++) {

        mesh -> vrtstart[fan_no] = vertex_no;		/* meshvrtstart       */
        mesh -> visible[fan_no]  = 1;
        vertex_no += pCommands[mesh -> fan[fan_no].type & 0x1F].numvertices;

    }

    mesh -> numfreevert = MAXTOTALMESHVERTICES - mesh -> numvert;
    /* Set flag that map has been loaded */
    mesh -> map_loaded = 1;
    mesh -> draw_mode  = EditState.draw_mode;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 *     which:  Type of map to create : EDITMAIN_NEWFLATMAP / EDITMAIN_NEWSOLIDMAP
 */
static int editmainCreateNewMap(MESH_T *mesh, int which)
{

    int x, y, fan, zadd;
    char fan_fx;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x     = EDITMAIN_MAX_MAPSIZE;
    mesh -> tiles_y     = EDITMAIN_MAX_MAPSIZE;
    mesh -> numvert     = 0;                        /* Vertices used in map    */
    mesh -> numfreevert = MAXTOTALMESHVERTICES;     /* Vertices left in memory */

    zadd   = 0;
    fan_fx = 0;

    if (which == EDITMAIN_NEWSOLIDMAP) {
        zadd   = 192;                           /* All walls resp. top tiles  */
        fan_fx = (MPDFX_WALL | MPDFX_IMPASS);   /* All impassable walls       */
    }

    fan = 0;
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            /* TODO: Generate empty map */
            mesh -> fan[fan].type  = 0;
            if (which == EDITMAIN_NEWSOLIDMAP) {
                mesh -> fan[fan].tx_no = (unsigned char)((((x & 1) + (y & 1)) & 1) + EDITMAIN_DEFAULT_TILE);
            }
            else {
                mesh -> fan[fan].tx_no = EDITMAIN_TOP_TILE;
            }
            mesh -> fan[fan].fx    = fan_fx;

            if (! editmainFanAdd(mesh, fan, x*EDITMAIN_TILEDIV, y*EDITMAIN_TILEDIV, zadd))
            {
                sprintf(EditState.msg, "%s", "NOT ENOUGH VERTICES!!!");
                return 0;
            }

            fan++;

        }

    }

    return 1;   /* TODO: Return 1 if mesh is created */

}

/*
 * Name:
 *     editfileSetVrta
 * Description:
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set 
 */
static int editfileSetVrta(MESH_T *mesh, int vert)
{
    /* TODO: Get all needed functions from cartman code */
    /*
    int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
    int newlevel, distance, disx, disy;

    // To make life easier
    x = mesh -> vrtx[vert];
    y = mesh -> vrty[vert];
    z = mesh -> vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = get_level(brx, y) +
          get_level(x, bry) +
          get_level(x + 46, y + 46);
    if (z < -128) z = -128;
    if (brz < -128) brz = -128;
    deltaz = z + z + z - brz;
    newa = (deltaz * direct >> 8);

    // Point lights !!!BAD!!!
    newlevel = 0;
    cnt = 0;
    while (cnt < numlight)
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(disx * disx + disy * disy);
        if (distance < light_lst[cnt].radius)
        {
            newlevel += ((light_lst[cnt].level * (light_lst[cnt].radius - distance)) / light_lst[cnt].radius);
        }
        cnt++;
    }
    newa += newlevel;

    // Bounds
    if (newa < -ambicut) newa = -ambicut;
    newa += ambi;
    if (newa <= 0) newa = 1;
    if (newa > 255) newa = 255;
    mesh -> vrta[vert] = newa;

    // Edge fade
    dist = dist_from_border(mesh  -> vrtx[vert], mesh -> vrty[vert]);
    if (dist <= FADEBORDER)
    {
        newa = newa * dist / FADEBORDER;
        if (newa == VERTEXUNUSED)  newa = 1;
        mesh  ->  vrta[vert] = newa;
    }

    return newa;
    */
    return 60;
}

/*
 * Name:
 *     editmainCalcVrta
 * Description:
 *     Generates the 'a'  numbers for all files
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static void editmainCalcVrta(MESH_T *mesh)
{
    int num, cnt;
    int vert;
    int fan;


    for (fan = 0; fan < mesh -> numfan; fan++) {

        vert = mesh -> vrtstart[fan];
        num  = pCommands[mesh -> fan[fan].type].numvertices;
        
        for (cnt = 0; cnt < num; cnt++) {
        
            editfileSetVrta(mesh, vert);
            vert++;
            
        }

    }
   
}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     editmainInit
 * Description:
 *     Does all initalizations for the editor
 * Input:
 *     None
 * Output:
 *     Pointer on EditState
 */
EDITMAIN_STATE_T *editmainInit(void)
{

    pCommands = editdrawInitData();
     
    memset(&EditState, 0, sizeof(EDITMAIN_STATE_T));

    EditState.display_flags |= EDITMAIN_SHOW2DMAP;
    EditState.fan_chosen    = -1;   /* No fan chosen                        */
    EditState.new_fan.type  = -1;   /* No fan-type chosen                   */    
    EditState.brush_size    = 3;    /* Size of raise/lower terrain brush    */
    EditState.brush_amount  = 50;   /* Amount of raise/lower                */

    EditState.draw_mode     = (EDIT_MODE_SOLID | EDIT_MODE_TEXTURED | EDIT_MODE_LIGHTMAX);

    return &EditState;
    
}

/*
 * Name:
 *     editmainExit
 * Description:
 *     Frees all data set an initialized by init code
 * Input:
 *     None
 */
void editmainExit(void)
{

    /* Free all data initialized by draw code */
    editdrawFreeData();

}

/*
 * Name:
 *     editmainMap
 * Description:
 *      Does the work for editing and sets edit states, if needed 
 * Input:
 *      command: What to do
 * Output:
 *      Result of given command
 */
int editmainMap(int command)
{

    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh, EditState.fan_chosen, EditState.new_fan.type);
            return 1;

        case EDITMAIN_NEWFLATMAP:
        case EDITMAIN_NEWSOLIDMAP:
            if (editmainCreateNewMap(&Mesh, command)) {

                editmainCompleteMapData(&Mesh);
                return 1;

            }
            break;

        case EDITMAIN_LOADMAP:
            memset(&Mesh, 0, sizeof(MESH_T));
            if (editfileLoadMapMesh(&Mesh, EditState.msg)) {

                editmainCompleteMapData(&Mesh);

                return 1;

            }
            break;

        case EDITMAIN_SAVEMAP:
            editmainCalcVrta(&Mesh);
            return editfileSaveMapMesh(&Mesh, EditState.msg);

    }

    return 0;

}

/*
 * Name:
 *     editmainDrawMap2D
 * Description:
 *      Draws the map as 2D-Map into given rectangle
 * Input:
 *      command: What to do
 * Output:
 *      Result of given command
 */
void editmainDrawMap2D(int x, int y, int w, int h)
{ 
    
    editdraw2DMap(&Mesh, x, y, w, h, EditState.fan_chosen);

}

/*
 * Name:
 *     editmainLoadSpawn
 * Description:
 *     Load the 'spawn.txt' list into given list
 * Input:
 *     None
 * Output:
 *     spawn_list *: Pointer on list of objects to be spawned
 */
SPAWN_OBJECT_T *editmainLoadSpawn(void)
{

    memset(SpawnObjects, 0, EDITMAIN_MAXSPAWN * sizeof(SPAWN_OBJECT_T));

    /* TODO: Load this list, if available */

    return SpawnObjects;

}

/*
 * Name:
 *     editmainToggleFlag
 * Description:
 *     Toggles the given flag using 'EditInfo'.
 *     Adjust accompanied
 * Input:
 *     edit_state *: To return the toggle states to caller
 *     which:        Which flag to change
 *
 */
void editmainToggleFlag(int which, unsigned char flag)
{

    switch(which) {            

        case EDITMAIN_TOGGLE_DRAWMODE:
            /* Change it in actual map */
            Mesh.draw_mode ^= flag;
            /* Show it in edit_state */
            EditState.draw_mode = Mesh.draw_mode;
            break;

        case EDITMAIN_TOGGLE_FX:
            if (EditState.fan_chosen >= 0 && EditState.fan_chosen < Mesh.numfan){
                /* Toggle it in chosen fan */
                Mesh.fan[EditState.fan_chosen].fx ^= flag;
                /* Now copy the actual state for display    */
                EditState.act_fan.fx = Mesh.fan[EditState.fan_chosen].fx;
            }
            else {
                EditState.act_fan.fx = 0;
            }
            break;

        case EDITMAIN_TOGGLE_TXHILO:
            if (EditState.fan_chosen >= 0) {
                Mesh.fan[EditState.fan_chosen].tx_flags ^= flag;
                /* And copy it for display */
                EditState.act_fan.tx_flags = Mesh.fan[EditState.fan_chosen].tx_flags;
            }
            break;

    }

}

/*
 * Name:
 *     editmainChooseFan
 * Description:
 *     Choose a fan from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'
 * Input:
 *     cx, cy: Position chosen
 *     w, h:   Extent of rectangle
 */
void editmainChooseFan(int cx, int cy, int w, int h)
{

    int fan_no;
    

    /* Save it as x/y-position, too */
    EditState.tile_x = Mesh.tiles_x * cx / w;
    EditState.tile_y = Mesh.tiles_y * cy / h;
    
    fan_no = (EditState.tile_y * Mesh.tiles_x) + EditState.tile_x;
    
    if (fan_no >= 0 && fan_no < Mesh.numfan) {
    
        EditState.fan_chosen = fan_no;
        /* And fill it into 'EditState' for display */
        memcpy(&EditState.act_fan, &Mesh.fan[fan_no], sizeof(FANDATA_T));
            
    }
 
}

/*
 * Name:
 *     editmainFanTypeName
 * Description:
 *     Returns the description of given fan-type 
 * Input:
 *     type_no: Number of fan type
 * Output:
 *     Pointer on a valid string, may be empty 
 */
char *editmainFanTypeName(int type_no)
{

    if (type_no >= 0 && type_no < MAXMESHTYPE) {

        if (pCommands[type_no & 0x1F].name != 0) {
        
            return pCommands[type_no & 0x1F].name;
        
        }
    
    }
    return "";

}

/*
 * Name:
 *     editmainChooseFanType
 * Description:
 *     Chooses a fan type and sets it for display.
 *     TODO: Ignore 'empty' fan types
 * Input:
 *     type_no:    Number of fan type  (-2: Switch off, 0: Switch on)
 *     direction:  != 0: Browsing direction trough list...
 *     fan_name *: Where to print hte name of chosen fan
 */
void editmainChooseFanType(int dir, char *fan_name)
{

    if (dir == -2) {
        /* Reset browsing */
        EditState.new_fan.type = -1;
        fan_name[0] = 0;
        return;
    }
    else if (dir == 0) {
        /* Start browsing */
        EditState.new_fan.type = 0;
    }
    else if (dir == -1) {
        if (EditState.new_fan.type > 0) {
            EditState.new_fan.type--;
        }
    }
    else {
        if (EditState.new_fan.type < 30) {
            EditState.new_fan.type++;
        }
    }

    editdrawChooseFanType(EditState.new_fan.type, EditState.tile_x, EditState.tile_y);
    sprintf(fan_name, "%s", editmainFanTypeName(EditState.new_fan.type));

}

/*
 * Name:
 *     editmain2DTex
 * Description:
 *     Draws the texture and chosen texture-part of actual chosen fan
 *     into given rectangle.
 * Input:
 *     x, y, w, h: Rectangle to draw into
 *     ft *:       Pointer on fandata to use for drawing
 *     tx_no:      From fan
 */
void editmain2DTex(int x, int y, int w, int h, FANDATA_T *ft)
{

    if (EditState.fan_chosen >= 0) {

        editdraw2DTex(x, y, w, h, ft -> tx_no, ft -> type & COMMAND_TEXTUREHI_FLAG);
        
    }
    
}

/*
 * Name:
 *     editmainFanSet
 * Description:
 *     Sets a fan at the actual chosen position, depending on edit_state. 
 *     Does an update on given fan. Including changed number of vertices,
 *     if needed.
 * Input:
 *     edit_state: How to handle the command 
 *     new_fan * : New fan data to replace old ones with
 *     is_floor:   Set floor in simple mode       
 */
int editmainFanSet(char edit_state, FANDATA_T *new_fan, char is_floor)
{

    if (EditState.fan_chosen >= 0) {
        
        if (edit_state == EDITMAIN_EDIT_NONE) {
            /* Do nothing, is view-mode */
            return 1;
        }
        else if (edit_state == EDITMAIN_EDIT_SIMPLE) {
 
            return editmainSetFanSimple(&Mesh, 
                                        EditState.fan_chosen,
                                        EditState.tile_x,
                                        EditState.tile_y,                
                                        is_floor);

        }
        else if (edit_state == EDITMAIN_EDIT_FULL) {
            /* Do 'simple' editing */
            /*
            return editmainDoFanUpdate(&Mesh,
                                       EditState.fan_chosen,
                                       EditState.tile_x,
                                       EditState.tile_y,
                                       new_fan);
            */
        }
    }

    return 0;
}