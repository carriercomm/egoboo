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
#pragma once

#include "egolib/Math/_Include.hpp"

/**
 * @brief
 *  Get the colour associated with textual feedback about a block action.
 * @return
 *  the colour
 */
const Ego::Math::Colour3f getBlockActionColour()
{
    using Colour3f = Ego::Math::Colour3f;
    static const auto& blockActionColour = Colour3f::parse(0x00, 0xbf, 0xff);
    return blockActionColour;
}
