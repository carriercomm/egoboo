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

///
/// @file
/// @brief Egoboo Object Profile
/// @details Definitions for encapsulating all of the data that can be used for creating any kind of Egoboo object.


#include "egoboo_types.h"
#include "egoboo_utility.h"
#include "egoboo_math.h"
#include "Physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define OBJLST_COUNT 1024

struct sGame;
struct sChr;
struct sPrt;

struct Mix_Chunk;

#define MAXSECTION                      4            ///< T-wi-n-k...  Most of 4 sections
#define MAXWAVE         16                             ///< Up to 16 waves per model


typedef enum e_prtpip
{
  PRTPIP_COIN_001 = 0,                  ///< Coins are the first particles loaded
  PRTPIP_COIN_005,                      //
  PRTPIP_COIN_025,                      //
  PRTPIP_COIN_100,                      //
  PRTPIP_WEATHER_1,                     ///< Weather particles
  PRTPIP_WEATHER_2,                     ///< Weather particle finish
  PRTPIP_SPLASH,                        ///< Water effects are next
  PRTPIP_RIPPLE,                        //
  PRTPIP_DEFEND,                        ///< Defend particle
  PRTPIP_PEROBJECT_COUNT                //
} PRTPIP;

//--------------------------------------------------------------------------------------------
struct sProfile
{
  egoboo_key_t      ekey;
  bool_t          Active;                          ///< is it loaded?

  // debug info
  STRING          name;                          ///< Model name

  // message stuff
  Uint16          msg_start;                      ///< The first message

  // skin stuff
  Uint16          skins;                         ///< Number of skins
  Uint16          skinstart;                     ///< Starting skin of model

  // sound stuff
  struct Mix_Chunk *     wavelist[MAXWAVE];             //sounds in a object

  // naming stuff
  Uint16          sectionsize[MAXSECTION];       ///< Number of choices, 0
  Uint16          sectionstart[MAXSECTION];      //

  EVE_REF         eve;
  CAP_REF         cap;
  MAD_REF         mad;
  AI_REF          ai;                              ///< AI for this model
  PIP_REF         prtpip[PRTPIP_PEROBJECT_COUNT];  ///< Local particles

};
typedef struct sProfile Profile_t;

typedef struct sProfile Obj_t;

#ifdef __cplusplus
  typedef TList<sProfile, OBJLST_COUNT>  ObjList_t;
  typedef TPList<sProfile, OBJLST_COUNT> PObj_t;
#else
  typedef Obj_t   ObjList_t[OBJLST_COUNT];
  typedef Obj_t * PObj_t;
#endif

Profile_t * CProfile_new(Profile_t *);
bool_t      CProfile_delete(Profile_t *);
Profile_t * CProfile_renew(Profile_t *);


bool_t  ObjList_new   ( struct sGame * gs );
bool_t  ObjList_delete( struct sGame * gs );
bool_t  ObjList_renew ( struct sGame * gs );
OBJ_REF ObjList_get_free(  struct sGame * gs, OBJ_REF request );

       Obj_t   * ObjList_getPObj(struct sGame * gs, OBJ_REF iobj);
struct sEve * ObjList_getPEve(struct sGame * gs, OBJ_REF iobj);
struct sCap * ObjList_getPCap(struct sGame * gs, OBJ_REF iobj);
struct sMad * ObjList_getPMad(struct sGame * gs, OBJ_REF iobj);
struct sPip * ObjList_getPPip(struct sGame * gs, OBJ_REF iobj, int i);

EVE_REF ObjList_getREve(struct sGame * gs, OBJ_REF obj);
CAP_REF ObjList_getRCap(struct sGame * gs, OBJ_REF obj);
MAD_REF ObjList_getRMad(struct sGame * gs, OBJ_REF obj);
MAD_REF ObjList_getRAi (struct sGame * gs, OBJ_REF obj);
PIP_REF ObjList_getRPip(struct sGame * gs, OBJ_REF iobj, int i);
void    ObjList_log_used( struct sGame * gs, char *savename );

#define VALID_OBJ_RANGE(XX)   ( /*(((XX)>=0) && */ ((XX)<OBJLST_COUNT) )
#define VALID_OBJ(LST, XX)    ( VALID_OBJ_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_OBJ(LST, XX) ( VALID_OBJ(LST, XX) ? (XX) : (INVALID_OBJ) )


//--------------------------------------------------------------------------------------------
struct sBData
{
  bool_t valid;       ///< is this data valid?

  Uint8  shadow;      ///< Size of shadow
  Uint8  size;        ///< Size of bumpers
  Uint8  sizebig;     ///< For octagonal bumpers
  Uint8  height;      ///< Distance from head to toe

