//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file mesh.c
/// @brief Functions for creating, reading, and writing egoboo's .mpd mesh file
/// @details

#include "mesh.h"

#include "log.h"
#include "graphic.h"

#include "egoboo_math.h"
#include "egoboo_endian.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"
#include "egoboo_mem.h"
#include "egoboo.h"

#include "SDL_extensions.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// mesh initialization - not accessible by scripts
static void   mesh_init_tile_offset( ego_mpd_t * pmesh );
static void   mesh_make_vrtstart( ego_mpd_t * pmesh );

static bool_t           mesh_free( ego_mpd_t * pmesh );

static tile_mem_t *  mesh_mem_ctor( tile_mem_t * pmem );
static tile_mem_t *  mesh_mem_dtor( tile_mem_t * pmem );
static bool_t        mesh_mem_deallocate( tile_mem_t * pmem );
static bool_t        mesh_mem_allocate( tile_mem_t * pmem, ego_mpd_info_t * pinfo );

static grid_mem_t *  grid_mem_ctor( grid_mem_t * pmem );
static grid_mem_t *  grid_mem_dtor( grid_mem_t * pmem );
static bool_t        grid_mem_allocate( grid_mem_t * pmem, ego_mpd_info_t * pinfo );
static bool_t        grid_mem_deallocate( grid_mem_t * pmem );
static void          grid_make_fanstart( grid_mem_t * pmesh, ego_mpd_info_t * pinfo );

static ego_mpd_info_t * mesh_info_ctor( ego_mpd_info_t * pinfo );
static ego_mpd_info_t * mesh_info_dtor( ego_mpd_info_t * pinfo );
static void             mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y );

// some twist/normal functions
static bool_t mesh_make_normals( ego_mpd_t * pmesh );
static Uint8 cartman_get_fan_twist( ego_mpd_t * pmesh, Uint32 tile );

static bool_t mesh_convert( ego_mpd_t * pmesh_dst, mpd_t * pmesh_src );
static bool_t mesh_make_bbox( ego_mpd_t * pmesh );

static float grid_get_mix( float u0, float u, float v0, float v );

bool_t   mesh_BSP_collide_nodes( BSP_node_t node_lst[], oct_bb_t * pvobj, int colst[], size_t colist_size, int * pcolst_index );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t   mesh;

mpd_BSP_t   mesh_BSP_root =
{
    0,        // size_t
    NULL      // BSP_node_t *
};

