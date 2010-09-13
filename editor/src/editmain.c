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

#include "sdlgl3d.h"            /* DEG2RAD                              */
#include "sdlglcfg.h"           /* Reading passages and spawn points    */
#include "editor.h"             /* Global needed definitions            */
#include "editfile.h"           /* Load and save map files              */
#include "editdraw.h"           /* Draw anything                        */
#include "wallmake.h"           /* Create walls/floors simple           */


#include "editmain.h"           /* My own header                */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_MAX_COMMAND 30

#define EDITMAIN_DEFAULT_TILE   ((char)1)
#define EDITMAIN_TOP_TILE       ((char)63)  /* Black texture    */
#define EDITMAIN_WALL_HEIGHT    192
#define EDITMAIN_TILEDIV        128         /* Size of tile     */

#define EDITMAIN_PRESET_MAX 4

#define EDITMAIN_NORTH  0x00
#define EDITMAIN_EAST   0x01
#define EDITMAIN_SOUTH  0x02
#define EDITMAIN_WEST   0x03

#define EDITMAIN_MAXSPAWN    500        /* Maximum Lines in spawn list  */
#define EDITMAIN_MAXPASSAGE   50

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int x, y;

} EDITMAIN_XY;

typedef struct {

    char line_name[25];         /* Only for information purposes */
    EDITMAIN_XY topleft;
    EDITMAIN_XY bottomright;
    char open;
    char shoot_trough;
    char slippy_close;
    
} EDITMAIN_PASSAGE_T;

typedef struct {

    char line_name[25];
    char item_name[20+1];
    int  slot_no;           /* Use it for coloring the bounding boxes */
    float x_pos, y_pos, z_pos;
    char view_dir;
    int  money;
    int  skin;
    int  pas;
    int  con;
    int  lvl;
    char stt;
    char gho;
    char team;
    
} EDITMAIN_SPAWNPT_T;     /* Spawn-Point for display on map. From 'spawn.txt' */

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

static MESH_T Mesh;
static COMMAND_T *pCommands;
static EDITMAIN_STATE_T EditState;

/* -------------Data for Spawn-Points ---------------- */
static EDITMAIN_SPAWNPT_T SpawnObjects[EDITMAIN_MAXSPAWN + 2];

static SDLGLCFG_VALUE SpawnVal[] = {
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].line_name, 24 },
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].item_name, 20 },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].slot_no },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].x_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].y_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].z_pos },
	{ SDLGLCFG_VAL_ONECHAR, &SpawnObjects[0].view_dir },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].money },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].skin },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].pas },
	{ 0 }
};

static SDLGLCFG_LINEINFO SpawnRec = {
	&SpawnObjects[0],
	EDITMAIN_MAXSPAWN,
	sizeof(EDITMAIN_SPAWNPT_T),
	&SpawnVal[0]
};

/* ------------ Data for passages -------------------- */
static EDITMAIN_PASSAGE_T Passages[EDITMAIN_MAXPASSAGE + 2];

static SDLGLCFG_VALUE PassageVal[] = {
	{ SDLGLCFG_VAL_STRING,  &Passages[0].line_name, 24 },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft.x },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft.y },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright.x },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright.y },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].open },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].shoot_trough },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].slippy_close },
	{ 0 }
};

static SDLGLCFG_LINEINFO PassageRec = {
	&Passages[0],
    EDITMAIN_MAXPASSAGE,
	sizeof(EDITMAIN_PASSAGE_T),
	&PassageVal[0]
};

/* --- Definition of preset tiles for 'simple' mode -- */
static FANDATA_T PresetTiles[] = {

    {  EDITMAIN_DEFAULT_TILE, 0, 0,  WALLMAKE_FLOOR },
    {  EDITMAIN_TOP_TILE,     0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_TOP },
    /* Walls, x/y values are rotated, if needed */
    {  63 + 33, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_WALL  },   /* Wall north            */
    {  63 + 51, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEO },   /* Outer edge north/east */
    {  63 +  1, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEI },   /* Inner edge north/west */
    { 0 }

};

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editmainSetFanStart
 * Description:
 *     Sets the 'vrtstart' for all fans in given map
 * Input:
 *     mesh*: Pointer on mesh to handle
 */
