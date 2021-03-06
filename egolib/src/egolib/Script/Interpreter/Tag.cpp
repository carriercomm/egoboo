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

/// @file   egolib/Script/Interpreter/Tag.cpp
/// @brief  A tag.
/// @author Michael Heilmann

#include "egolib/Script/Interpreter/Tag.hpp"

namespace Ego {
namespace Script {
namespace Interpreter {

std::string toString(Tag tag) {
    switch (tag) {
        case Tag::Boolean:
            return "Boolean";
        case Tag::Integer:
            return "Integer";
        case Tag::Object:
            return "Object";
        case Tag::Real:
            return "Real";
        case Tag::Vector2:
            return "Vector2";
        case Tag::Vector3:
            return "Vector3";
        case Tag::Void:
            return "Void";
        default:
            throw Id::UnhandledSwitchCaseException(__FILE__, __LINE__);
    }
}

} // namespace Interpreter
} // namespace Script
} // namespace Ego
