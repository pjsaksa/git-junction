/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "exception.hh"

#include <ostream>
#include <cstring>

std::ostream &operator<< (std::ostream &out, const generic_exception &e)
{
    return out << e.message;
}

std::ostream &operator<< (std::ostream &out, const stdlib_exception &e)
{
    return out << e.function << ": " << strerror(e.error);
}
