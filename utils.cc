/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#include "utils.hh"
#include "exception.hh"
#include "process_io.hh"

#include <iostream>
#include <sstream>

#include <cerrno>
#include <cstring>

#include <sys/stat.h>

//

void lowercase(std::string &input)
{
    for (auto ptr =input.begin();
         ptr != input.end();
         ++ptr)
    {
        *ptr =tolower(*ptr);
    }
}

// *********************************************************

void crop_name(std::string &path)
{
    using config::base_path;
    //

    // remove base path + '/'

    if (path.size() >= base_path.size() + 1
        && path.compare(0, base_path.size(), base_path) == 0
        && path[base_path.size()] == '/')
    {
        path.erase(0, base_path.size() + 1);
    }

    // remove trailing "/.git"

    if (path.size() > 5
        && path.compare(path.size() - 5, std::string::npos, "/.git") == 0)
    {
        path.erase(path.size() - 5);
    }
}

// *********************************************************

std::string polish_name(std::string path)
{
    crop_name(path);

    const std::string::size_type dash =path.find('/');

    if (dash != std::string::npos)
        path.replace(dash, 1, " : ");

    return path;
}

// *********************************************************

void filter_new_repository_name(std::string &name)
{
    crop_name(name);

    while (!name.empty())
    {
        // apply filters

        if (name[0] == '/') {
            name.erase(0, 1);
            continue;
        }

        if (name[name.size()-1] == '/') {
            name.erase(name.size()-1, 1);
            continue;
        }

        if (name.compare(name.size()-4, std::string::npos, ".git") == 0) {
            name.erase(name.size()-4);
            continue;
        }

        // done filtering

        name += ".git";
        break;
    }
}

// *********************************************************

bool scan_for_user(const std::string &user)
{
    opendir_raii dir{"."};
    struct dirent *dirent;

    while ((dirent =dir.readdir()))
    {
        if (dirent->d_name[0] == '.')
            continue;

        const std::string d_name =dirent->d_name;

        if (d_name.size() != user.size() + 1 + config::hash_size
            || d_name[user.size()] != '-'
            || d_name.compare(0, user.size(), user) != 0)
        {
            continue;
        }

        //

        struct stat st;

        if (stat(dirent->d_name, &st) != 0) {
            std::cerr << "stat(\"" << d_name << "\"): " << strerror(errno) << "\n";
            continue;
        }

        if (S_ISDIR(st.st_mode))
            return true;
    }

    return false;
}

// *********************************************************

std::string calc_directory_name(const std::string &user, const std::string &password)
{
    process_io hashsum{config::hashsum_bin, false};

    hashsum.write() << config::salt
                    << user
                    << password
                    << '\n';

    hashsum.close_write();

    //

    std::string hash;

    if (!getline(hashsum.read(), hash, ' '))
        throw generic_exception{"getline(hashsum) failed"};
    if (hash.size() != config::hashsum_result_size)
        throw generic_exception{"hashsum_bin produces incorrect size hashes"};

    hash.erase(config::hash_size);

    std::ostringstream directory_oss;
    directory_oss << user << '-' << hash;
    return directory_oss.str();
}

// *********************************************************

opendir_raii::opendir_raii(const std::string &path)
    : dir{opendir(path.c_str())}
{
    if (!dir)
        throw stdlib_exception{"opendir(" + path + ")", errno};
}

opendir_raii::~opendir_raii()
{
    closedir(dir);
}

struct dirent *opendir_raii::readdir()
{
    return ::readdir(dir);
}

// *********************************************************

git_dir_functor::~git_dir_functor()
{
}

// *********************************************************

static bool is_git_dir(const std::string &path)
{
    struct stat st;

    // check that "<path>/objects" is directory

    {
        const std::string chk =path + "/objects";

        if (stat(chk.c_str(), &st) != 0) {
            if (errno == ENOENT)
                return false;
            throw stdlib_exception{"stat(" + chk + ")", errno};
        }

        if (!S_ISDIR(st.st_mode))
            return false;
    }

    // check that "<path>/HEAD" is a regular file

    {
        const std::string chk =path + "/HEAD";

        if (stat(chk.c_str(), &st) != 0) {
            if (errno == ENOENT)
                return false;
            throw stdlib_exception{"stat(" + chk + ")", errno};
        }

        if (!S_ISREG(st.st_mode))
            return false;
    }

    return true;
}

void for_each_git_dir(const git_dir_functor &callback, const std::string &path)
{
    opendir_raii dir{path.c_str()};
    struct dirent *dirent;

    while ((dirent =dir.readdir()))
    {
        if (dirent->d_name[0] == '.')
            continue;

        struct stat st;

        const std::string next =path + '/' + dirent->d_name;

        if (stat(next.c_str(), &st) != 0)
            throw stdlib_exception{"stat(" + next + ")", errno};
        if (!S_ISDIR(st.st_mode))
            continue;

        const std::string next_git =next + "/.git";

        if (is_git_dir(next_git))
            callback(next_git);
        else if (is_git_dir(next))
            callback(next);
        else {
            for_each_git_dir(callback, next);
        }
    }
}

// *********************************************************

escape_bash::escape_bash_streambuf::escape_bash_streambuf(std::ostream &o)
    : out{o}
{
}

bool escape_bash::escape_bash_streambuf::dangerous(char_type ch)
{
    switch (ch) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case ',':
    case '.':
    case '_':
    case '+':
    case ':':
    case '@':
    case '%':
    case '/':
    case '-':
        return false;

    default:
        return true;
    }
}

std::streamsize escape_bash::escape_bash_streambuf::xsputn(const char_type *s, std::streamsize n)
{
    const char_type *const end =&s[n];

    while (s < end) {
        {
            const char_type *ptr =s;

            for (ptr =s; ptr < end; ++ptr) {
                if (dangerous(*ptr))
                    break;
            }

            if (ptr > s) {
                out.write(s, ptr - s);
                s =ptr;
            }
        }

        if (s >= end)
            break;

        do {
            out.put('\\');
            out.put(*s++);

        } while (s < end
                 && dangerous(*s));
    }

    return n;
}

std::streambuf::int_type escape_bash::escape_bash_streambuf::overflow(int_type c)
{
    if (dangerous(c))
        out.put('\\');

    out.put(c);
    return c;
}

// *********************************************************

escape_bash::escape_bash(std::ostream &out)
    : std::ostream{&buf},
      buf{out}
{
}
