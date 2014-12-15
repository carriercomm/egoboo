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

/// @file game/enchant.inl

#include "game/EncList.inl"
#include "game/ChrList.inl"

#include "game/char.inl"
#include "game/profile.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARARIONS
//--------------------------------------------------------------------------------------------
static INLINE PRO_REF   enc_get_ipro( const ENC_REF ienc );
static INLINE pro_t   * enc_get_ppro( const ENC_REF ienc );

static INLINE CHR_REF   enc_get_iowner( const ENC_REF ienc );
static INLINE chr_t   * enc_get_powner( const ENC_REF ienc );

static INLINE EVE_REF   enc_get_ieve( const ENC_REF ienc );
static INLINE eve_t   * enc_get_peve( const ENC_REF ienc );

static INLINE IDSZ      enc_get_idszremove( const ENC_REF ienc );
static INLINE ego_bool    enc_is_removed( const ENC_REF ienc, const PRO_REF test_profile );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
CHR_REF enc_get_iowner( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_CHR_REF;
    penc = EncList_get_ptr( ienc );

    if ( !INGAME_CHR( penc->owner_ref ) ) return INVALID_CHR_REF;

    return penc->owner_ref;
}

//--------------------------------------------------------------------------------------------
chr_t * enc_get_powner( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList_get_ptr( ienc );

    if ( !INGAME_CHR( penc->owner_ref ) ) return NULL;

    return ChrList_get_ptr( penc->owner_ref );
}

//--------------------------------------------------------------------------------------------
EVE_REF enc_get_ieve( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_EVE_REF;
    penc = EncList_get_ptr( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return INVALID_EVE_REF;

    return penc->eve_ref;
}

//--------------------------------------------------------------------------------------------
eve_t * enc_get_peve( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList_get_ptr( ienc );

    if ( !LOADED_EVE( penc->eve_ref ) ) return NULL;

    return EveStack_get_ptr( penc->eve_ref );
}

//--------------------------------------------------------------------------------------------
PRO_REF  enc_get_ipro( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return INVALID_PRO_REF;
    penc = EncList_get_ptr( ienc );

    if ( !LOADED_PRO( penc->profile_ref ) ) return INVALID_PRO_REF;

    return penc->profile_ref;
}

//--------------------------------------------------------------------------------------------
pro_t * enc_get_ppro( const ENC_REF ienc )
{
    enc_t * penc;

    if ( !DEFINED_ENC( ienc ) ) return NULL;
    penc = EncList_get_ptr( ienc );

    if ( !LOADED_PRO( penc->profile_ref ) ) return NULL;

    return ProList_get_ptr( penc->profile_ref );
}

//--------------------------------------------------------------------------------------------
IDSZ enc_get_idszremove( const ENC_REF ienc )
{
    eve_t * peve = enc_get_peve( ienc );
    if ( NULL == peve ) return IDSZ_NONE;

    return peve->removedbyidsz;
}

//--------------------------------------------------------------------------------------------
ego_bool enc_is_removed( const ENC_REF ienc, const PRO_REF test_profile )
{
    IDSZ idsz_remove;

    if ( !INGAME_ENC( ienc ) ) return ego_false;
    idsz_remove = enc_get_idszremove( ienc );

    // if nothing can remove it, just go on with your business
    if ( IDSZ_NONE == idsz_remove ) return ego_false;

    // check vs. every IDSZ that could have something to do with cancelling the enchant
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_TYPE ) ) return ego_true;
    if ( idsz_remove == pro_get_idsz( test_profile, IDSZ_PARENT ) ) return ego_true;

    return ego_false;
}

//--------------------------------------------------------------------------------------------

#define _enchant_inl
