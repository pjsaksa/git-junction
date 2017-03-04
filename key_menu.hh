/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_KEY_MENU_HEADER
#define GIT_JUNCTION_KEY_MENU_HEADER

#include "cgitrc.hh"
#include "ssh_key.hh"

#include <string>
#include <iosfwd>

//

class key_menu {
public:
    typedef std::vector<ssh_key *> keys_t;

private:
    class accept_menu_command;
    class accept_key_index;

    //

    static keys_t load_keys(const std::string &user);
    static void update_key_indices(keys_t &);
    static void install_keys(const std::string &user);

    //

    const std::string user;
    keys_t keys;

    key_menu(const std::string &user);

    void ask_for_new_key();
    void remove_key();

public:
    ~key_menu();

    static bool run(const std::string &user);

    //
    friend std::ostream &operator<< (std::ostream &, const key_menu &);
};

// *****

std::ostream &operator<< (std::ostream &, const key_menu &);

#endif