static void editmainSetFanStart(MESH_T *mesh)
{

    int fan_no, vertex_no;

    for (fan_no = 0, vertex_no = 0; fan_no < mesh -> numfan; fan_no++) {

        mesh -> vrtstart[fan_no] = vertex_no;	
        mesh -> visible[fan_no]  = 1;
        vertex_no += pCommands[mesh -> fan[fan_no].type & 0x1F].numvertices;

    }

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


    ft = &pCommands[mesh -> fan[fan].type & 0x1F];

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
 *     info in 'EditState.ft'.
 *     If the type has changed, the vertice data is updated.
 * Input:
 *     mesh *:       Pointer on mesh to handle
 *     edit_state *: Pointer on edit state, holding all data neede for work
 *     tx, ty:       Position of tile in units
 * Output:
 *    Fan could be updated, yes/no
 */
static int editmainDoFanUpdate(MESH_T *mesh, EDITMAIN_STATE_T *edit_state, int tx, int ty)
{

    COMMAND_T *act_fd;
    FANDATA_T *act_ft;
    int cnt;
    int vrt_diff, vrt_size;
    int src_vtx, dst_vtx;
    int vertex;


    act_ft = &mesh -> fan[edit_state -> fan_chosen];
    act_fd = &pCommands[act_ft -> type & 0x1F];

    /* Do an update on the 'static' data of the fan */
    act_ft -> tx_no    = edit_state -> ft.tx_no;
    act_ft -> tx_flags = edit_state -> ft.tx_flags;
    act_ft -> fx       = edit_state -> ft.fx;

    if (act_ft -> type == edit_state -> ft.type) {

        vrt_diff = 0;       /* May be rotated */

    }
    else {

        vrt_diff = edit_state -> fd.numvertices - act_fd -> numvertices;

    }

    if (0 == vrt_diff) {
        /* Same number of vertices, only overwrite is needed */
        /* Set the new type of fan */
        act_ft -> type = edit_state -> ft.type;

    }
    else {

        if (vrt_diff > 0) {
            /* Insert more vertices -- check for space */
            if (vrt_diff > mesh -> numfreevert) {
                /* Not enough vertices left in buffer */
                return 0;
            }

        }
        /* Set the new type of fan */
        act_ft -> type = edit_state -> ft.type;

        /* Copy from start vertex of next fan */
        src_vtx = mesh -> vrtstart[edit_state -> fan_chosen] + act_fd -> numvertices;
        dst_vtx = mesh -> vrtstart[edit_state -> fan_chosen] + edit_state -> fd.numvertices;
        /* Number of vertices to copy */
        vrt_size = (mesh -> numvert - src_vtx + 1) * sizeof(int);
        /* Add/remove vertices -- update it's numbers */
        mesh -> numfreevert -= vrt_diff;
        mesh -> numvert     += vrt_diff;
        /* Now, clear space or new vertices or remove superfluos vertices */
        /* dest, src, size */
        memmove(&mesh -> vrtx[dst_vtx], &mesh -> vrtx[src_vtx], vrt_size);
        memmove(&mesh -> vrty[dst_vtx], &mesh -> vrty[src_vtx], vrt_size);
        memmove(&mesh -> vrtz[dst_vtx], &mesh -> vrtz[src_vtx], vrt_size);

    }

    /* Fill in the vertex values from type definition */
    vertex = mesh -> vrtstart[edit_state -> fan_chosen];
    for (cnt = 0; cnt < edit_state -> fd.numvertices; cnt++) {
        /* Replace actual values by new values */
        mesh -> vrtx[vertex] = tx + edit_state -> fd.vtx[cnt].x;
        mesh -> vrty[vertex] = ty + edit_state -> fd.vtx[cnt].y;
        mesh -> vrtz[vertex] = edit_state -> fd.vtx[cnt].z;
        vertex++;
    }

    /* Adjust 'vrtstart' for all following fans */
    if (vrt_diff != 0) {
        editmainSetFanStart(mesh);
    }

    return 1;

}

/*
 * Name:
 *     editmainFanTypeRotate
 * Description:
 *     For 'simple' mode. Rotates the given fan type and returns it in
 *     'dest'. The translation has to be done by the caller
 * Input:
 *     type:   Number of fan type to rotate
 *     dest *: Copy of base type, rotated
 *     dir:    Into which direction to rotate
 */
static void editmainFanTypeRotate(int type, COMMAND_T *dest, char dir)
{

    int cnt;
    double angle;
    double result_x, result_y;



    /* Get a copy from  the fan type list, get only the 'lower' numbers */
    memcpy(dest, &pCommands[type & 0x1F], sizeof(COMMAND_T));

    if (dir != EDITMAIN_NORTH) {

        angle  = DEG2RAD(dir * 90.0);

        for (cnt = 0; cnt < dest -> numvertices; cnt++) {

            /* First translate it to have rotation center in middle of fan */
            dest -> vtx[cnt].x -= 64.0;
            dest -> vtx[cnt].y -= 64.0;
            /* And now rotate it */
            result_x = (cos(angle) * dest -> vtx[cnt].x - sin(angle) * dest -> vtx[cnt].y);
            result_y = (sin(angle) * dest -> vtx[cnt].x + cos(angle) * dest -> vtx[cnt].y);
            /* And store it back */
            dest -> vtx[cnt].x = result_x;
            dest -> vtx[cnt].y = result_y;
            /* And move it back to start position */
            dest -> vtx[cnt].x += 64.0;
            dest -> vtx[cnt].y += 64.0;

        }

    }

    /* Otherwise no rotation is needed */
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
    editmainSetFanStart(mesh);      

    mesh -> numfreevert = (MAXTOTALMESHVERTICES - 10) - mesh -> numvert;
    /* Set flag that map has been loaded */
    mesh -> map_loaded = 1;
    mesh -> draw_mode  = EditState.draw_mode;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Creates a new, empty map consisting of all floor or wall types,
 *     depending on 'which'
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 *     which:  Type of map to create : EDITMAIN_NEWFLATMAP / EDITMAIN_NEWSOLIDMAP
 */
static int editmainCreateNewMap(MESH_T *mesh, int which)
{

    int x, y, fan, zadd;
    char fan_fx, fan_type;
    unsigned char tx_no;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x     = EditState.map_size;
    mesh -> tiles_y     = EditState.map_size;
    mesh -> numvert     = 0;                         /* Vertices used in map    */
    mesh -> numfreevert = MAXTOTALMESHVERTICES - 10; /* Vertices left in memory */

    /* ------- Init for flat ---- */
    if (which == EDITMAIN_NEWSOLIDMAP) {
        /* All 'top' tiles */
        fan_type = WALLMAKE_TOP;
        fan_fx   = (MPDFX_WALL | MPDFX_IMPASS);   /* All impassable walls       */
        tx_no    = EDITMAIN_TOP_TILE;
        zadd     = 192;
    }
    else {
        fan_fx   = 0;
        fan_type = WALLMAKE_FLOOR;
        tx_no    = EDITMAIN_DEFAULT_TILE;
        zadd     = 0;
    }

    fan = 0;
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            mesh -> fan[fan].type  = fan_type;
            mesh -> fan[fan].fx    = fan_fx;
            mesh -> fan[fan].tx_no = tx_no;

            if (! editmainFanAdd(mesh, fan, x*EDITMAIN_TILEDIV, y*EDITMAIN_TILEDIV, zadd))
            {
                sprintf(EditState.msg, "%s", "NOT ENOUGH VERTICES!!!");
                return 0;
            }

            fan++;

        }

    }

    return 1;

}

