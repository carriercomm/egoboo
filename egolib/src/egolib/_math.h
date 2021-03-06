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

/// @file egolib/_math.h
/// @details The name's pretty self explanatory, doncha think?

#pragma once

#include "egolib/typedef.h"
#include "egolib/Math/_Include.hpp"
#include "egolib/Log/_Include.hpp"

//--------------------------------------------------------------------------------------------
// IEEE 32-BIT FLOATING POINT NUMBER FUNCTIONS
//--------------------------------------------------------------------------------------------


#if defined(TEST_NAN_RESULT)
#    define LOG_NAN(XX)      if( ieee32_bad(XX) ) log_error( "**** A math operation resulted in an invalid result (NAN) ****\n    (\"%s\" - %d)\n", __FILE__, __LINE__ );
#else
#    define LOG_NAN(XX)
#endif

#define FACE_RANDOM  Random::next<FACING_T>(std::numeric_limits<FACING_T>::max())

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// basic constants

/**
 * @brief \f$\frac{1}{255}\f$.
 * @return \f$\frac{1}{255}\f$ as a value of type @a T.
 * @remark Specializations for single- and double-precision floating-point types are provided.
 */
template <typename T>
T INV_FF();

template <>
inline float INV_FF<float>() {
    return 0.003921568627450980392156862745098f;
}

template <>
inline double INV_FF<double>() {
    return 0.000015259021896696421759365224689097;
}


/**
* @brief \f$\frac{1}{255^2}\f$.
* @return \f$\frac{1}{255^2}\f$ as a value of type @a T.
* @remark Specializations for single- and double-precision floating-point types are provided.
 */
template <typename T>
T INV_FFFF();

template <>
inline float INV_FFFF<float>() {
    return 0.000015259021896696421759365224689097f;
}

template <>
inline double INV_FFFF<double>() {
    return 0.000015259021896696421759365224689097;
}


/**
 * @brief
 *  Convert an angle from radians to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in radians
 * @return
 *  the angle in "facing"
 */
inline FACING_T RAD_TO_FACING(float x) {
	// UINT16_MAX / (2 * PI).
	return Ego::Math::clipBits<16>(x * (std::numeric_limits<FACING_T>::max() / Ego::Math::twoPi<float>()));
}

/**
 * @brief
 *  Convert an angle "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$) to radians.
 * @param x
 *  the angle in facing
 * @return
 *  the angle in radians
 */
inline float FACING_TO_RAD(FACING_T facing) {
	// (2 * PI) / UINT16_MAX
	return facing * (Ego::Math::twoPi<float>() / std::numeric_limits<FACING_T>::max());
}

/**
 * @brief
 *  Convert an angle from turns to "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$).
 * @param x
 *  the angle in turns
 * @return
 *  the angle in "facing"
 */
inline FACING_T TurnsToFacing(float x) {
	return Ego::Math::clipBits<16>((int)(x * (float)0x00010000));
}

/**
 * @brief
 *  Convert an angle from "facing" (some custom Egoboo unit in the interval \f$[0,2^16-1]\f$)) to turns.
 * @param x
 *	the angle in "facing"
 * @return
 *  the angle in turns
 */
inline float FacingToTurns(FACING_T x) {
	return ((float)Ego::Math::clipBits<16>(x) / (float)0x00010000);
}

//--------------------------------------------------------------------------------------------
// the lookup tables for sine and cosine

#define TRIG_TABLE_BITS   14
#define TRIG_TABLE_SIZE   (1<<TRIG_TABLE_BITS)
#define TRIG_TABLE_MASK   (TRIG_TABLE_SIZE-1)
#define TRIG_TABLE_OFFSET (TRIG_TABLE_SIZE>>2)

#if defined(__cplusplus)
extern "C"
{
#endif

/// @note - Aaron uses two terms without much attention to their meaning
///         I think that we should use "face" or "facing" to mean the fill 16-bit value
///         and use "turn" to be the TRIG_TABLE_BITS-bit value

    extern float turntosin[TRIG_TABLE_SIZE];           ///< Convert TURN_T == FACING_T>>2...  to sine
    extern float turntocos[TRIG_TABLE_SIZE];           ///< Convert TURN_T == FACING_T>>2...  to cosine

/// pre defined directions
static constexpr uint16_t FACE_WEST =  0x0000; ///< Character facings
static constexpr uint16_t FACE_NORTH = 0x4000;
static constexpr uint16_t FACE_EAST =  0x8000;
static constexpr uint16_t FACE_SOUTH = 0xC000;

//Directional aliases
static constexpr uint16_t ATK_FRONT =  FACE_WEST;
static constexpr uint16_t ATK_RIGHT =  FACE_NORTH;
static constexpr uint16_t ATK_BEHIND = FACE_EAST;
static constexpr uint16_t ATK_LEFT =   FACE_SOUTH;

inline TURN_T TO_TURN(const FACING_T &facing)
{
    return static_cast<TURN_T>( (static_cast<FACING_T>(facing) >> 2) & TRIG_TABLE_MASK);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if !defined(SGN)
#    define SGN(X)  LAMBDA( 0 == (X), 0, LAMBDA( (X) > 0, 1, -1) )
#endif

#if !defined(SQR)
#    define SQR(A) ((A)*(A))
#endif

//--------------------------------------------------------------------------------------------
// FAST CONVERSIONS


#if !defined(INV_0100)
#   define INV_0100            0.00390625f
#endif


#define FF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FF<float>() )

#define FFFF_TO_FLOAT( V1 )  ( (float)(V1) * INV_FFFF<float>() )
#define FLOAT_TO_FFFF( V1 )  ( (int)((V1) * 0xFFFF) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// prototypes of other math functions

    void make_turntosin( void );

// conversion functions
    FACING_T vec_to_facing( const float dx, const float dy );
    void     facing_to_vec( const FACING_T facing, float * dx, float * dy );

// rotation functions
    int terp_dir( const FACING_T majordir, const FACING_T minordir, const int weight );

// limiting functions
    void getadd_int( const int min, const int value, const int max, int* valuetoadd );
    void getadd_flt( const float min, const float value, const float max, float* valuetoadd );

// random functions
    int generate_irand_pair( const IPair num );
    int generate_irand_range( const FRange num );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}

#endif
