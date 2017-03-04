/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_SSH_KEY_HEADER
#define GIT_JUNCTION_SSH_KEY_HEADER

#include <string>
#include <vector>

//

class ssh_key {
    enum flags_t {
        f_disabled,
    };

    //

    std::string original_filename;
    int index;

    time_t timestamp;
    std::string label;
    unsigned int flags;

    std::string data;

    ssh_key();

    void set_flag(flags_t);
    void clear_flag(flags_t);

public:
    ssh_key(const std::string &label, const std::string &data);
    ~ssh_key();

    ssh_key(const ssh_key &) =delete;
    ssh_key(ssh_key &&) =delete;
    ssh_key& operator= (const ssh_key &) =delete;
    ssh_key& operator= (ssh_key &&) =delete;

    bool flag(flags_t) const;

    std::string compile_filename() const;

    void update_index(int);
    void update_filename();
    void remove();

    // output/export

    void print_to_menu(std::ostream &) const;
    void export_to_storage(const std::string &user);

    // named constructors

    static ssh_key &import_from_storage(const std::string &user, const std::string &filename);
};

#endif
