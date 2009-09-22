#pragma once

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

/* Egoboo - enchant.h
 * data and functions for reading and writing enchant.txt files
 */

#include "egoboo_typedef.h"

// missile treatments
#define MISNORMAL               0                  // Treat missiles normally
#define MISDEFLECT              1                  // Deflect incoming missiles
#define MISREFLECT              2                  // Reflect them back!

// Different set values for enchants
typedef enum enchant_set
{
    SETDAMAGETYPE = 0,      // Type of damage dealt
    SETNUMBEROFJUMPS,       // Max number of jumps
    SETLIFEBARCOLOR,        // Color of life bar
    SETMANABARCOLOR,        // Color of mana bar
    SETSLASHMODIFIER,      // Damage modifiers
    SETCRUSHMODIFIER,
    SETPOKEMODIFIER,
    SETHOLYMODIFIER,
    SETEVILMODIFIER,
    SETFIREMODIFIER,
    SETICEMODIFIER,
    SETZAPMODIFIER,
    SETFLASHINGAND,             // Flash rate
    SETLIGHTBLEND,              // Transparency
    SETALPHABLEND,              // Alpha
    SETSHEEN,                   // Shinyness
    SETFLYTOHEIGHT,             // Fly to this height
    SETWALKONWATER,             // Walk on water?
    SETCANSEEINVISIBLE,         // Can it see invisible?
    SETMISSILETREATMENT,        // How to treat missiles
    SETCOSTFOREACHMISSILE,      // Cost for each missile treat
    SETMORPH,                   // Morph character?
    SETCHANNEL,                 // Can channel life as mana?
    MAX_ENCHANT_SET
} enum_enchant_set;

//--------------------------------------------------------------------------------------------
typedef enum enchant_add
{
    ADDJUMPPOWER = 0,
    ADDBUMPDAMPEN,
    ADDBOUNCINESS,
    ADDDAMAGE,
    ADDSIZE,
    ADDACCEL,
    ADDRED,                        // Red shift
    ADDGRN,                        // Green shift
    ADDBLU,                        // Blue shift
    ADDDEFENSE,                    // Defence adjustments
    ADDMANA,
    ADDLIFE,
    ADDSTRENGTH,
    ADDWISDOM,
    ADDINTELLIGENCE,
    ADDDEXTERITY,
    MAX_ENCHANT_ADD
} enum_enchant_add;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_eve
{
    EGO_PROFILE_STUFF;

    bool_t  override;                    // Override other enchants?
    bool_t  removeoverridden;            // Remove other enchants?
    bool_t  retarget;                    // Pick a weapon?
    bool_t  killonend;                   // Kill the target on end?
    bool_t  poofonend;                   // Spawn a poof on end?
    bool_t  endifcantpay;                // End on out of mana
    bool_t  stayifnoowner;               // Stay if owner has died?
    Sint16  time;                        // Time in seconds
    Sint32  endmessage;                  // Message for end -1 for none
    Uint8   dontdamagetype;              // Don't work if ...
    Uint8   onlydamagetype;              // Only work if ...
    IDSZ    removedbyidsz;               // By particle or [NONE]
    Uint16  contspawntime;               // Spawn timer
    Uint8   contspawnamount;             // Spawn amount
    Uint16  contspawnfacingadd;          // Spawn in circle
    Uint16  contspawnpip;                // Spawn type ( local )
    Sint16  endsoundindex;               // Sound on end (-1 for none)
    Uint8   overlay;                     // Spawn an overlay?
    bool_t  stayifdead;                  // Stay if target has died?

    // Boost values
    Sint16  ownermana;
    Sint16  ownerlife;
    Sint16  targetmana;
    Sint16  targetlife;

    // the enchant values
    bool_t  setyesno[MAX_ENCHANT_SET];    // Set this value?
    Uint8   setvalue[MAX_ENCHANT_SET];    // Value to use
    Sint32  addvalue[MAX_ENCHANT_ADD];    // The values to add

    // other values that are enchanted
    Uint16  seekurse;                    // Allow target to see kurses
};
typedef struct s_eve eve_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
eve_t *  load_one_enchant_file( const char* szLoadName, eve_t * peve );
bool_t   save_one_enchant_file( const char* szLoadName, eve_t * peve );