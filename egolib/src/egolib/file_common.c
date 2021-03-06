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

/// @file egolib/file_common.c
/// @brief Base implementation of the Egoboo filesystem
/// @details File operations that are shared between various operating systems.
/// OS-specific code goes in *-file.c (such as win-file.c)

#include "egolib/file_common.h"

#include "egolib/Log/_Include.hpp"

#include "egolib/strutil.h"
#include "egolib/vfs.h"
#include "egolib/platform.h"

#if !defined(MAX_PATH)
#define MAX_PATH 260  // Same value that Windows uses...
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool _fs_initialized = false;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
/**
 * @brief
 *  Initialize the platform file system.
 * @param argv0
 *  the first argument of the command-line
 * @return
 *  @a 0 on success, a non-zero value on failure
 */
int sys_fs_init(const char *argv0);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

int fs_init(const char *argv0)
{
    if (_fs_initialized)
    {
        return 0;
    }
    if (sys_fs_init(argv0))
    {
        return 1;
    }
    _fs_initialized = true;
    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void fs_removeDirectoryAndContents(const char *dirname, int recursive)
{
    /// @author ZZ
    /// @details This function deletes all files in a directory,
    ///    and the directory itself

    char filePath[MAX_PATH] = EMPTY_CSTR;
    const char *fileName;
    fs_find_context_t fs_search;

    // List all the files in the directory
    fileName = fs_findFirstFile(dirname, NULL, &fs_search);
    while (NULL != fileName)
    {
        // Ignore files that start with a ., like .svn for example.
        if ('.' != fileName[0])
        {
            snprintf(filePath, MAX_PATH, "%s" SLASH_STR "%s", dirname, fileName);
            if (fs_fileIsDirectory(filePath))
            {
                if (recursive)
                {
                    fs_removeDirectoryAndContents(filePath, recursive);
                }
                else
                {
                    fs_removeDirectory(filePath);
                }
            }
            else
            {
                fs_deleteFile(filePath);
            }
        }
        fileName = fs_findNextFile(&fs_search);
    }
    fs_findClose(&fs_search);

    fs_removeDirectory(dirname);
}

//--------------------------------------------------------------------------------------------
void fs_copyDirectory(const char *sourceDir, const char *targetDir)
{
    fs_find_context_t fs_search;

    // List all the files in the directory
    const char *filename = fs_findFirstFile(sourceDir, NULL, &fs_search);
    if (filename)
    {
        // Make sure the destination directory exists.
        fs_createDirectory(targetDir); /// @todo Error handling here - if the directory does not exist, we can stop.

        while (filename)
        {
            // Ignore files that begin with a `'.'`.
            if ('.' != filename[0])
            {
                char sourcePath[MAX_PATH] = EMPTY_CSTR, targetPath[MAX_PATH] = EMPTY_CSTR;
                snprintf(sourcePath, MAX_PATH, "%s" SLASH_STR "%s", sourceDir, filename);
                snprintf(targetPath, MAX_PATH, "%s" SLASH_STR "%s", targetDir, filename);
                fs_copyFile(sourcePath, targetPath);
            }

            filename = fs_findNextFile(&fs_search);
        }
    }

    fs_findClose(&fs_search);
}

//--------------------------------------------------------------------------------------------
int fs_fileExists(const char *filename)
{
    if (INVALID_CSTR(filename))
    {
        return -1;
    }
    FILE *ptmp = fopen(filename, "rb");
    if (ptmp)
    {
        fclose(ptmp);
        return 1;
    }
    return 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
const char *fs_createBinaryDirectoryFilename(const char *relative_pathname)
{
    static char path[1024]; ///< @todo Not mt-safe.
    const char *dir_name_ptr;

    path[0] = CSTR_END;
    if (!VALID_CSTR(relative_pathname)) return path;

    dir_name_ptr = fs_getBinaryDirectory();

    if (VALID_CSTR(dir_name_ptr))
    {
        snprintf(path, SDL_arraysize(path), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname);
    }
    else
    {
        snprintf(path, SDL_arraysize(path), "." SLASH_STR "%s", relative_pathname);
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char *fs_createDataDirectoryFilename(const char *relative_pathname)
{
    static char path[1024]; ///< @todo Not mt-safe.
    const char *dir_name_ptr;

    path[0] = CSTR_END;
    if (!VALID_CSTR(relative_pathname)) return path;

    dir_name_ptr = fs_getDataDirectory();

    if (VALID_CSTR(dir_name_ptr))
    {
        snprintf(path, SDL_arraysize(path), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname);
    }
    else
    {
        snprintf(path, SDL_arraysize(path), "." SLASH_STR "%s", relative_pathname);
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char * fs_createUserDirectoryFilename(const char *relative_pathname)
{
    static char path[1024]; ///< @todo Not mt-safe.
    const char *dir_name_ptr;

    path[0] = CSTR_END;
    if (!VALID_CSTR(relative_pathname)) return path;

    dir_name_ptr = fs_getUserDirectory();

    if (VALID_CSTR(dir_name_ptr))
    {
        snprintf(path, SDL_arraysize(path), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname);
    }
    else
    {
        snprintf(path, SDL_arraysize(path), "." SLASH_STR "%s", relative_pathname);
    }

    return path;
}

//--------------------------------------------------------------------------------------------
const char *fs_createConfigDirectoryFilename(const char *relative_pathname)
{
    static char path[1024];
    const char * dir_name_ptr;

    path[0] = CSTR_END;
    if (!VALID_CSTR(relative_pathname)) return path;

    dir_name_ptr = fs_getConfigDirectory();

    if (VALID_CSTR(dir_name_ptr))
    {
        snprintf(path, SDL_arraysize(path), "%s" SLASH_STR "%s", dir_name_ptr, relative_pathname);
    }
    else
    {
        snprintf(path, SDL_arraysize(path), "." SLASH_STR "%s", relative_pathname);
    }

    return path;
}
#endif
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#if 0
FILE * fs_openBinaryDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createBinaryDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}
#endif

//--------------------------------------------------------------------------------------------
#if 0
FILE * fs_openDataDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createDataDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}
#endif
//--------------------------------------------------------------------------------------------
#if 0
FILE * fs_openUserDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createUserDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}
#endif

//--------------------------------------------------------------------------------------------
#if 0
FILE * fs_openConfigDirectoryFile( const char * relative_pathname, const char * mode )
{
    FILE       * file_ptr = NULL;
    const char * path_ptr;

    if ( !VALID_CSTR( relative_pathname ) ) return NULL;

    path_ptr = fs_createConfigDirectoryFilename( relative_pathname );

    if ( VALID_CSTR( path_ptr ) )
    {
        file_ptr = fopen( path_ptr, mode );
    }
    else
    {
        file_ptr = fopen( relative_pathname, mode );
    }

    return file_ptr;
}
#endif
//--------------------------------------------------------------------------------------------
bool fs_ensureUserFile( const char * relative_filename, bool required )
{
    /// @author BB
    /// @details if the file does not exist in the user data directory, it is copied from the
    /// data directory. Pass this function a system-dependent pathneme, relative the the root of the
    /// data directory.
    ///
    /// @note we can't use the vfs to do this in win32 because of the dir structure and
    /// the fact that PHYSFS will not add the same directory to 2 different mount points...
    /// seems pretty stupid to me, but there you have it.

    STRING path_str;
    int found;

    snprintf( path_str, SDL_arraysize( path_str ), "%s" SLASH_STR "%s", fs_getUserDirectory(), relative_filename );
    str_convert_slash_sys( path_str, SDL_arraysize( path_str ) );

    found = fs_fileExists( path_str );
    if ( 0 == found )
    {
        STRING src_path_str;

        // copy the file from the Data Directory to the User Directory

        snprintf( src_path_str, SDL_arraysize( src_path_str ), "%s" SLASH_STR "%s", fs_getConfigDirectory(), relative_filename );

        fs_copyFile( src_path_str, path_str );

        found = fs_fileExists( path_str );
    }

    // if it still doesn't exist, we have problems
    if (( 0 == found ) && required )
    {
		std::ostringstream os;
		os << "cannot find the file `" << relative_filename << "`" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }

    return ( 0 != found );
}