/*
 * Name:
 *     editfileSetVrta
 * Description:
 *     Set the 'vrta'-value for given vertex
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set
 */
static int editfileSetVrta(MESH_T *mesh, int vert)
{
    /* TODO: Get all needed functions from cartman code */
    int newa, x, y, z;
    int brx, bry;
    /*
    int , brz, deltaz, dist, cnt;
    int newlevel, distance, disx, disy;
    */

    /* To make life easier  */
    x = mesh -> vrtx[vert];
    y = mesh -> vrty[vert];
    z = mesh -> vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    /*
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

/*
 * Name:
 *     editmainCreateWallMakeInfo
 * Description:
 *     Fills the given list in a square 5 x 5 from left top to right bottom
 *     with infos about fans. 'fan is the center of the square.
 *     Tiles off the map are signed as 'top' tiles
 *     Generates the 'a'  numbers for all files
 * Input:
 *     mesh*: Pointer on mesh to get the info from
 *     fan:   This is the center fan
 *     wi *:  Array to fill with info needed by the wallmaker-code
 *
 */
static void editmainCreateWallMakeInfo(MESH_T *mesh, int fan, WALLMAKER_INFO_T *wi)
{

    int mid_x, mid_y;
    int tx, ty;
    int x, y;
    int index;
    int pos;
    
    mid_x = fan % mesh -> tiles_x;
    mid_y = fan / mesh -> tiles_x;
    
    /* Now create a square 5 x 5 of tile info */
    index = 0;
    for (y = -2; y < 3; y++) {
    
        for (x = -2; x < 3; x++) {
        
            tx  = mid_x + x;
            ty  = mid_y + y; 
                        
            if ((tx >= 0) && (tx < mesh -> tiles_x)
                && (ty >= 0) && (ty < mesh -> tiles_y)) {

                wi[index].pos  = (ty * mesh -> tiles_x) + tx;
                wi[index].type = mesh -> fan[wi[index].pos].type;
                
            }
            else {

                wi[index].pos  =  -1;
                wi[index].type = WALLMAKE_TOP;  /* Handle as wall */

            }

            wi[index].dir = 0;      /* Initialize with default value */
            
            index++;

        }

    }

}

/*                                             
 * Name:
 *     editmainTranslateWallMakeInfo
 * Description:
 *     Translates the given wallmaker info into fans on map 
 * Input:
 *     mesh*: Pointer on mesh to get the info from
 *     wi *:  Array from wallmaker to create walls from
 *     num_fan: Number of fans in 'wi'-array
 */
static void editmainTranslateWallMakeInfo(MESH_T *mesh, WALLMAKER_INFO_T *wi, int num_fan)
{

    int i;
    char type_no;
	int tx, ty;
    

    for (i = 0; i < num_fan; i++) {

        if (wi[i].pos >= 0) {

            /* -- Do update in any case */
			type_no = wi[i].type;
                
            EditState.ft.type     = type_no;
            EditState.ft.tx_flags = 0;
            EditState.ft.tx_no    = pCommands[type_no & 0x1F].default_tx_no;
            EditState.fan_dir     = wi[i].dir;  
            
            tx = wi[i].pos % mesh -> tiles_x;
			ty = wi[i].pos / mesh -> tiles_x;
            
            editmainFanTypeRotate(EditState.ft.type,
                                  &EditState.fd,
                                  EditState.fan_dir);
            
            editmainDoFanUpdate(mesh, &EditState, tx, ty);

        }

    }

}

/*                                             
 * Name:
 *     editmainLoadAdditionalData
 * Description:
 *     Loads additional data needed for map. SPAWN-Points an Passages 
 * Input:
 *     mesh*: Pointer on mesh to get the info from
 *     wi *:  Array from wallmaker to create walls from
 *
 */
static void editmainLoadAdditionalData(void)
{
    
    EDITDRAW_PASSAGE_T psg[EDITDRAW_MAXPASSAGE + 2];
    EDITDRAW_SPAWNPOS_T sp[EDITDRAW_MAXSPAWNPOS + 2];
    EDITMAIN_PASSAGE_T *ppsg;
    EDITMAIN_SPAWNPT_T *psp;
    int i, psg_no;
    int x, y;
        

    sdlglcfgReadEgoboo("module/passage.txt", &PassageRec);
    sdlglcfgReadEgoboo("module/spawn.txt", &SpawnRec);
    /* ----- Translate passage data to data usable by 'editdraw'  ----- */
    i = 0;
    psg_no = 1;
    ppsg = &Passages[psg_no];
    while(ppsg -> line_name[0] > 0) {
        for (y = ppsg -> topleft.y; y <= ppsg -> topleft.y; y++) {
            for (x = ppsg -> topleft.y; x <= ppsg -> topleft.x; x++) {
                psg[i].no     = psg_no;
                psg[i].fan_no = (y * Mesh.tiles_x) +  x;
                i++;
            }
        }
        /* ----- */
        ppsg++;
        psg_no++;
    }

    psg[i].no = -1;     /* Sign end of list */
    
    /* ---- Translate spawn position data to data usable by 'editdraw' ----- */
    psp = &SpawnObjects[1];
    while(psp -> line_name[0] > 0) {
        if (psp -> x_pos > 0.1) {
            sp[i].x = psp -> x_pos;
            sp[i].y = psp -> y_pos;
            sp[i].z = psp -> z_pos;
        }
        /* ----- */
        psp++;
        i++;
    }

    sp[i].x = -0.1;     /* Sign end of list */

    /* TODO: Now hand the passage data to the draw code */
    /*
    editdrawSetPassage(psg);
    editdrawSetSpawn(sp);
    */

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
 *     map_size: Default map size
 * Output:
 *     Pointer on EditState
 */
EDITMAIN_STATE_T *editmainInit(int map_size)
{

    pCommands = editdrawInitData();
     
    memset(&EditState, 0, sizeof(EDITMAIN_STATE_T));

    EditState.display_flags |= EDITMAIN_SHOW2DMAP;
    EditState.fan_chosen    = -1;   /* No fan chosen                        */
    EditState.ft.type       = -1;   /* No fan-type chosen                   */    
    EditState.map_size      = map_size;

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
 *      command:  What to do
 * Output:
 *      Result of given command
 */
int editmainMap(int command)
{

    int cnt, x, y;


    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh, EditState.fan_chosen, &EditState.ft, &EditState.fd);
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
                /* -------- Now read the data for spawn points and passages */
                editmainLoadAdditionalData();
                return 1;

            }
            break;

        case EDITMAIN_SAVEMAP:
            editmainCalcVrta(&Mesh);
            return editfileSaveMapMesh(&Mesh, EditState.msg);

        case EDITMAIN_ROTFAN:
            if (-1 != EditState.fan_chosen) {
                EditState.fan_dir++;
                EditState.fan_dir &= 0x03;

                editmainFanTypeRotate(EditState.ft.type,
                                      &EditState.fd,
                                      EditState.fan_dir);
                /* Now translate the fan to chosen fan position */
                x = EditState.tx * 128;
                y = EditState.ty * 128;

                for (cnt = 0; cnt < EditState.fd.numvertices; cnt++) {
                    EditState.fd.vtx[cnt].x += x;
                    EditState.fd.vtx[cnt].y += y;
                }

            }
            return 1;

        case EDITMAIN_UPDATEFAN:
            editmainDoFanUpdate(&Mesh, 
                                &EditState, 
                                EditState.tx * 128.0,
                                EditState.ty * 128.0);
            break;
            
        case EDITMAIN_SETFANPROPERTY:
            /* Do update the chosen texture */
            Mesh.fan[EditState.fan_chosen].tx_no = EditState.ft.tx_no;
            /* Do update on flags */
            Mesh.fan[EditState.fan_chosen].fx = EditState.ft.fx;
            /* TODO: Do update on fan vertices, fan type */
            break;
 
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
 *     editmainToggleFlag
 * Description:
 *     Toggles the given flag using 'EditInfo'.
 *     Adjust accompanied
 * Input:
 *     edit_state *: To return the toggle states to caller
 *     which:        Which flag to change
 *
 */
char editmainToggleFlag(int which, unsigned char flag)
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
                EditState.ft.fx = Mesh.fan[EditState.fan_chosen].fx;
            }
            else {
                EditState.ft.fx = 0;
            }
            break;

        case EDITMAIN_TOGGLE_TXHILO:
            if (EditState.fan_chosen >= 0) {
                Mesh.fan[EditState.fan_chosen].tx_flags ^= flag;
                /* And copy it for display */
                EditState.ft.tx_flags = Mesh.fan[EditState.fan_chosen].tx_flags;
            }
            break;

        case EDITMAIN_EDITSTATE:
            if (flag == 0) {
                EditState.edit_mode = 0;
            }
            else {
                EditState.edit_mode++;
                if (EditState.edit_mode > 2) {
                    EditState.edit_mode = 0;
                }
            }
            return EditState.edit_mode;

    }

    return 0;

}