  bool_t calc_is_platform;
  bool_t calc_is_mount;

  float  calc_size;
  float  calc_size_big;
  float  calc_height;

  CVolume_t        cv;
  CVolume_Tree_t * cv_tree;
  vect3   mids_hi, mids_lo;
};
typedef struct sBData BData_t;

//--------------------------------------------------------------------------------------------
typedef enum e_Team
{
  TEAM_EVIL            = 'E' -'A',                      ///< 'E' == Evil
  TEAM_GOOD            = 'G' -'A',                      ///< 'G' == Good
  TEAM_NULL            = 'N' -'A',                      ///< 'N' == Null or Neutral
  TEAM_ZIPPY           = 'Z' -'A',                      ///< 'Z' == Zippy?
  TEAM_DAMAGE,                                          ///< For damage tiles
  TEAM_COUNT                                            ///< Teams A-Z, +1 more for damage tiles
} TEAM;

struct sTeam
{
  egoboo_key_t ekey;
  bool_t  hatesteam[TEAM_COUNT];  ///< Don't damage allies...
  Uint16  morale;                 ///< Number of characters on team
  CHR_REF leader;                 ///< The leader of the team
  CHR_REF sissy;                  ///< Whoever called for help last
};
typedef struct sTeam Team_t;

#ifdef __cplusplus
  typedef TList<sTeam, TEAM_COUNT> TeamList_t;
  typedef TPList<sTeam, TEAM_COUNT> PTeam;
#else
  typedef Team_t TeamList_t[TEAM_COUNT];
  typedef Team_t * PTeam;
#endif

Team_t * Team_new(Team_t *pteam);
bool_t   Team_delete(Team_t *pteam);
Team_t * Team_renew(Team_t *pteam);

#define VALID_TEAM_RANGE(XX) ( /*(((XX)>=0) && */ ((XX)<TEAM_COUNT) )

//--------------------------------------------------------------------------------------------
// struct for blended vertex data
struct sVData_Blended
{
  Uint32  frame0;
  Uint32  frame1;
  Sint32  vrtmin;
  Sint32  vrtmax;
  float   lerp;
  bool_t  needs_lighting;

  // Storage for blended vertices
  vect3 *Vertices;
  vect3 *Normals;
  vect4 *Colors;
  vect2 *Texture;
  float *Ambient;      ///< Lighting hack ( Ooze )
};
typedef struct sVData_Blended VData_Blended_t;

//--------------------------------------------------------------------------------------------
enum e_damage
{
  DAMAGE_SLASH   = 0,                          //
  DAMAGE_CRUSH,                                //
  DAMAGE_POKE,                                 //
  DAMAGE_HOLY,                                 ///< (Most invert Holy damage )
  DAMAGE_EVIL,                                 //
  DAMAGE_FIRE,                                 //
  DAMAGE_ICE,                                  //
  DAMAGE_ZAP,                                  //
  MAXDAMAGETYPE,                               ///< Damage types
  DAMAGE_NULL     = 255                        //
};
typedef enum e_damage DAMAGE;

#define DAMAGE_SHIFT         3                        ///< 000000xx Resistance ( 1 is common )
#define DAMAGE_INVERT       (1 << 2)                 // 00000x00 Makes damage heal
#define DAMAGE_CHARGE       (1 << 3)                 // 0000x000 Converts damage to mana
#define DAMAGE_MANA         (1 << 4)                 // 000x0000 Makes damage deal to mana

//--------------------------------------------------------------------------------------------
struct s_tile_damage
{
  PIP_REF  parttype;
  short  partand;
  Sint8  sound;
  int    amount; ///<  EQ( 256 );                           ///< Amount of damage
  DAMAGE type; ///<  EQ( DAMAGE_FIRE );                      ///< Type of damage
};
typedef struct s_tile_damage TILE_DAMAGE;

bool_t tile_damage_reset(TILE_DAMAGE * t);

#define DELAY_TILESOUND 16
#define TILEREAFFIRMAND  3


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


void   disaffirm_attached_particles( struct sGame * gs, CHR_REF character );
Uint16 number_of_attached_particles( struct sGame * gs, CHR_REF character );
void   reaffirm_attached_particles( struct sGame * gs, CHR_REF character );

void switch_team( struct sGame * gs, CHR_REF character, TEAM_REF team );
int  restock_ammo( struct sGame * gs, CHR_REF character, IDSZ idsz );
void issue_clean( struct sGame * gs, CHR_REF character );

int load_one_object( struct sGame * gs, int skin_count, const char * szObjectpath, char* szObjectname, OBJ_REF slot_override );

void obj_clear_pips( struct sGame * gs );
