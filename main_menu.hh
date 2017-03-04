/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_MAIN_MENU_HEADER
#define GIT_JUNCTION_MAIN_MENU_HEADER

#include <string>
#include <vector>

class main_menu {
    class scanner;
    class accept_functor;

    //

    static bool repository_compare(std::string left, std::string right);
    static void share_local_repository(const std::string &user);
    static void mirror_remote_repository(const std::string &user);

    // *****

    typedef std::vector<std::string> repositories_t;

    //

    repositories_t repositories;

    void add_repository(const std::string &r);

public:
    main_menu(const std::string &user);

    accept_functor get_accept_functor() const;

    //

    static bool run(const std::string &user);

    //
    friend std::ostream &operator<< (std::ostream &, const main_menu &);
};

// *****

std::ostream &operator<< (std::ostream &out, const main_menu &menu);

#endif
