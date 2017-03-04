/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_REPOSITORY_MENU_HEADER
#define GIT_JUNCTION_REPOSITORY_MENU_HEADER

#include "cgitrc.hh"

#include <string>
#include <iosfwd>

class repository_menu {
    class accept_functor;

    //

    enum {
        safe_max   =10000,
        safe_width =4,
    };

    //

    static bool read_publicity(const std::string &path);

    // *****

    const std::string path;
    const cgitrc rc;
    const bool publicity;

    const unsigned int abandon_safe;
    const unsigned int remove_safe;

    //

    repository_menu(const std::string &user, const std::string &path);

    accept_functor get_accept_functor() const;

    void change_description();
    void toggle_publicity();

public:
    static bool run(const std::string &user, const std::string &path);

    //
    friend std::ostream &operator<< (std::ostream &, const repository_menu &);
};

// *****

std::ostream &operator<< (std::ostream &, const repository_menu &);

#endif
