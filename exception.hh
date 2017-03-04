/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_EXCEPTION_HEADER
#define GIT_JUNCTION_EXCEPTION_HEADER

#include <iosfwd>
#include <string>

//

class stdin_exception {};
class import_exception {};

//

class generic_exception {
    const std::string message;
public:
    generic_exception(const std::string &m)
        : message{m} {}

    //
    friend std::ostream &operator<< (std::ostream &, const generic_exception &);
};

//

class stdlib_exception {
    const std::string function;
    const int error;
public:
    stdlib_exception(const std::string &f, int e)
        : function{f}, error{e} {}

    //
    friend std::ostream &operator<< (std::ostream &, const stdlib_exception &);
};

// *********************************************************

std::ostream &operator<< (std::ostream &, const generic_exception &);
std::ostream &operator<< (std::ostream &, const stdlib_exception &);

#endif