static int mesh_BSP_collide_leaf_lvl   = 0;
static int mesh_BSP_collide_nodes_count = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_ctor( ego_mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return pinfo;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
void mesh_info_init( ego_mpd_info_t * pinfo, int numvert, size_t tiles_x, size_t tiles_y )
{
    // set the desired number of tiles
    pinfo->tiles_x = tiles_x;
    pinfo->tiles_y = tiles_y;
    pinfo->tiles_count = pinfo->tiles_x * pinfo->tiles_y;

    // set the desired number of fertices
    if ( numvert < 0 )
    {
        numvert = MAXMESHVERTICES * pinfo->tiles_count;
    };
    pinfo->vertcount = numvert;
}

//--------------------------------------------------------------------------------------------
ego_mpd_info_t * mesh_info_dtor( ego_mpd_info_t * pinfo )
{
    if ( NULL == pinfo ) return NULL;

    memset( pinfo, 0, sizeof( *pinfo ) );

    return pinfo;
}

//--------------------------------------------------------------------------------------------
tile_mem_t * mesh_mem_ctor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
tile_mem_t * mesh_mem_dtor( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    mesh_mem_deallocate( pmem );

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_ctor_default( ego_mpd_t * pmesh )
{
    /// @details BB@> initialize the ego_mpd_t structure

    if ( NULL != pmesh )
    {
        memset( pmesh, 0, sizeof( *pmesh ) );

        mesh_init_tile_offset( pmesh );

        mesh_mem_ctor( &( pmesh->tmem ) );
        grid_mem_ctor( &( pmesh->gmem ) );
        mesh_info_ctor( &( pmesh->info ) );
    }

    // global initialization
    mesh_make_twist();

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_dtor( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    if ( NULL == mesh_mem_dtor( &( pmesh->tmem ) ) ) return NULL;
    if ( NULL == grid_mem_dtor( &( pmesh->gmem ) ) ) return NULL;
    if ( NULL == mesh_info_dtor( &( pmesh->info ) ) ) return NULL;

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_free( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return bfalse;

    mesh_mem_deallocate( &( pmesh->tmem ) );
    grid_mem_deallocate( &( pmesh->gmem ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_renew( ego_mpd_t * pmesh )
{
    pmesh = mesh_dtor( pmesh );

    return mesh_ctor_default( pmesh );
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_ctor( ego_mpd_t * pmesh, int tiles_x, int tiles_y )
{

    if ( NULL == pmesh ) return pmesh;

    // intitalize the mesh info using the max number of vertices for each tile
    mesh_info_init( &( pmesh->info ), -1, tiles_x, tiles_y );

    // allocate the mesh memory
    mesh_mem_allocate( &( pmesh->tmem ), &( pmesh->info ) );
    grid_mem_allocate( &( pmesh->gmem ), &( pmesh->info ) );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_create( ego_mpd_t * pmesh, int tiles_x, int tiles_y )
{
    if ( NULL == pmesh )
    {
        pmesh = EGOBOO_NEW( ego_mpd_t );
        pmesh = mesh_ctor_default( pmesh );
    }

    return mesh_ctor( pmesh, tiles_x, tiles_y );
}

//--------------------------------------------------------------------------------------------
void mesh_init_tile_offset( ego_mpd_t * pmesh )
{
    int cnt;

    // Fix the tile offsets for the mesh textures
    for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
    {
        pmesh->tileoff[cnt].x = (( cnt >> 0 ) & 7 ) / 8.0f;
        pmesh->tileoff[cnt].y = (( cnt >> 3 ) & 7 ) / 8.0f;
    }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_remove_ambient( ego_mpd_t * pmesh )
{
    /// @details BB@> remove extra ambient light in the lightmap

    Uint32 cnt;
    Uint16 min_vrt_a = 255;

    if ( NULL == pmesh ) return bfalse;

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        min_vrt_a = MIN( min_vrt_a, pmesh->gmem.grid_list[cnt].a );
    }

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        pmesh->gmem.grid_list[cnt].a -= min_vrt_a;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_recalc_twist( ego_mpd_t * pmesh )
{
    Uint32 fan;
    ego_mpd_info_t * pinfo;
    tile_mem_t  * ptmem;
    grid_mem_t  * pgmem;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &( pmesh->info );
    ptmem  = &( pmesh->tmem );
    pgmem  = &( pmesh->gmem );

    // recalculate the twist
    for ( fan = 0; fan < pinfo->tiles_count; fan++ )
    {
        Uint8 twist = cartman_get_fan_twist( pmesh, fan );
        pgmem->grid_list[fan].twist = twist;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_set_texture( ego_mpd_t * pmesh, Uint16 tile, Uint16 image )
{
    ego_tile_info_t * ptile;
    Uint16 tile_value, tile_upper, tile_lower;

    if ( !VALID_TILE( pmesh, tile ) ) return bfalse;
    ptile = pmesh->tmem.tile_list + tile;

    // get the upper and lower bits for this tile image
    tile_value = pmesh->tmem.tile_list[tile].img;
    tile_lower = image      & TILE_LOWER_MASK;
    tile_upper = tile_value & TILE_UPPER_MASK;

    // set the actual image
    pmesh->tmem.tile_list[tile].img = tile_upper | tile_lower;

    // update the pre-computed texture info
    return mesh_update_texture( pmesh, tile );
}

//--------------------------------------------------------------------------------------------
bool_t mesh_update_texture( ego_mpd_t * pmesh, Uint16 tile )
{
    size_t mesh_vrt;
    int    tile_vrt;
    Uint16 vertices;
    float  offu, offv;
    Uint16 image;
    Uint8  type;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;
    ego_tile_info_t * ptile;

    ptmem  = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );
    pinfo = &( pmesh->info );

    if ( !VALID_TILE( pmesh, tile ) ) return bfalse;
    ptile = ptmem->tile_list + tile;

    image = TILE_GET_LOWER_BITS( ptile->img );
    type  = ptile->type & 0x3F;

    offu  = pmesh->tileoff[image].x;          // Texture offsets
    offv  = pmesh->tileoff[image].y;

    mesh_vrt = ptmem->tile_list[tile].vrtstart;    // Number of vertices
    vertices = tile_dict[type].numvertices;      // Number of vertices
    for ( tile_vrt = 0; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
    {
        ptmem->tlst[mesh_vrt][SS] = tile_dict[type].u[tile_vrt] + offu;
        ptmem->tlst[mesh_vrt][TT] = tile_dict[type].v[tile_vrt] + offv;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_make_texture( ego_mpd_t * pmesh )
{
    Uint32 cnt;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
    pinfo = &( pmesh->info );

    // set the texture coordinate for every vertex
    for ( cnt = 0; cnt < pinfo->tiles_count; cnt++ )
    {
        mesh_update_texture( pmesh, cnt );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_finalize( ego_mpd_t * pmesh )
{
    if ( NULL == pmesh ) return NULL;

    mesh_make_vrtstart( pmesh );
    mesh_remove_ambient( pmesh );
    mesh_recalc_twist( pmesh );
    mesh_make_normals( pmesh );
    mesh_make_bbox( pmesh );
    mesh_make_texture( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_convert( ego_mpd_t * pmesh_dst, mpd_t * pmesh_src )
{
    Uint32 cnt;

    mpd_mem_t  * pmem_src;
    mpd_info_t * pinfo_src;

    tile_mem_t * ptmem_dst;
    grid_mem_t * pgmem_dst;
    ego_mpd_info_t * pinfo_dst;
    bool_t allocated_dst;

    if ( NULL == pmesh_src ) return bfalse;
    pmem_src = &( pmesh_src->mem );
    pinfo_src = &( pmesh_src->info );

    // clear out all data in the destination mesh
    if ( NULL == mesh_renew( pmesh_dst ) ) return bfalse;
    ptmem_dst = &( pmesh_dst->tmem );
    pgmem_dst = &( pmesh_dst->gmem );
    pinfo_dst = &( pmesh_dst->info );

    // set up the destination mesh from the source mesh
    mesh_info_init( pinfo_dst, pinfo_src->vertcount, pinfo_src->tiles_x, pinfo_src->tiles_y );

    allocated_dst = mesh_mem_allocate( ptmem_dst, pinfo_dst );
    if ( !allocated_dst ) return bfalse;

    allocated_dst = grid_mem_allocate( pgmem_dst, pinfo_dst );
    if ( !allocated_dst ) return bfalse;

    // copy all the per-tile info
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        tile_info_t     * ptile_src = pmem_src->tile_list  + cnt;
        ego_tile_info_t * ptile_dst = ptmem_dst->tile_list + cnt;
        ego_grid_info_t * pgrid_dst = pgmem_dst->grid_list + cnt;

        memset( ptile_dst, 0, sizeof( *ptile_dst ) );
        ptile_dst->type         = ptile_src->type;
        ptile_dst->img          = ptile_src->img;
        ptile_dst->vrtstart     = ptile_src->vrtstart;

        memset( pgrid_dst, 0, sizeof( *pgrid_dst ) );
        pgrid_dst->fx    = ptile_src->fx;
        pgrid_dst->twist = ptile_src->twist;

        // lcache is set in a hepler function
        // nlst is set in a hepler function
    }

    // copy all the per-vertex info
    for ( cnt = 0; cnt < pinfo_src->vertcount; cnt++ )
    {
        // copy all info from mpd_mem_t
        ptmem_dst->plst[cnt][XX] = pmem_src->vlst[cnt].pos.x;
        ptmem_dst->plst[cnt][YY] = pmem_src->vlst[cnt].pos.y;
        ptmem_dst->plst[cnt][ZZ] = pmem_src->vlst[cnt].pos.z;

        // default color
        ptmem_dst->clst[cnt][XX] = ptmem_dst->clst[cnt][GG] = ptmem_dst->clst[cnt][BB] = 0.0f;

        // tlist is set below
    }

    // copy some of the pre-calculated grid lighting
    for ( cnt = 0; cnt < pinfo_dst->tiles_count; cnt++ )
    {
        size_t vertex = ptmem_dst->tile_list[cnt].vrtstart;
        pgmem_dst->grid_list[cnt].a = pmem_src->vlst[vertex].a;
        pgmem_dst->grid_list[cnt].l = 0.0f;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mpd_t * mesh_load( const char *modname, ego_mpd_t * pmesh )
{
    // trap bad module names
    if ( !VALID_CSTR( modname ) ) return pmesh;

    // initialize the mesh
    {
        // create a new mesh if we are passed a NULL pointer
        if ( NULL == pmesh )
        {
            pmesh = mesh_ctor_default( pmesh );
        }
        if ( NULL == pmesh ) return pmesh;

        // free any memory that has been allocated
        pmesh = mesh_renew( pmesh );
    }

    // actually do the loading
    {
        mpd_t  local_mpd, * pmpd;

        // load a raw mpd
        mpd_ctor( &local_mpd );
        tile_dictionary_load( "data/fans.txt", tile_dict, MAXMESHTYPE );
        pmpd = mpd_load( vfs_resolveReadFilename( "data/level.mpd" ), &local_mpd );

        // convert it into a convenient version for egoboo
        if ( !mesh_convert( pmesh, pmpd ) )
        {
            pmesh = NULL;
        }

        // delete the now useless mpd data
        mpd_dtor( &local_mpd );
    }

    // do some calculation to set up the mpd as a game mesh
    pmesh = mesh_finalize( pmesh );

    return pmesh;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block_int( ego_mpd_t * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->gmem.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->gmem.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->gmem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile_int( ego_mpd_t * pmesh, int tile_x,  int tile_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( tile_x < 0 || tile_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( tile_y < 0 || tile_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return tile_x + pmesh->gmem.tilestart[tile_y];
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_ctor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return pmem;

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
grid_mem_t * grid_mem_dtor( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return NULL;

    grid_mem_deallocate( pmem );

    memset( pmem, 0, sizeof( *pmem ) );

    return pmem;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_allocate( grid_mem_t * pgmem, ego_mpd_info_t * pinfo )
{

    if ( NULL == pgmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !grid_mem_deallocate( pgmem ) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // set the desired blocknumber of grids
    pgmem->grids_x = pinfo->tiles_x;
    pgmem->grids_y = pinfo->tiles_y;
    pgmem->grid_count = pgmem->grids_x * pgmem->grids_y;

    // set the mesh edge info
    pgmem->edge_x = ( pgmem->grids_x + 1 ) << TILE_BITS;
    pgmem->edge_y = ( pgmem->grids_y + 1 ) << TILE_BITS;

    // set the desired blocknumber of blocks
    pgmem->blocks_x = ( pinfo->tiles_x >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_x, 0x03 ) ) pgmem->blocks_x++;

    pgmem->blocks_y = ( pinfo->tiles_y >> 2 );
    if ( HAS_SOME_BITS( pinfo->tiles_y, 0x03 ) ) pgmem->blocks_y++;

    pgmem->blocks_count = pgmem->blocks_x * pgmem->blocks_y;

    // allocate per-grid memory
    pgmem->grid_list = EGOBOO_NEW_ARY( ego_grid_info_t, pgmem->grid_count );
    if ( NULL == pgmem->grid_list ) goto grid_mem_allocate_fail;

    // helper info
    pgmem->blockstart = EGOBOO_NEW_ARY( Uint32, pgmem->blocks_y );
    if ( NULL == pgmem->blockstart ) goto grid_mem_allocate_fail;

    pgmem->tilestart  = EGOBOO_NEW_ARY( Uint32, pinfo->tiles_y );
    if ( NULL == pgmem->tilestart ) goto grid_mem_allocate_fail;

    // initialize the fanstart/blockstart data
    grid_make_fanstart( pgmem, pinfo );

    return btrue;

grid_mem_allocate_fail:

    grid_mem_deallocate( pgmem );
    log_error( "grid_mem_allocate() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t grid_mem_deallocate( grid_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->blockstart != NULL )
    {
        free( pmem->blockstart );
        pmem->blockstart = NULL;
    }

    if ( pmem->tilestart != NULL )
    {
        free( pmem->tilestart );
        pmem->tilestart = NULL;
    }

    // reset some values to safe values
    memset( pmem, 0, sizeof( *pmem ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_allocate( tile_mem_t * pmem, ego_mpd_info_t * pinfo )
{

    if ( NULL == pmem || NULL == pinfo || 0 == pinfo->vertcount ) return bfalse;

    // free any memory already allocated
    if ( !mesh_mem_deallocate( pmem ) ) return bfalse;

    if ( pinfo->vertcount > MESH_MAXTOTALVERTRICES )
    {
        log_warning( "Mesh requires too much memory ( %d requested, but max is %d ). \n", pinfo->vertcount, MESH_MAXTOTALVERTRICES );
        return bfalse;
    }

    // allocate per-vertex memory
    pmem->plst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->plst ) goto mesh_mem_allocate_fail;

    pmem->tlst = EGOBOO_NEW_ARY( GLXvector2f, pinfo->vertcount );
    if ( NULL == pmem->tlst ) goto mesh_mem_allocate_fail;

    pmem->clst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->clst ) goto mesh_mem_allocate_fail;

    pmem->nlst = EGOBOO_NEW_ARY( GLXvector3f, pinfo->vertcount );
    if ( NULL == pmem->nlst ) goto mesh_mem_allocate_fail;

    // allocate per-tile memory
    pmem->tile_list  = ego_tile_info_alloc_ary( pinfo->tiles_count );
    if ( NULL == pmem->tile_list ) goto mesh_mem_allocate_fail;

    pmem->vert_count = pinfo->vertcount;
    pmem->tile_count = pinfo->tiles_count;

    return btrue;

mesh_mem_allocate_fail:

    mesh_mem_deallocate( pmem );
    log_error( "mesh_mem_allocate() - reduce the maximum number of vertices! (Check MESH_MAXTOTALVERTRICES)\n" );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_mem_deallocate( tile_mem_t * pmem )
{
    if ( NULL == pmem ) return bfalse;

    // free the memory
    if ( pmem->plst != NULL )
    {
        free( pmem->plst );
        pmem->plst = NULL;
    }

    if ( pmem->nlst != NULL )
    {
        free( pmem->nlst );
        pmem->nlst = NULL;
    }

    if ( pmem->clst != NULL )
    {
        free( pmem->clst );
        pmem->clst = NULL;
    }

    if ( pmem->tlst != NULL )
    {
        free( pmem->tlst );
        pmem->tlst = NULL;
    }

    // per-tile values
    if ( NULL != pmem->tile_list )
    {
        free( pmem->tile_list );
        pmem->tile_list = NULL;
    }

    // reset some values to safe values
    pmem->vert_count  = 0;
    pmem->tile_count = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
void grid_make_fanstart( grid_mem_t * pgmem, ego_mpd_info_t * pinfo )
{
    /// @details ZZ@> This function builds a look up table to ease calculating the
    ///    fan number given an x,y pair

    int cnt;

    if ( NULL == pgmem || NULL == pinfo ) return;

    // do the tilestart
    for ( cnt = 0; cnt < pinfo->tiles_y; cnt++ )
    {
        pgmem->tilestart[cnt] = pinfo->tiles_x * cnt;
    }

    // calculate some of the block info
    if ( pgmem->blocks_x >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the x direction too large (%d out of %d).\n", pgmem->blocks_x, MAXMESHBLOCKY );
    }

    if ( pgmem->blocks_y >= MAXMESHBLOCKY )
    {
        log_warning( "Number of mesh blocks in the y direction too large (%d out of %d).\n", pgmem->blocks_y, MAXMESHBLOCKY );
    }

    // do the blockstart
    for ( cnt = 0; cnt < pgmem->blocks_y; cnt++ )
    {
        pgmem->blockstart[cnt] = pgmem->blocks_x * cnt;
    }

}

//--------------------------------------------------------------------------------------------
void mesh_make_vrtstart( ego_mpd_t * pmesh )
{
    size_t vert;
    Uint32 tile;

    ego_mpd_info_t * pinfo;
    tile_mem_t  * ptmem;

    if ( NULL == pmesh ) return;

    pinfo = &( pmesh->info );
    ptmem  = &( pmesh->tmem );

    vert = 0;
    for ( tile = 0; tile < pinfo->tiles_count; tile++ )
    {
        Uint8 ttype;

        ptmem->tile_list[tile].vrtstart = vert;

        ttype = ptmem->tile_list[tile].type;

        // throw away any remaining upper bits
        ttype &= 0x3F;

        vert += tile_dict[ttype].numvertices;
    }

    if ( vert != pinfo->vertcount )
    {
        log_warning( "mesh_make_vrtstart() - unexpected number of vertices %d of %d\n", vert, pinfo->vertcount );
    }
}

//--------------------------------------------------------------------------------------------
void mesh_make_twist()
{
    /// @details ZZ@> This function precomputes surface normals and steep hill acceleration for
    ///    the mesh

    Uint16 cnt;

    for ( cnt = 0; cnt < 256; cnt++ )
    {
        GLfloat nrm[3];

        twist_to_normal( cnt, nrm, 1.0f );

        map_twist_nrm[cnt].x = nrm[XX];
        map_twist_nrm[cnt].y = nrm[YY];
        map_twist_nrm[cnt].z = nrm[ZZ];

        map_twist_x[cnt] = ( Uint16 )( - vec_to_facing( nrm[2], nrm[1] ) );
        map_twist_y[cnt] = vec_to_facing( nrm[2], nrm[0] );

        // this is about 5 degrees off of vertical
        map_twist_flat[cnt] = bfalse;
        if ( nrm[2] > 0.9945f )
        {
            map_twist_flat[cnt] = btrue;
        }

        // projection of the gravity perpendicular to the surface
        map_twistvel_x[cnt] = nrm[0] * ABS( nrm[2] ) * gravity;
        map_twistvel_y[cnt] = nrm[1] * ABS( nrm[2] ) * gravity;
        map_twistvel_z[cnt] = nrm[2] * nrm[2] * gravity;
    }
}

//---------------------------------------------------------------------------------------------
float mesh_get_level( ego_mpd_t * pmesh, float x, float y )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precisely

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = mesh_get_tile( pmesh, x, y );
    if ( !VALID_TILE( pmesh, tile ) ) return 0;

    ix = x;
    iy = y;

    ix &= TILE_MASK;
    iy &= TILE_MASK;

    z0 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 0 ][ZZ];
    z1 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 1 ][ZZ];
    z2 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 2 ][ZZ];
    z3 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 3 ][ZZ];

    zleft  = ( z0 * ( TILE_SIZE - iy ) + z3 * iy ) / TILE_SIZE;
    zright = ( z1 * ( TILE_SIZE - iy ) + z2 * iy ) / TILE_SIZE;
    zdone  = ( zleft * ( TILE_SIZE - ix ) + zright * ix ) / TILE_SIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x <= pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y <= pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= BLOCK_BITS;
        iy >>= BLOCK_BITS;

        block = mesh_get_block_int( pmesh, ix, iy );
    }

    return block;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( ego_mpd_t * pmesh, float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y < pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= TILE_BITS;
        iy >>= TILE_BITS;

        tile = mesh_get_tile_int( pmesh, ix, iy );
    }

    return tile;
}

//------------------------------------------------------------------------------
Uint8 cartman_get_fan_twist( ego_mpd_t * pmesh, Uint32 tile )
{
    size_t vrtstart;
    float z0, z1, z2, z3;
    float zx, zy;

    // check for a valid tile
    if ( INVALID_TILE == tile  || tile > pmesh->info.tiles_count ) return TWIST_FLAT;

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->tmem.tile_list[tile].img ) return TWIST_FLAT;

    vrtstart = pmesh->tmem.tile_list[tile].vrtstart;

    z0 = pmesh->tmem.plst[vrtstart + 0][ZZ];
    z1 = pmesh->tmem.plst[vrtstart + 1][ZZ];
    z2 = pmesh->tmem.plst[vrtstart + 2][ZZ];
    z3 = pmesh->tmem.plst[vrtstart + 3][ZZ];

    zx = CARTMAN_FIXNUM * ( z0 + z3 - z1 - z2 ) / CARTMAN_SLOPE;
    zy = CARTMAN_FIXNUM * ( z2 + z3 - z0 - z1 ) / CARTMAN_SLOPE;

    return cartman_get_twist( zx, zy );
}

//------------------------------------------------------------------------------
bool_t mesh_clear_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags )
{
    Uint32 old_flags;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    // save a copy of the fx
    old_flags = pmesh->gmem.grid_list[itile].fx;

    // clear the wall and impass flags
    pmesh->gmem.grid_list[itile].fx &= ~flags;

    // succeed only of something actually changed
    return 0 != ( old_flags & flags );
}

//------------------------------------------------------------------------------
bool_t mesh_add_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags )
{
    Uint32 old_flags;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    // save a copy of the fx
    old_flags = pmesh->gmem.grid_list[itile].fx;

    // add in the flags
    pmesh->gmem.grid_list[itile].fx = old_flags | flags;

    // succeed only of something actually changed
    return 0 != ( old_flags & flags );
}

//------------------------------------------------------------------------------
Uint32 mesh_test_fx( ego_mpd_t * pmesh, Uint32 itile, Uint32 flags )
{
    // test for mesh
    if ( NULL == pmesh ) return 0;

    // test for invalid tile
    if ( itile > pmesh->info.tiles_count )
    {
        return flags & ( MPDFX_WALL | MPDFX_IMPASS );
    }

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->tmem.tile_list[itile].img )
    {
        return flags & ( MPDFX_WALL | MPDFX_IMPASS );
    }

    return pmesh->gmem.grid_list[itile].fx & flags;
}

//------------------------------------------------------------------------------
bool_t mesh_make_bbox( ego_mpd_t * pmesh )
{
    /// @details BB@> set the bounding box for each tile, and for the entire mesh

    size_t mesh_vrt;
    int tile_vrt;
    Uint32 cnt;

    tile_mem_t * ptmem;
    grid_mem_t * pgmem;
    ego_mpd_info_t * pinfo;

    if ( NULL == pmesh ) return bfalse;
    ptmem  = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );
    pinfo = &( pmesh->info );

    ptmem->bbox.mins[XX] = ptmem->bbox.maxs[XX] = ptmem->plst[0][XX];
    ptmem->bbox.mins[YY] = ptmem->bbox.maxs[YY] = ptmem->plst[0][YY];
    ptmem->bbox.mins[ZZ] = ptmem->bbox.maxs[ZZ] = ptmem->plst[0][ZZ];

    for ( cnt = 0; cnt < pmesh->info.tiles_count; cnt++ )
    {
        oct_bb_t       * poct;
        ego_tile_info_t * ptile;
        Uint16 vertices;
        Uint8 type;

        ptile = ptmem->tile_list + cnt;
        poct   = &( ptile->oct );

        type = ptile->type;
        type &= 0x3F;

        mesh_vrt = ptmem->tile_list[cnt].vrtstart;    // Number of vertices
        vertices = tile_dict[type].numvertices;          // Number of vertices

        // set the bounding box for this tile
        poct->mins[XX] = poct->maxs[XX] = ptmem->plst[mesh_vrt][XX];
        poct->mins[YY] = poct->maxs[YY] = ptmem->plst[mesh_vrt][YY];
        poct->mins[ZZ] = poct->maxs[ZZ] = ptmem->plst[mesh_vrt][ZZ];
        for ( tile_vrt = 1; tile_vrt < vertices; tile_vrt++, mesh_vrt++ )
        {
            poct->mins[XX] = MIN( poct->mins[XX], ptmem->plst[mesh_vrt][XX] );
            poct->mins[YY] = MIN( poct->mins[YY], ptmem->plst[mesh_vrt][YY] );
            poct->mins[ZZ] = MIN( poct->mins[ZZ], ptmem->plst[mesh_vrt][ZZ] );

            poct->maxs[XX] = MAX( poct->maxs[XX], ptmem->plst[mesh_vrt][XX] );
            poct->maxs[YY] = MAX( poct->maxs[YY], ptmem->plst[mesh_vrt][YY] );
            poct->maxs[ZZ] = MAX( poct->maxs[ZZ], ptmem->plst[mesh_vrt][ZZ] );
        }

        // extend the mesh bounding box
        ptmem->bbox.mins[XX] = MIN( ptmem->bbox.mins[XX], poct->mins[XX] );
        ptmem->bbox.mins[YY] = MIN( ptmem->bbox.mins[YY], poct->mins[YY] );
        ptmem->bbox.mins[ZZ] = MIN( ptmem->bbox.mins[ZZ], poct->mins[ZZ] );

        ptmem->bbox.maxs[XX] = MAX( ptmem->bbox.maxs[XX], poct->maxs[XX] );
        ptmem->bbox.maxs[YY] = MAX( ptmem->bbox.maxs[YY], poct->maxs[YY] );
        ptmem->bbox.maxs[ZZ] = MAX( ptmem->bbox.maxs[ZZ], poct->maxs[ZZ] );
    }

    return btrue;
}

//------------------------------------------------------------------------------
bool_t mesh_make_normals( ego_mpd_t * pmesh )
{
    /// @details BB@> this function calculates a set of normals for the 4 corners
    ///               of a given tile. It is supposed to generate smooth normals for
    ///               most tiles, but where there is a creas (i.e. between the floor and
    ///               a wall) the normals should not be smoothed.

    int ix, iy;
    Uint32 fan0, fan1;
    tile_mem_t * ptmem;
    grid_mem_t * pgmem;

    int     edge_is_crease[4];
    fvec3_t nrm_lst[4], vec_sum;
    float   weight_lst[4];

    // test for mesh
    if ( NULL == pmesh ) return bfalse;
    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

    // set the default normal for each fan, based on the calculated twist value
    for ( fan0 = 0; fan0 < ptmem->tile_count; fan0++ )
    {
        Uint8 twist = pgmem->grid_list[fan0].twist;

        ptmem->nlst[fan0][XX] = map_twist_nrm[twist].x;
        ptmem->nlst[fan0][YY] = map_twist_nrm[twist].y;
        ptmem->nlst[fan0][ZZ] = map_twist_nrm[twist].z;
    }

    // find an "average" normal of each corner of the tile
    for ( iy = 0; iy < pmesh->info.tiles_y; iy++ )
    {
        for ( ix = 0; ix < pmesh->info.tiles_x; ix++ )
        {
            int ix_off[4] = {0, 1, 1, 0};
            int iy_off[4] = {0, 0, 1, 1};
            int i, j, k;

            fan0 = mesh_get_tile_int( pmesh, ix, iy );
            if ( !VALID_TILE( pmesh, fan0 ) ) continue;

            nrm_lst[0].x = ptmem->nlst[fan0][XX];
            nrm_lst[0].y = ptmem->nlst[fan0][YY];
            nrm_lst[0].z = ptmem->nlst[fan0][ZZ];

            // for each corner of this tile
            for ( i = 0; i < 4; i++ )
            {
                int dx, dy;
                int loc_ix_off[4];
                int loc_iy_off[4];

                // the offset list needs to be shifted depending on what i is
                j = ( 6 - i ) % 4;

                if ( 1 == ix_off[4-j] ) dx = -1; else dx = 0;
                if ( 1 == iy_off[4-j] ) dy = -1; else dy = 0;

                for ( k = 0; k < 4; k++ )
                {
                    loc_ix_off[k] = ix_off[( 4-j + k ) % 4 ] + dx;
                    loc_iy_off[k] = iy_off[( 4-j + k ) % 4 ] + dy;
                }

                // cache the normals
                // nrm_lst[0] is already known.
                for ( j = 1; j < 4; j++ )
                {
                    int jx, jy;

                    jx = ix + loc_ix_off[j];
                    jy = iy + loc_iy_off[j];

                    fan1 = mesh_get_tile_int( pmesh, jx, jy );

                    if ( VALID_TILE( pmesh, fan1 ) )
                    {
                        nrm_lst[j].x = ptmem->nlst[fan1][XX];
                        nrm_lst[j].y = ptmem->nlst[fan1][YY];
                        nrm_lst[j].z = ptmem->nlst[fan1][ZZ];

                        if ( nrm_lst[j].z < 0 )
                        {
                            nrm_lst[j].x *= -1.0f;
                            nrm_lst[j].y *= -1.0f;
                            nrm_lst[j].z *= -1.0f;
                        }
                    }
                    else
                    {
                        nrm_lst[j].x = 0;
                        nrm_lst[j].y = 0;
                        nrm_lst[j].z = 1;
                    }
                }

                // find the creases
                for ( j = 0; j < 4; j++ )
                {
                    float vdot;
                    int k = ( j + 1 ) % 4;

                    vdot = fvec3_dot_product( nrm_lst[j].v, nrm_lst[k].v );

                    edge_is_crease[j] = ( vdot < INV_SQRT_TWO );

                    weight_lst[j] = fvec3_dot_product( nrm_lst[j].v, nrm_lst[0].v );
                }

                weight_lst[0] = 1.0f;
                if ( edge_is_crease[0] )
                {
                    // this means that there is a crease between tile 0 and 1
                    weight_lst[1] = 0.0f;
                }

                if ( edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 3
                    weight_lst[3] = 0.0f;
                }

                if ( edge_is_crease[0] && edge_is_crease[3] )
                {
                    // this means that there is a crease between tile 0 and 1
                    // and a crease between tile 0 and 3, isolating tile 2
                    weight_lst[2] = 0.0f;
                }

                vec_sum = nrm_lst[0];
                for ( j = 1; j < 4; j++ )
                {
                    if ( weight_lst[j] > 0.0f )
                    {
                        vec_sum.x += nrm_lst[j].x * weight_lst[j];
                        vec_sum.y += nrm_lst[j].y * weight_lst[j];
                        vec_sum.z += nrm_lst[j].z * weight_lst[j];
                    }
                }

                vec_sum = fvec3_normalize( vec_sum.v );

                ptmem->tile_list[fan0].ncache[i][XX] = vec_sum.x;
                ptmem->tile_list[fan0].ncache[i][YY] = vec_sum.y;
                ptmem->tile_list[fan0].ncache[i][ZZ] = vec_sum.z;
            }
        }
    }

    //            dy_min = iy_off[i] - 1;
    //            dy_max = iy_off[i];

    //            wt_cnt = 0;
    //            vec_sum.x = vec_sum.y = vec_sum.z = 0.0f;
    //            for (dy = dy_min; dy <= dy_max; dy++)
    //            {
    //                jy = iy + dy;
    //                for (dx = dx_min; dx <= dx_max; dx++)
    //                {
    //                    jx = ix + dx;

    //                    fan1 = mesh_get_tile_int(pmesh, jx, jy);
    //                    if ( VALID_TILE(pmesh, fan1) )
    //                    {
    //                        float wt;

    //                        vec1.x = ptmem->nlst[fan1][XX];
    //                        vec1.y = ptmem->nlst[fan1][YY];
    //                        vec1.z = ptmem->nlst[fan1][ZZ];
    //                        if ( vec1.z < 0 )
    //                        {
    //                            vec1.x *= -1.0f;
    //                            vec1.y *= -1.0f;
    //                            vec1.z *= -1.0f;
    //                        }

    //                        wt = fvec3_dot_product( vec0.v, vec1.v );
    //                        if ( wt > 0 )
    //                        {
    //                            vec_sum.x += wt * vec1.x;
    //                            vec_sum.y += wt * vec1.y;
    //                            vec_sum.z += wt * vec1.z;

    //                            wt_cnt += 1;
    //                        }
    //                    }
    //                }
    //            }

    //            if ( wt_cnt > 1 )
    //            {
    //                vec_sum = fvec3_normalize( vec_sum.v );

    //                ptmem->ncache[fan0][i][XX] = vec_sum.x;
    //                ptmem->ncache[fan0][i][YY] = vec_sum.y;
    //                ptmem->ncache[fan0][i][ZZ] = vec_sum.z;
    //            }
    //            else
    //            {
    //                ptmem->ncache[fan0][i][XX] = vec0.x;
    //                ptmem->ncache[fan0][i][YY] = vec0.y;
    //                ptmem->ncache[fan0][i][ZZ] = vec0.z;
    //            }
    //        }
    //    }
    //}

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t grid_light_one_corner( ego_mpd_t * pmesh, int fan, float height, float nrm[], float * plight )
{
    bool_t             reflective;
    lighting_cache_t * lighting;

    if ( NULL == pmesh || NULL == plight || !VALID_TILE( pmesh, fan ) ) return bfalse;

    // get the grid lighting
    lighting = &( pmesh->gmem.grid_list[fan].cache );

    reflective = ( 0 != mesh_test_fx( pmesh, fan, MPDFX_DRAWREF ) );

    // evaluate the grid lighting at this node
    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( lighting, nrm, height, pmesh->tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( lighting, nrm, height, pmesh->tmem.bbox, NULL, NULL );
    }

    // clip the light to a reasonable value
    ( *plight ) = CLIP(( *plight ), 0, 255 );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_test_one_corner( ego_mpd_t * pmesh, GLXvector3f pos, float * pdelta )
{
    float loc_delta, low_delta, hgh_delta;
    float hgh_wt, low_wt;

    if ( NULL == pdelta ) pdelta = &loc_delta;

    // interpolate the lighting for the given corner of the mesh
    *pdelta = grid_lighting_test( pmesh, pos, &low_delta, &hgh_delta );

    // determine the weighting
    hgh_wt = ( pos[ZZ] - pmesh->tmem.bbox.mins[kZ] ) / ( pmesh->tmem.bbox.maxs[kZ] - pmesh->tmem.bbox.mins[kZ] );
    hgh_wt = CLIP( hgh_wt, 0.0f, 1.0f );
    low_wt = 1.0f - hgh_wt;

    *pdelta = low_wt * low_delta + hgh_wt * hgh_delta;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_light_one_corner( ego_mpd_t * pmesh, int itile, GLXvector3f pos, GLXvector3f nrm, float * plight )
{
    lighting_cache_t grid_light;
    bool_t reflective;

    if ( !VALID_TILE( pmesh, itile ) ) return bfalse;

    // add in the effect of this lighting cache node
    reflective = ( 0 != mesh_test_fx( pmesh, itile, MPDFX_DRAWREF ) );

    // interpolate the lighting for the given corner of the mesh
    grid_lighting_interpolate( pmesh, &grid_light, pos[XX], pos[YY] );

    if ( reflective )
    {
        float light_dir, light_amb;

        lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], pmesh->tmem.bbox, &light_amb, &light_dir );

        // make ambient light only illuminate 1/2
        ( *plight ) = light_amb + 0.5f * light_dir;
    }
    else
    {
        ( *plight ) = lighting_evaluate_cache( &grid_light, nrm, pos[ZZ], pmesh->tmem.bbox, NULL, NULL );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_test_corners( ego_mpd_t * pmesh, int itile, float threshold )
{
    bool_t retval;
    int corner;

    tile_mem_t    * ptmem;
    light_cache_t * lcache;
    light_cache_t * d1_cache;

    if ( NULL == pmesh || !VALID_TILE( pmesh, itile ) ) return bfalse;
    ptmem = &( pmesh->tmem );

    // get the normal and lighting cache for this tile
    lcache   = &( ptmem->tile_list[itile].lcache );
    d1_cache = &( ptmem->tile_list[itile].d1_cache );

    retval = bfalse;
    for ( corner = 0; corner < 4; corner++ )
    {
        float            delta;
        float          * pdelta;
        float          * plight;
        GLXvector3f    * ppos;

        pdelta = ( *d1_cache ) + corner;
        plight = ( *lcache ) + corner;
        ppos   = ptmem->plst + ptmem->tile_list[itile].vrtstart + corner;

        mesh_test_one_corner( pmesh, *ppos, &delta );

        if ( 0.0f == *plight )
        {
            delta = 10.0f;
        }
        else
        {
            delta /= *plight;
            delta = CLIP( delta, 0, 10.0f );
        }

        *pdelta += delta;

        if ( *pdelta > threshold ) retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
float mesh_light_corners( ego_mpd_t * pmesh, int itile, float mesh_lighting_keep )
{
    int corner;
    float max_delta;

    ego_mpd_info_t * pinfo;
    tile_mem_t     * ptmem;
    grid_mem_t     * pgmem;
    normal_cache_t * ncache;
    light_cache_t  * lcache;
    light_cache_t  * d1_cache, * d2_cache;

    if ( NULL == pmesh || !VALID_TILE( pmesh, itile ) ) return 0.0f;
    pinfo = &( pmesh->info );
    ptmem = &( pmesh->tmem );
    pgmem = &( pmesh->gmem );

    // get the normal and lighting cache for this tile
    ncache   = &( ptmem->tile_list[itile].ncache );
    lcache   = &( ptmem->tile_list[itile].lcache );
    d1_cache = &( ptmem->tile_list[itile].d1_cache );
    d2_cache = &( ptmem->tile_list[itile].d2_cache );

    max_delta = 0.0f;
    for ( corner = 0; corner < 4; corner++ )
    {
        float light_new, light_old, delta, light_tmp;

        GLXvector3f    * pnrm;
        float          * plight;
        float          * pdelta1, * pdelta2;
        GLXvector3f    * ppos;

        pnrm    = ( *ncache ) + corner;
        plight  = ( *lcache ) + corner;
        pdelta1 = ( *d1_cache ) + corner;
        pdelta2 = ( *d2_cache ) + corner;
        ppos    = ptmem->plst + ptmem->tile_list[itile].vrtstart + corner;

        light_new = 0.0f;
        mesh_light_one_corner( pmesh, itile, *ppos, *pnrm, &light_new );

        if ( *plight != light_new )
        {
            light_old = *plight;
            *plight = light_old * mesh_lighting_keep + light_new * ( 1.0f - mesh_lighting_keep );

            // measure the actual delta
            delta = ABS( light_old - *plight );

            // measure the relative change of the lighting
            light_tmp = 0.5f * ( ABS( *plight ) + ABS( light_old ) );
            if ( 0.0f == light_tmp )
            {
                delta = 10.0f;
            }
            else
            {
                delta /= light_tmp;
                delta = CLIP( delta, 0.0f, 10.0f );
            }

            // add in the actual change this update
            *pdelta2 += ABS( delta );

            // update the estimate to match the actual change
            *pdelta1 = *pdelta2;
        }

        max_delta = MAX( max_delta, *pdelta1 );
    }

    return max_delta;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_interpolate_vertex( tile_mem_t * pmem, int fan, float pos[], float * plight )
{
    int cnt;
    int ix_off[4] = {0, 1, 1, 0}, iy_off[4] = {0, 0, 1, 1};
    float u, v, wt, weight_sum;

    oct_bb_t       * poct;
    light_cache_t  * lc;

    if ( NULL == plight ) return bfalse;
    ( *plight ) = 0.0f;

    if ( NULL == pmem ) return bfalse;

    poct = &( pmem->tile_list[fan].oct );
    lc   = &( pmem->tile_list[fan].lcache );

    // determine a u,v coordinate for the vertex
    u = ( pos[XX] - poct->mins[XX] ) / ( poct->maxs[XX] - poct->mins[XX] );
    v = ( pos[YY] - poct->mins[YY] ) / ( poct->maxs[YY] - poct->mins[YY] );

    // average the cached data on the 4 corners of the mesh
    weight_sum = 0.0f;
    for ( cnt = 0; cnt < 4; cnt++ )
    {
        wt = grid_get_mix( ix_off[cnt], u, iy_off[cnt], v );

        weight_sum += wt;
        ( *plight )  += wt * ( *lc )[cnt];
    }

    if (( *plight ) > 0 && weight_sum > 0.0 )
    {
        ( *plight ) /= weight_sum;
    }
    else
    {
        ( *plight ) = 0;
    }

    ( *plight ) = CLIP(( *plight ), 0, 255 );

    return btrue;
}

#define BLAH_MIX_1(DU,UU) (4.0f/9.0f*((UU)-(-1+(DU)))*((2+(DU))-(UU)))
#define BLAH_MIX_2(DU,UU,DV,VV) (BLAH_MIX_1(DU,UU)*BLAH_MIX_1(DV,VV))

float grid_get_mix( float u0, float u, float v0, float v )
{
    float wt_u, wt_v;
    float du = u - u0;
    float dv = v - v0;

    // du *= 1.0f;
    if ( ABS( du ) > 1.0 ) return 0;
    wt_u = ( 1.0f - du ) * ( 1.0f + du );

    // dv *= 1.0f;
    if ( ABS( dv ) > 1.0 ) return 0;
    wt_v = ( 1.0f - dv ) * ( 1.0f + dv );

    return wt_u * wt_v;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_hitawall( ego_mpd_t * pmesh, float pos[], float radius, Uint32 bits, float nrm[], float * pressure )
{
    /// @details BB@> an abstraction of the functions of __chrhitawall() and __prthitawall()

    Uint32 pass;
    Uint32 itile;
    int   ix_min, ix_max, iy_min, iy_max;
    float fx_min, fx_max, fy_min, fy_max, obj_area;
    float dist2;
    int ix, iy, tx0, ty0;
    bool_t invalid;

    float  loc_pressure;
    fvec3_base_t loc_nrm;

    ego_mpd_info_t  * pinfo;
    ego_tile_info_t * tlist;
    ego_grid_info_t * glist;

    // deal with the optional parameters
    if ( NULL == pressure ) pressure = &loc_pressure;
    *pressure = 0.0f;

    if ( NULL == nrm ) nrm =  loc_nrm;
    nrm[kX] = nrm[kY] = nrm[kZ] = 0.0f;

    if ( NULL == pos || 0 == bits ) return 0;

    if ( NULL == pmesh || 0 == pmesh->info.tiles_count || 0 == pmesh->tmem.tile_count ) return 0;
    pinfo = &( pmesh->info );
    tlist = pmesh->tmem.tile_list;
    glist = pmesh->gmem.grid_list;

    if ( 0.0f == radius )
    {
        fx_min = fx_max = pos[kX];
        fy_min = fy_max = pos[kY];

        obj_area = 0.0f;
    }
    else
    {
        // make sure it is positive
        radius = ABS( radius );

        fx_min = pos[kX] - radius;
        fx_max = pos[kX] + radius;

        fy_min = pos[kY] - radius;
        fy_max = pos[kY] + radius;

        tx0 = ( int )pos[kX] / TILE_ISIZE;
        ty0 = ( int )pos[kY] / TILE_ISIZE;

        obj_area = ( fx_max - fx_min ) * ( fy_max - fy_min );
    }

    ix_min = floor( fx_min / TILE_SIZE );
    ix_max = floor( fx_max / TILE_SIZE );

    iy_min = floor( fy_min / TILE_SIZE );
    iy_max = floor( fy_max / TILE_SIZE );

    pass = 0;
    nrm[kX] = nrm[kY] = 0.0f;
    for ( iy = iy_min; iy <= iy_max; iy++ )
    {
        const float tile_area = TILE_SIZE * TILE_SIZE;
        float ty_min, ty_max;

        invalid = bfalse;

        ty_min = ( iy + 0 ) * TILE_SIZE;
        ty_max = ( iy + 1 ) * TILE_SIZE;

        if ( iy < 0 || iy >= pinfo->tiles_y )
        {
            pass    |= ( MPDFX_IMPASS | MPDFX_WALL );
            nrm[kY] += ( iy + 0.5f ) * TILE_SIZE - pos[kY];
            invalid = btrue;
        }

        for ( ix = ix_min; ix <= ix_max; ix++ )
        {
            float tx_min, tx_max;

            tx_min = ( ix + 0 ) * TILE_SIZE;
            tx_max = ( ix + 1 ) * TILE_SIZE;

            if ( ix < 0 || ix >= pinfo->tiles_x )
            {
                pass    |=  MPDFX_IMPASS | MPDFX_WALL;
                nrm[kX] += ( ix + 0.5f ) * TILE_SIZE - pos[kX];
                invalid = btrue;
            }

            if ( !invalid )
            {
                itile = mesh_get_tile_int( pmesh, ix, iy );
                if ( VALID_TILE( pmesh, itile ) )
                {
                    float area_ratio;
                    float ovl_x_min, ovl_x_max;
                    float ovl_y_min, ovl_y_max;

                    if ( 0.0f == radius )
                    {
                        area_ratio = 1.0f;
                        ovl_x_min = tx_min;
                        ovl_x_max = tx_max;

                        ovl_y_min = ty_min;
                        ovl_y_max = ty_max;
                    }
                    else
                    {
                        float min_area;

                        // determine the area overlap of the tile with the
                        // object's bounding box
                        ovl_x_min = MAX( fx_min, tx_min );
                        ovl_x_max = MIN( fx_max, tx_max );

                        ovl_y_min = MAX( fy_min, ty_min );
                        ovl_y_max = MIN( fy_max, ty_max );

                        min_area = MIN( tile_area, obj_area );

                        area_ratio = 0.0f;
                        if ( ovl_x_min <= ovl_x_max && ovl_y_min <= ovl_y_max )
                        {
                            if ( 0.0f == min_area )
                            {
                                area_ratio = 1.0f;
                            }
                            else
                            {
                                area_ratio  = ( ovl_x_max - ovl_x_min ) * ( ovl_y_max - ovl_y_min ) / min_area;
                            }
                        }
                    }

                    if ( HAS_SOME_BITS( glist[itile].fx, bits ) )
                    {
                        // hiting the mesh
                        nrm[kX] += (( ovl_x_max + ovl_x_min ) * 0.5f - pos[kX] ) * area_ratio;
                        nrm[kY] += (( ovl_y_max + ovl_y_min ) * 0.5f - pos[kY] ) * area_ratio;

                        // do not count it as a hit unless there is a 25% overlap
                        if ( area_ratio > 0.25f )
                        {
                            pass |= glist[itile].fx;
                        }
                    }
                    else
                    {
                        // not hitting the mesh
                        nrm[kX] -= (( ovl_x_max + ovl_x_min ) * 0.5f - pos[kX] ) * area_ratio;
                        nrm[kY] -= (( ovl_y_max + ovl_y_min ) * 0.5f - pos[kY] ) * area_ratio;
                    }
                }
            }
        }
    }

    dist2 = nrm[kX] * nrm[kX] + nrm[kY] * nrm[kY];
    if ( dist2 > 0.0f )
    {
        float dist = SQRT( dist2 );

        *pressure = dist;
        nrm[kX] /= -dist;
        nrm[kY] /= -dist;
    }

    return pass & bits;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float mesh_get_max_vertex_0( ego_mpd_t * pmesh, int tile_x, int tile_y )
{
    Uint32 itile;
    int type;
    Uint32 cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    if ( NULL == pmesh ) return 0.0f;

    itile = mesh_get_tile_int( pmesh, tile_x, tile_y );

    if ( INVALID_TILE == itile ) return 0.0f;

    type   = pmesh->tmem.tile_list[itile].type;
    vstart = pmesh->tmem.tile_list[itile].vrtstart;
    vcount = MIN( 4, pmesh->tmem.vert_count );

    ivrt = vstart;
    zmax = pmesh->tmem.plst[ivrt][ZZ];
    for ( ivrt++, cnt = 1; cnt < vcount; ivrt++, cnt++ )
    {
        zmax = MAX( zmax, pmesh->tmem.plst[ivrt][ZZ] );
    }

    return zmax;
}

//--------------------------------------------------------------------------------------------
float mesh_get_max_vertex_1( ego_mpd_t * pmesh, int tile_x, int tile_y, float xmin, float ymin, float xmax, float ymax )
{
    Uint32 itile;
    int type;
    int cnt;
    float zmax;
    size_t vcount, vstart, ivrt;

    int ix_off[4] = {1, 1, 0, 0};
    int iy_off[4] = {0, 1, 1, 0};

    if ( NULL == pmesh ) return 0.0f;

    itile = mesh_get_tile_int( pmesh, tile_x, tile_y );

    if ( INVALID_TILE == itile ) return 0.0f;

    type   = pmesh->tmem.tile_list[itile].type;
    vstart = pmesh->tmem.tile_list[itile].vrtstart;
    vcount = MIN( 4, pmesh->tmem.vert_count );

    zmax = -1e6;
    for ( ivrt = vstart, cnt = 0; cnt < vcount; ivrt++, cnt++ )
    {
        float fx, fy;
        GLXvector3f * pvert = pmesh->tmem.plst + ivrt;

        // we are evaluating the height based on the grid, not the actual vertex positions
        fx = ( tile_x + ix_off[cnt] ) * TILE_SIZE;
        fy = ( tile_y + iy_off[cnt] ) * TILE_SIZE;

        if ( fx >= xmin && fx <= xmax && fy >= ymin && fy <= ymax )
        {
            zmax = MAX( zmax, ( *pvert )[ZZ] );
        }
    }

    if ( -1e6 == zmax ) zmax = 0.0f;

    return zmax;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_init( ego_tile_info_t * ptr )
{
    if ( NULL == ptr ) return ptr;

    memset( ptr, 0, sizeof( *ptr ) );

    // set the non-zero, non-NULL, non-bfalse values
    ptr->fanoff             = btrue;
    ptr->inrenderlist_frame = -1;
    ptr->needs_lighting_update = btrue;

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_alloc()
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW( ego_tile_info_t );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_init( retval );
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_init_ary( ego_tile_info_t * ptr, size_t count )
{
    Uint32 cnt;

    if ( NULL == ptr ) return ptr;

    for ( cnt = 0; cnt < count; cnt++ )
    {
        ego_tile_info_init( ptr + cnt );
    }

    return ptr;
}

//--------------------------------------------------------------------------------------------
ego_tile_info_t * ego_tile_info_alloc_ary( size_t count )
{
    ego_tile_info_t * retval = NULL;

    retval = EGOBOO_NEW_ARY( ego_tile_info_t, count );
    if ( NULL == retval ) return NULL;

    return ego_tile_info_init_ary( retval, count );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_init_1( mpd_BSP_t * pbsp, ego_mpd_t * pmesh, int size_x, int size_y )
{
    // BB> Create a new BSP tree for the mesh.
    //     These parameters duplicate the max resolution of the old system.

    Uint32 i;
    int depth;
    BSP_node_t * pnode;

    if ( NULL == pmesh ) return bfalse;

    depth = ceil( log( 0.5f * MAX( size_x, size_y ) ) / log( 2.0f ) );

    // make a 2D BSP tree with "max depth" depth
    BSP_tree_ctor_1( &( pbsp->tree ), 2, depth );

    // allocate nodes for all of the tiles
    pbsp->nodes = EGOBOO_NEW_ARY( BSP_node_t, pmesh->gmem.grid_count );
    for ( i = 0; i < pmesh->gmem.grid_count; i++ )
    {
        pnode = pbsp->nodes + i;

        // let data type 1 stand for a tile, -1 is uninitialized
        BSP_node_ctor( pnode, pmesh->tmem.tile_list + i, 1 );
        pnode->index = i;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
mpd_BSP_t * mpd_BSP_dtor( mpd_BSP_t * pbsp )
{
    if ( NULL == pbsp ) return NULL;

    BSP_tree_clear( &( pbsp->tree ) );

    EGOBOO_DELETE( pbsp->nodes );
    pbsp->node_count = 0;
    pbsp->volume.mins[OCT_X] = pbsp->volume.maxs[OCT_X] = 0.0f;

    return pbsp;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_insert_node( mpd_BSP_t * pbsp, BSP_node_t * pnode, int depth, int address_x[], int address_y[] )
{
    int i;
    bool_t retval;
    Uint32 index;
    BSP_leaf_t * pleaf, * pnew_leaf;
    BSP_tree_t * ptree = &( pbsp->tree );

    retval = bfalse;
    if ( depth < 0 )
    {
        // this can only happen if the node does not intersect the BSP bounding box
        pnode->next = ptree->infinite;
        ptree->infinite = pnode;
        retval = btrue;
    }
    else if ( 0 == depth )
    {
        // this can only happen if the tile should be in the root node list
        pnode->next = ptree->root->nodes;
        ptree->root->nodes = pnode;
        retval = btrue;
    }
    else
    {
        // insert the node into the tree at this point
        pleaf = ptree->root;
        for ( i = 0; i < depth; i++ )
        {
            index = (( Uint32 )address_x[i] ) + ((( Uint32 )address_y[i] ) << 1 );

            pnew_leaf = BSP_tree_ensure_leaf( ptree, pleaf, index );
            if ( NULL == pnew_leaf ) break;

            pleaf = pnew_leaf;
        };

        // insert the node in this leaf
        retval = BSP_tree_insert( ptree, pleaf, pnode, -1 );
    };

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_fill( mpd_BSP_t * pbsp, mpd_mem_t * mem )
{
    Uint32 tile;
    int * address_x, * address_y;

    BSP_tree_t * ptree = &( pbsp->tree );

    address_x = EGOBOO_NEW_ARY( int, ptree->depth );
    address_y = EGOBOO_NEW_ARY( int, ptree->depth );

    for ( tile = 0; tile < mem->tile_count; tile++ )
    {
        mpd_BSP_insert_tile_node( pbsp, pbsp->nodes + tile, ptree->depth, address_x, address_y );
    }

    EGOBOO_DELETE( address_x );
    EGOBOO_DELETE( address_y );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_BSP_collide_nodes( BSP_node_t node_lst[], oct_bb_t * pvobj, int colst[], size_t colist_size, int * pcolst_index )
{
    // BB > check for collisions with the given node list

    BSP_node_t      * pnode;
    ego_tile_info_t * ptile;
    oct_bb_t         int_ov;

    if ( NULL == node_lst || NULL == pvobj || NULL == pcolst_index ) return bfalse;
    if ( *pcolst_index >= colist_size || 0 == colist_size ) return bfalse;

    for ( pnode = node_lst; NULL != pnode; pnode = pnode->next )
    {
        assert( pnode->data_type >= 0 );

        ptile = ( ego_tile_info_t * )pnode->data;
        if ( oct_bb_empty( ptile->oct ) ) continue;

        if ( oct_bb_intersection( *pvobj, ptile->oct, &int_ov ) )
        {
            // we have a possible intersection
            colst[*pcolst_index] = pnode->index;
            ( *pcolst_index )++;
            mesh_BSP_collide_nodes_count++;

            if (( *pcolst_index ) >= colist_size )
            {
                // too many nodes. break out of the search.
                mesh_BSP_collide_leaf_lvl--;
                return bfalse;
            };
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_BSP_collide_leaf( BSP_leaf_t * pleaf, oct_bb_t * pvleaf, oct_bb_t * pvobj, int colst[], size_t colist_size, int * pcolst_index )
{
    // BB > Recursively search the BSP tree for collisions with the pvobj
    //      Return bfalse if we need to break out of the recursive search for any reson.

    Uint32 i;
    oct_bb_t int_ov, tmp_ov;
    float    x_mid, y_mid;
    int      address_x, address_y;

    if ( NULL == pvobj  || oct_bb_empty( *pvobj ) ) return bfalse;
    if ( NULL == pvleaf || oct_bb_empty( *pvleaf ) ) return bfalse;

    // return if the object does not intersect the leaf
    if ( !oct_bb_intersection( *pvobj, *pvleaf, &int_ov ) )
    {
        return bfalse;
    }

    mesh_BSP_collide_leaf_lvl++;

    // check the nodes
    if ( !mesh_BSP_collide_nodes( pleaf->nodes, pvobj, colst, colist_size, pcolst_index ) )
    {
        return bfalse;
    }

    // check for collisions with any of the children
    x_mid = ( pvleaf->maxs[OCT_X] + pvleaf->mins[OCT_X] ) * 0.5f;
    y_mid = ( pvleaf->maxs[OCT_Y] + pvleaf->mins[OCT_Y] ) * 0.5f;
    for ( i = 0; i < pleaf->child_count; i++ )
    {
        // scan all the children
        if ( NULL == pleaf->children[i] ) continue;

        // create the volume of this node
        address_x = i & ( 1 << 0 );
        address_y = i & ( 1 << 1 );

        tmp_ov = *( pvleaf );

        if ( 0 == address_x )
        {
            tmp_ov.mins[OCT_X] = x_mid;
        }
        else
        {
            tmp_ov.maxs[OCT_X] = x_mid;
        }

        if ( 0 == address_y )
        {
            tmp_ov.mins[OCT_Y] = y_mid;
        }
        else
        {
            tmp_ov.maxs[OCT_Y] = y_mid;
        }

        if ( oct_bb_intersection( *pvobj, tmp_ov, &int_ov ) )
        {
            // potential interaction with the child. go recursive!
            bool_t   ret = mesh_BSP_collide_leaf( pleaf->children[i], &( tmp_ov ), pvobj, colst, colist_size, pcolst_index );
            if ( !ret ) { mesh_BSP_collide_leaf_lvl--; return ret; }
        }
    }

    mesh_BSP_collide_leaf_lvl--;
    return btrue;
}

//--------------------------------------------------------------------------------------------
int mpd_BSP_collide( mpd_BSP_t * pbsp, oct_bb_t * pvobj, int colst[], size_t colist_size )
{
    // BB > fill the collision list with references to tiles that the object volume may overlap.
    //      Return the number of collisions found.

    int collision_count;

    mesh_BSP_collide_leaf_lvl   = 0;
    mesh_BSP_collide_nodes_count = 0;

    if ( NULL == pbsp || NULL == pvobj || 0 == colist_size ) return 0;

    // do the actual collisions
    collision_count = 0;

    // collide with any "infinite" nodes
    mesh_BSP_collide_nodes( pbsp->tree.infinite, pvobj, colst, colist_size, &collision_count );

    // collise with the rest of the tree
    mesh_BSP_collide_leaf( pbsp->tree.root, &( pbsp->volume ), pvobj, colst, colist_size, &collision_count );

    return collision_count;
}

//--------------------------------------------------------------------------------------------
bool_t mpd_BSP_insert_tile_node( mpd_BSP_t * pbsp, BSP_node_t * pnode, size_t depth, int address_x[], int address_y[] )
{
    // BB> insert a tile wrapped in a BSP_node_t into the BSP_tree_t

    int i;
    ego_tile_info_t * ptile = ( ego_tile_info_t * )( pnode->data );

    i = BSP_find_address_2d( pbsp->volume, ptile->oct, depth, address_x, address_y );

    return mpd_BSP_insert_node( pbsp, pnode, i, address_x, address_y );
}