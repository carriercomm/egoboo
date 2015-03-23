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

/// @file egolib/Graphics/Font.cpp
/// @brief TTF management
/// @details TrueType font drawing functionality.  Uses the SDL_ttf module
///          to do its business. This depends on SDL_ttf and OpenGL.

#include "egolib/Graphics/Font.hpp"

#include "egolib/Core/StringUtilities.hpp"
#include "egolib/Graphics/FontManager.hpp"
#include "egolib/Extensions/ogl_texture.h"
#include "egolib/Renderer/Renderer.hpp"
#include "egolib/log.h"
#include "egolib/vfs.h"

// this include must be the absolute last include
#include "egolib/mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

namespace Ego
{
    struct Font::StringCacheData
    {
        Uint32 lastUseInTicks;
        oglx_texture_t *tex;
        std::string text;
        
        StringCacheData(const StringCacheData &) = delete;
        StringCacheData &operator=(const StringCacheData &) = delete;
        
        StringCacheData() :
        lastUseInTicks(0),
        tex(new oglx_texture_t())
        {
            oglx_texture_t *tmp = oglx_texture_t::ctor(tex);
            EGOBOO_ASSERT(tex == tmp);
        }
        
        ~StringCacheData()
        {
            if (tex != nullptr)
            {
                oglx_texture_t::dtor(tex);
                delete tex;
            }
        }
    };
    
    Font::Font(const std::string &fileName, int pointSize) :
    _ttfFont(nullptr),
    _stringCache(),
    _sortedCache()
    {
        SDL_RWops *rwops = vfs_openRWopsRead(fileName.c_str());
        if (rwops == nullptr)
        {
            log_warning("Failed to open '%s' via vfs: %s\n", fileName.c_str(), vfs_getError());
            return;
        }
        _ttfFont = TTF_OpenFontRW(rwops, 1, pointSize);
        if (_ttfFont == nullptr)
        {
            log_warning("Failed to open '%s' via SDL_ttf: %s\n", fileName.c_str(), TTF_GetError());
            return;
        }
    }
    
    Font::~Font()
    {
        if (_ttfFont && Ego::FontManager::isInitialized()) TTF_CloseFont(_ttfFont);
    }
    
    void Font::getTextSize(const std::string &text, int *width, int *height) const
    {
        if (_ttfFont == nullptr) return;
        
        int w = 0;
        int h = 0;
        if (width == nullptr) width = &w;
        if (height == nullptr) height = &h;
        
        TTF_SizeUTF8(_ttfFont, text.c_str(), width, height);
    }
    
    void Font::getTextBoxSize(const std::string &text, int spacing, int *width, int *height) const
    {
        if (_ttfFont == nullptr) return;
        
        int w = 0;
        int h = 0;
        if (width == nullptr) width = &w;
        if (height == nullptr) height = &h;
        
        *width = 0; *height = 0;
        for (const std::string &line : Ego::split(text, std::string("\n")))
        {
            if (line == "\n") continue;
            int lineWidth = 0;
            int lineHeight = 0;
            TTF_SizeUTF8(_ttfFont, line.c_str(), &lineWidth, &lineHeight);
            *width = std::max(*width, lineWidth);
            *height += spacing;
        }
    }
    
    void Font::drawTextToTexture(oglx_texture_t *tex, const std::string &text, const Ego::Math::Colour4f &colour) const
    {
        if (_ttfFont == nullptr) return;
        
        SDL_Color sdlColor;
        sdlColor.r = static_cast<Uint8>(colour.getRed() * 255);
        sdlColor.g = static_cast<Uint8>(colour.getGreen() * 255);
        sdlColor.b = static_cast<Uint8>(colour.getBlue() * 255);
        //this is called 'a' and is used as alpha in SDL2
        sdlColor.unused = static_cast<Uint8>(colour.getAlpha() * 255);
        
        SDL_Surface *textSurface = TTF_RenderUTF8_Blended(_ttfFont, text.c_str(), sdlColor);
        if (textSurface == nullptr)
        {
            log_warning("Got a null surface from SDL_TTF: %s", TTF_GetError());
            return;
        }
        
        oglx_texture_convert(tex, textSurface, INVALID_KEY);
        std::string name = "Font text '" + text + "'";
        strncpy(tex->name, name.c_str(), SDL_arraysize(tex->name));
        tex->name[SDL_arraysize(tex->name) - 1] = '\0';
    }
    
    void Font::drawText(const std::string &text, int x, int y, const Ego::Math::Colour4f &colour)
    {
        if (_ttfFont == nullptr || text.empty()) return;
        StringCacheDataPtr cache;
        bool updateTexture = true;
        auto cacheIterator = _stringCache.find(text);
        if (cacheIterator != _stringCache.end() && !cacheIterator->second.expired())
        {
            cache = cacheIterator->second.lock();
            updateTexture = false;
        }
        else if (_sortedCache.size() < 20)
        {
            cache = std::make_shared<StringCacheData>();
            _sortedCache.push_back(cache);
        }
        else
        {
            std::sort(_sortedCache.begin(), _sortedCache.end(), compareStringCacheData);
            cache = _sortedCache.at(0);
            _stringCache.erase(cache->text);
        }
        
        if (updateTexture) drawTextToTexture(cache->tex, text);
        
        float w = cache->tex->imgW;
        float h = cache->tex->imgH;
        float u = w / cache->tex->base.width;
        float v = h / cache->tex->base.height;
        
        Ego::Renderer::get().setColour(colour);
        oglx_texture_bind(cache->tex);
        
        GL_DEBUG(glBegin)(GL_QUADS);
        {
            GL_DEBUG(glTexCoord2f)(0, 0);
            GL_DEBUG(glVertex2f)(x, y);
            
            GL_DEBUG(glTexCoord2f)(u, 0);
            GL_DEBUG(glVertex2f)(x + w, y);
            
            GL_DEBUG(glTexCoord2f)(u, v);
            GL_DEBUG(glVertex2f)(x + w, y + h);
            
            GL_DEBUG(glTexCoord2f)(0, v);
            GL_DEBUG(glVertex2f)(x, y + h);
        }
        GL_DEBUG_END();
        
        cache->lastUseInTicks = SDL_GetTicks();
        cache->text = text;
        _stringCache[text] = cache;
    }
    
    void Font::drawTextBox(const std::string &text, int x, int y, int width, int height, int spacing, const Ego::Math::Colour4f &colour)
    {
        if (_ttfFont == nullptr) return;
        for (const std::string &line : Ego::split(text, std::string("\n")))
        {
            if (line == "\n") continue;
            drawText(line, x, y, colour);
            y += spacing;
        }
    }
    
    int Font::getLineSpacing() const
    {
        if (_ttfFont == nullptr) return 0;
        return TTF_FontLineSkip(_ttfFont);
    }
    
    bool Font::compareStringCacheData(const StringCacheDataPtr &a, const StringCacheDataPtr &b)
    {
        if (b == nullptr) return false;
        if (a == nullptr) return true;
        return a->lastUseInTicks < b->lastUseInTicks;
    }
}

