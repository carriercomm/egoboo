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

/// @file   IdLib/UnhandledSwitchCaseException.hpp
/// @brief  Definition of an exception indicating a value is out of bounds
/// @author Michael Heilmann

#pragma once

#if !defined(IDLIB_PRIVATE) || IDLIB_PRIVATE != 1
#error(do not include directly, include `IdLib/IdLib.hpp` instead)
#endif

#include "IdLib/Exception.hpp"

namespace Id {

using namespace std;

/**
 * @brief
 *  Exception indicating a value is out of bounds
 */
class OutOfBoundsException : public Exception {

public:

    /**
     * @brief
     *  Construct this exception.
     * @param file
     *  the C++ source file (as obtained by the __FILE__ macro) associated with this exception
     * @param line
     *  the line within the C++ source file (as obtained by the __LINE__ macro) associated with this exception
     * @param message
     *  optional exception string message
     */
    OutOfBoundsException(const char *file, int line, const string& message = "Value out of bounds") :
        Exception(file, line), message(message) {}

    /**
     * @brief
     *  Construct this exception with the value of another exception.
     * @param other
     *  the other exception
     */
    OutOfBoundsException(const OutOfBoundsException& other) :
        Exception(other), message(other.message) {}

    /**
     * @brief
     *  Assign this exception the values of another exception.
     * @param other
     *  the other exception
     * @return
     *  this exception
     */
    OutOfBoundsException& operator=(const OutOfBoundsException& other) {
        Exception::operator=(other);
        message = other.message;
        return *this;
    }

public:

    /**
    * @brief
    *  Get the message associated with this exception.
    * @return
    *  the message associated with this exception
    */
    const string& getMessage() const {
        return message;
    }

    /**
    * @brief
    *  Overloaded cast operator for casting into std::string.
    * @return
    *  a human-readable textual description of the string.
    */
    virtual operator string() const {
        ostringstream buffer;
        buffer << "(raised in file " << getFile() << ", line " << getLine() << ")"
            << ":" << std::endl;
        buffer << message;
        return buffer.str();
    }

private:
    string message;
};

} // namespace Id
