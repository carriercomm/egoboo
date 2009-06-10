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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egoboo_strutil.c
 * String manipulation functions.  Not currently in use.
 */

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void str_trim( char *pStr )
{
    /// @details ZZ> str_trim remove all space and tabs in the beginning and at the _dtor of the string

    Sint32 DebPos = 0, EndPos = 0, CurPos = 0;

    if ( NULL == pStr  )
    {
        return;
    }
    // look for the first character in string
    DebPos = 0;
    while ( isspace( pStr[DebPos] ) && pStr[DebPos] != 0 )
    {
        DebPos++;
    }

    // look for the last character in string
    CurPos = DebPos;
    while ( pStr[CurPos] != 0 )
    {
        if ( !isspace( pStr[CurPos] ) )
        {
            EndPos = CurPos;
        }
        CurPos++;
    }

    if ( DebPos != 0 )
    {
        // shift string left
        for ( CurPos = 0; CurPos <= ( EndPos - DebPos ); CurPos++ )
        {
            pStr[CurPos] = pStr[CurPos + DebPos];
        }
        pStr[CurPos] = 0;
    }
    else
    {
        pStr[EndPos + 1] = 0;
    }
}

//--------------------------------------------------------------------------------------------
char * str_decode( char *strout, size_t insize, const char * strin )
{
    /// @details BB> str_decode converts a string from "storage mode" to an actual string

    char *pin = strin, *pout = strout, *plast = pout + insize;

    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;
    while ( pout < plast && EOS != *pin )
    {
        *pout = *pin;
        if      ( '_' == *pout ) *pout = ' ';
        else if ( '~' == *pout ) *pout = '\t';
        pout++;
        pin++;
    };

    if ( pout < plast ) *pout = EOS;

    return strout;
}

//--------------------------------------------------------------------------------------------
char * str_encode( char *strout, size_t insize, const char * strin )
{
    /// @details BB> str_encode converts an actual string to "storage mode"

    char chrlast = 0;
    char *pin = strin, *pout = strout, *plast = pout + insize;

    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;
    while ( pout < plast && EOS != *pin )
    {
        if ( !isspace( *pin ) && isprint( *pin ) )
        {
            chrlast = *pout = tolower( *pin );
            pin++;
            pout++;
        }
        else if ( ' ' == *pin )
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else if ( '\t' == *pin )
        {
            chrlast = *pout = '~';
            pin++;
            pout++;
        }
        else if ( isspace( *pin ) )
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else if ( chrlast != '_' )
        {
            chrlast = *pout = '_';
            pin++;
            pout++;
        }
        else
        {
            pin++;
        }
    };

    if ( pout < plast ) *pout = EOS;

    return strout;
}

//--------------------------------------------------------------------------------------------
char * str_convert_slash_net(char * str, size_t size)
{
    /// @details BB> converts the slashes in a string to those appropriate for the Net

    size_t i;

    if ( INVALID_CSTR(str) ) return str;

    for (i = 0; i < size; i++)
    {
        if ('/' == str[i] || '\\' == str[i])
        {
            str[i] = NET_SLASH_CHR;
        }
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_convert_slash_sys(char * str, size_t size)
{
    /// @details BB> converts the slashes in a string to those appropriate this system

    size_t i;

    if ( INVALID_CSTR(str) ) return str;

    for (i = 0; i < size; i++)
    {
        if ('/' == str[i] || '\\' == str[i])
        {
            str[i] = SLASH_CHR;
        }
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_append_slash_net(char * str, size_t size)
{
    /// @details BB> appends a network-type slash to a string, if it does not already have one

    size_t len;

    if ( INVALID_CSTR(str) ) return str;

    len = strlen( str );
    if ( str[len-1] != '/' && str[len-1] != '\\' )
    {
        strncat(str, NET_SLASH_STR, size);
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_append_slash(char * str, size_t size)
{
    /// @details BB> appends this system's slash to a string, if it does not already have one

    size_t len;

    if ( INVALID_CSTR(str) ) return NULL;

    len = strlen( str );
    if ( str[len-1] != '/' && str[len-1] != '\\' )
    {
        strncat(str, SLASH_STR, size);
    }

    return str;
}

//--------------------------------------------------------------------------------------------
char * str_encode_path( const char *szName )
{
    //ZF> This turns a szName name into a proper filepath for loading and saving files
    //    also turns all letter to lower case in case of case sensitive OS.

    static STRING szPathname;

    char * pname, * pname_end;
    char * ppath, * ppath_end;
    char letter;

    pname = szName;
    pname_end = pname + 255;

    ppath = szPathname;
    ppath_end = ppath + 255 - 5;
    while ( '\0' != *pname && pname < pname_end && ppath < ppath_end )
    {
        letter = tolower( *pname );

        if ( ( letter < 'a' || letter > 'z' ) )  letter = '_';

        *ppath = letter;

        pname++;
        ppath++;
    }
    *ppath = '\0';

    strncat(szPathname, ".obj", SDL_arraysize(szPathname) - strlen(szPathname) );

    return szPathname;
}

//--------------------------------------------------------------------------------------------
void str_add_linebreaks( const char * text, size_t text_len, size_t line_len )
{
    char * text_end, * text_break, * text_stt;

    if ( NULL == text || '\0' == text[0] || 0 == text_len  || 0 == line_len ) return;

    text_end = text + text_len;
    text_break = text_stt = text;
    while (text < text_end && '\0' != *text)
    {
        // scan for the next whitespace
        text = strpbrk(text, " \n");
        if ( '\0' == text )
        {
            // reached the end of the string
            break;
        }
        else if ( '\n' == *text )
        {
            // respect existing line breaks
            text_break = text;
            text++;
            continue;
        }

        // until the line is too long, then insert
        // replace the last good ' ' with '\n'
        if ( ((size_t)text - (size_t)text_break) > line_len )
        {
            if ( ' ' != *text_break )
            {
                text_break = text;
            }

            // convert the character
            *text_break = '\n';

            // start over again
            text = text_break;
        }

        text++;
    }
}