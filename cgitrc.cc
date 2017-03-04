/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "cgitrc.hh"
#include "exception.hh"

#include <ostream>
#include <fstream>

void cgitrc::export_to_file(const std::string &file)
{
    std::ofstream ofs{file};

    if (!ofs
        || !(ofs << *this))
    {
        throw generic_exception{"cgitrc export failed (" + file + ")"};
    }
}

cgitrc cgitrc::new_instance(const std::string &o, repo_type t)
{
    return cgitrc{o, t};
}

cgitrc cgitrc::import_from_file(const std::string &file)
{
    std::ifstream ifs{file};

    if (!ifs)
        throw import_exception{};

    cgitrc rc;
    std::string line;

    while (getline(ifs, line))
    {
        const std::string::size_type hash =line.find('#');

        if (hash != std::string::npos)
            line.erase(hash);

        if (line.compare(0, 6, "owner=") == 0)
            rc.owner =line.substr(6);
        else if (line.compare(0, 5, "desc=") == 0)
            rc.desc =line.substr(5);
        else if (line == "mirrored")
            rc.type =repo_type::mirrored;
        else if (line == "shared")
            rc.type =repo_type::shared;
    }

    return rc;
}

// *********************************************************

std::ostream &operator<< (std::ostream &out, const cgitrc &rc)
{
    using repo_type =cgitrc::repo_type;
    //

    if (!rc.owner.empty())
        out << "owner=" << rc.owner << '\n';
    if (!rc.desc.empty())
        out << "desc=" << rc.desc << '\n';

    switch (rc.type) {
    case repo_type::shared:   out << "shared\n"; break;
    case repo_type::mirrored: out << "mirrored\n"; break;
        //
    case repo_type::unknown: break;
    }

    return out;
}
