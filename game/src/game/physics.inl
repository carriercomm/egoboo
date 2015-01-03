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

/// @file game/physics.inl
/// @details Put all the functions that need to be fast in this file
///
/// @note
/// The test functions are designed to detect an interaction with the "least possible" computation.
/// Don't spoil the optimization by calling a test_interaction* function and then a get_depth* function
///
/// The numbers attached to these functions signify the level of precision that is used in calculating the
/// collision. The smallest number, 0, indicates the least precise collision and is equivalent to the "old" Egoboo method.
///
/// The _close_ keyword indicates that you are checking for something like whether a character is able to
/// stand on a platform or something where it will tend to fall off if it starts to step off the edge.
///
/// Use the test_platform flag if you want to test whether the objects are close enough for some platform interaction.
/// You could determine whether this should be set to true by determining whether either of the objects was a platform
/// and whether the other object could use the platform.
///
/// If you definitely are going to need the depth info, make sure to use the get_depth* functions with the break_out
/// flag set to false. Setting break_out to true will make the function faster in the case that there is no collision,
/// but it will leave some of the "depth vector" uncalculated, which might leave it with uninitialized data.

#pragma once

#include "game/physics.h"
#include "egolib/bbox.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

static INLINE apos_t * apos_self_clear( apos_t * val );

static INLINE bool test_interaction_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform );
static INLINE bool test_interaction_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform );
static INLINE bool test_interaction_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, int test_platform );
static INLINE bool test_interaction_close_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform );
static INLINE bool test_interaction_close_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform );
static INLINE bool test_interaction_close_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, int test_platform );

static INLINE bool get_depth_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth );
static INLINE bool get_depth_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth );
static INLINE bool get_depth_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth );
static INLINE bool get_depth_close_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth );
static INLINE bool get_depth_close_1( const oct_bb_t * cv_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth );
static INLINE bool get_depth_close_2( const oct_bb_t * cv_a, const oct_bb_t * cv_b, bool break_out, oct_vec_t depth );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_close_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_close_2( &cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_2( &cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_close_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_close_2( cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return test_interaction_2( cv_a, pos_a, &cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_close_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    float depth;
    oct_vec_t oa, ob;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = std::min(( ob[cnt] + cv_b->maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b->mins[cnt] ) );
        float ftmp2 = std::min(( oa[cnt] + cv_a->maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a->mins[cnt] ) );
        depth = std::max( ftmp1, ftmp2 );
        if ( depth <= 0.0f ) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min( cv_b->maxs[OCT_Z] + ob[OCT_Z], cv_a->maxs[OCT_Z] + oa[OCT_Z] ) -
            std::max( cv_b->mins[OCT_Z] + ob[OCT_Z], cv_a->mins[OCT_Z] + oa[OCT_Z] );

    return TO_C_BOOL( test_platform ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f ) );
}

//--------------------------------------------------------------------------------------------
static INLINE bool test_interaction_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, int test_platform )
{
    /// @author BB
    /// @details Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    float depth;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        depth  = std::min( cv_b->maxs[cnt] + ob[cnt], cv_a->maxs[cnt] + oa[cnt] ) -
                 std::max( cv_b->mins[cnt] + ob[cnt], cv_a->mins[cnt] + oa[cnt] );

        if ( depth <= 0.0f ) return false;
    }

    // treat the z coordinate the same as always
    depth = std::min( cv_b->maxs[OCT_Z] + ob[OCT_Z], cv_a->maxs[OCT_Z] + oa[OCT_Z] ) -
            std::max( cv_b->mins[OCT_Z] + ob[OCT_Z], cv_a->mins[OCT_Z] + oa[OCT_Z] );

    return TO_C_BOOL(( 0 != test_platform ) ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_close_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    // shift the bumpers
    oct_bb_self_add_fvec3( &cv_a, pos_a );
    oct_bb_self_add_fvec3( &cv_b, pos_b );

    return get_depth_close_2( &cv_a, &cv_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_0( bumper_t bump_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_a, cv_b;

    // convert the bumpers to the correct format
    oct_bb_set_bumper( &cv_a, bump_a );
    oct_bb_set_bumper( &cv_b, bump_b );

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return get_depth_2( &cv_a, pos_a, &cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_close_1( const oct_bb_t * cv_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    // shift the bumper
    oct_bb_self_add_fvec3( &cv_b, pos_b );

    return get_depth_close_2( cv_a, &cv_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_1( const oct_bb_t * cv_a, const fvec3_base_t pos_a, bumper_t bump_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    oct_bb_t cv_b;

    // convert the bumper to the correct format
    oct_bb_set_bumper( &cv_b, bump_b );

    return get_depth_2( cv_a, pos_a, &cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_close_2( const oct_bb_t * cv_a, const oct_bb_t * cv_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    bool valid;
    float ftmp1, ftmp2;
    float opos_a, opos_b;

    if ( NULL == depth ) return false;

    if ( NULL == cv_a || NULL == cv_b ) return false;

    // calculate the depth
    valid = true;
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        // get positions from the bounding volumes
        opos_a = ( cv_a->mins[cnt] + cv_a->maxs[cnt] ) * 0.5f;
        opos_b = ( cv_b->mins[cnt] + cv_b->maxs[cnt] ) * 0.5f;

        // measue the depth
        ftmp1 = std::min( cv_b->maxs[cnt] - opos_a, opos_a - cv_b->mins[cnt] );
        ftmp2 = std::min( cv_a->maxs[cnt] - opos_b, opos_b - cv_a->mins[cnt] );
        depth[cnt] = std::max( ftmp1, ftmp2 );

        if ( depth[cnt] <= 0.0f )
        {
            valid = false;
            if ( break_out ) return false;
        }
    }

    // treat the z coordinate the same as always
    depth[OCT_Z]  = std::min( cv_b->maxs[OCT_Z], cv_a->maxs[OCT_Z] ) -
                    std::max( cv_b->mins[OCT_Z], cv_a->mins[OCT_Z] );

    if ( depth[OCT_Z] <= 0.0f )
    {
        valid = false;
        if ( break_out ) return false;
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
static INLINE bool get_depth_2( const oct_bb_t * cv_a, const fvec3_base_t pos_a, const oct_bb_t * cv_b, const fvec3_base_t pos_b, bool break_out, oct_vec_t depth )
{
    /// @author BB
    /// @details Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    oct_vec_t oa, ob;
    bool valid;

    if ( NULL == cv_a || NULL == pos_a || NULL == cv_b || NULL == pos_b ) return false;

    if ( NULL == depth ) return false;

    // translate the positions to oct_vecs
    oct_vec_ctor( oa, pos_a );
    oct_vec_ctor( ob, pos_b );

    // calculate the depth
    valid = true;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        depth[cnt]  = std::min( cv_b->maxs[cnt] + ob[cnt], cv_a->maxs[cnt] + oa[cnt] ) -
                      std::max( cv_b->mins[cnt] + ob[cnt], cv_a->mins[cnt] + oa[cnt] );

        if ( depth[cnt] <= 0.0f )
        {
            valid = false;
            if ( break_out ) return false;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
static INLINE apos_t * apos_self_clear( apos_t * val )
{
    if ( NULL == val ) return val;

    BLANK_STRUCT_PTR( val )

    return val;
}