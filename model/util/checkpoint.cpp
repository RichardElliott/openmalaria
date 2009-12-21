/*
 This file is part of OpenMalaria.
 
 Copyright (C) 2005-2009 Swiss Tropical Institute and Liverpool School Of Tropical Medicine
 
 OpenMalaria is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or (at
 your option) any later version.
 
 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#include "util/checkpoint.hpp"
#include "util/errors.hpp"

#include <sstream>
using namespace std;

namespace OM { namespace util { namespace checkpoint {
    
    void validateListSize (long length) {
	if (length < 0 || length > 1000) {
	    ostringstream s;
	    s << "List length out of range: " << length;
	    throw checkpoint_error(s.str());
	}
    }
    
    // Note: this won't handle many characters a string might contain.
    // I use a quick hack to confirm things are expected, doing most of the work on loading.
    void operator& (string x, ostream& stream) {
	x.length() & stream;
	stream << x << endl;
    }
    void operator& (string& x, istream& stream) {
	size_t len;
	len & stream;
	stream >> x;
	if (x.length() != len)
	    // This means saved string contained spaces or something
	    throw checkpoint_error ("string error");
    }
    
} } }