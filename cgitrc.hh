/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_CGITRC_HEADER
#define GIT_JUNCTION_CGITRC_HEADER

#include <string>
#include <iosfwd>

class cgitrc {
public:
    enum class repo_type {
        unknown,
        shared,
        mirrored,
    };

private:
    std::string owner;
    std::string desc;
    repo_type type;

    cgitrc()
        : type{repo_type::unknown} {}
    cgitrc(const std::string &o, repo_type t)
        : owner{o}, type{t} {}

public:
    cgitrc(const cgitrc &) =default;
    cgitrc(cgitrc &&) =default;
    cgitrc &operator= (const cgitrc &) =default;
    cgitrc &operator= (cgitrc &&) =default;

    const std::string &get_owner() const { return owner; }
    const std::string &get_desc() const { return desc; }
    repo_type          get_type() const { return type; }

    void set_desc(const std::string &d) { desc =d; }

    void export_to_file(const std::string &file);

    //

    static cgitrc new_instance(const std::string &owner_, repo_type);
    static cgitrc import_from_file(const std::string &file);

    //
    friend std::ostream &operator<< (std::ostream &, const cgitrc &);
};

std::ostream &operator<< (std::ostream &out, const cgitrc &rc);

#endif