/*
 * Name:
 *     editmainChooseFan
 * Description:
 *     Choose a fan from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'
 * Input:
 *     cx, cy:   Position chosen
 *     w, h:     Extent of rectangle
 *     get_info: Get info from chosen fan yes/no
 */
void editmainChooseFan(int cx, int cy, int w, int h, int get_info)
{

    int fan_no;
    int x, y, i;
    int old_tx, old_ty;


    /* ------------------------- */
    old_tx = EditState.tx;
    old_ty = EditState.ty;

    /* Save it as x/y-position, too */
    EditState.tx = Mesh.tiles_x * cx / w;
    EditState.ty = Mesh.tiles_y * cy / h;

    fan_no = (EditState.ty * Mesh.tiles_x) + EditState.tx;

    if (fan_no >= 0 && fan_no < Mesh.numfan) {

        EditState.fan_chosen = fan_no;
        /* And fill it into 'EditState' for display, if asked for */
        if (get_info) {

            memcpy(&EditState.ft, &Mesh.fan[fan_no], sizeof(FANDATA_T));

        }
        else {
            /* 'Move' actual 'fd'-data to new position */
            /* Now move it to the chosen position */
            x = (EditState.tx - old_tx) * 128;
            y = (EditState.ty - old_ty) * 128;

            for (i = 0; i < EditState.fd.numvertices; i++) {
                EditState.fd.vtx[i].x += x;
                EditState.fd.vtx[i].y += y;
            }
        }

        /* And now set camera to move/look at this position */
        editdrawAdjustCamera(EditState.tx, EditState.ty);         

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
 *     dir:       Direction to browse trough list
 *     fan_name *: Where to print the name of chosen fan
 */
void editmainChooseFanType(int dir, char *fan_name)
{

    char i;
    int  x, y;


    if (dir == -2) {
        /* Reset browsing */
        EditState.bft_no = -1;
        fan_name[0] = 0;
        return;
    }
    else if (dir == 0) {
        /* Start browsing */
        EditState.bft_no = 0;
    }
    else if (dir == -1) {
        if (EditState.bft_no > 0) {
            EditState.bft_no--;
        }
    }
    else {
        if (EditState.bft_no < EDITMAIN_PRESET_MAX) {
            EditState.bft_no++;
        }
        else {  /* Wrap around */
            EditState.bft_no = 0;    
        }
    }

    memcpy(&EditState.ft, &PresetTiles[EditState.bft_no], sizeof(FANDATA_T));
    memcpy(&EditState.fd, &pCommands[EditState.ft.type & 0x1F], sizeof(COMMAND_T));

    /* Now move it to the chosen position */
    x = EditState.tx * 128;
    y = EditState.ty * 128;

    for (i = 0; i < EditState.fd.numvertices; i++) {
        EditState.fd.vtx[i].x += x;
        EditState.fd.vtx[i].y += y;
    }

    EditState.fan_dir = 0;

    sprintf(fan_name, "%s", editmainFanTypeName(EditState.ft.type & 0x1F));

}

/*
 * Name:
 *     editmain2DTex
 * Description:
 *     Draws the texture and chosen texture-part of actual chosen fan
 *     into given rectangle. Uses data from EditState
 * Input:
 *     x, y, w, h: Rectangle to draw into
 */
void editmain2DTex(int x, int y, int w, int h)
{

    if (EditState.fan_chosen >= 0) {

        editdraw2DTex(x, y, w, h,
                      EditState.ft.tx_no,
                      (char)(EditState.ft.type & COMMAND_TEXTUREHI_FLAG));
        
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
 *     is_floor:   Set floor in simple mode       
 */
int editmainFanSet(char is_floor)
{

    int num_fan;
    WALLMAKER_INFO_T wi[30];            /* List of fans to create */


    if (EditState.fan_chosen >= 0) {

        if (EditState.edit_mode == EDITMAIN_EDIT_NONE) {
            return 1;       /* Do nothing, is view-mode */
        }               
        
        if (EditState.edit_mode == EDITMAIN_EDIT_SIMPLE) {
        
            /* Get a list of fans surrounding this one */
            editmainCreateWallMakeInfo(&Mesh, EditState.fan_chosen, wi);
            
            num_fan = wallmakeMakeTile(EditState.fan_chosen, is_floor, wi);
            /* Create tiles from WALLMAKER_INFO_T */
            editmainTranslateWallMakeInfo(&Mesh, wi, num_fan);

            return num_fan;

        }
        else if (EditState.edit_mode == EDITMAIN_EDIT_FREE) {
            return editmainDoFanUpdate(&Mesh, &EditState, EditState.tx, EditState.ty);
        }
    }

    return 0;
}

/*
 * Name:
 *     editmainChooseTex
 * Description:
 *     Choses Texture from square with given coordinates 'cx/cy' from
 *     Rectangle with given size 'w/h'
 *     The texture extent to choose is allways 8x8 squares
 * Input:
 *     cx, cy: Chosen point in rectangle
 *     w,h:    Extent of rectangle
 */
void editmainChooseTex(int cx, int cy, int w, int h, int big)
{

    int tex_x, tex_y;
    char tex_no;
    char new_tex;
 
    
    /* Save it as x/y-position, too */
    tex_x = 8 * cx / w;
    tex_y = 8 * cy / h;
    
    if (big) {
        if (tex_x > 6) tex_x = 6;
        if (tex_y > 6) tex_y = 6;
    }

    tex_no = (char)((tex_y * 8) + tex_x);
    if (tex_no >= 0 && tex_no < 63) {
        /* Valid texture_no, merge it with number of main texture */
        new_tex = (char)(EditState.ft.tx_no & 0xC0);
        new_tex |= tex_no;
        /* Do an update of the edit-state and the mesh */
        EditState.ft.tx_no = new_tex;
    }      
  
}
