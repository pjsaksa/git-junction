/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 */

#ifndef GIT_JUNCTION_UTILS_HEADER
#define GIT_JUNCTION_UTILS_HEADER

#include "config.hh"

#include <algorithm>
#include <ostream>
#include <streambuf>
#include <string>

#include <dirent.h>

void lowercase(std::string &input);
void crop_name(std::string &path);
std::string polish_name(std::string path);
void filter_new_repository_name(std::string &name);
bool scan_for_user(const std::string &user);
std::string calc_directory_name(const std::string &user, const std::string &password);

// *****

template <typename Vector>
void delete_vector(Vector v)
{
    std::for_each(v.begin(),
                  v.end(),
                  [](typename Vector::value_type x) { delete x; });
    v.clear();
}

// *****

class opendir_raii {
    DIR *dir;

public:
    opendir_raii(const std::string &path);
    ~opendir_raii();

    struct dirent *readdir();
};

// *****

class git_dir_functor {
public:
    virtual ~git_dir_functor();
    virtual void operator() (const std::string &) const =0;
};

//

void for_each_git_dir(const git_dir_functor &callback, const std::string &path =config::base_path);

// *****

class escape_bash : public std::ostream {
    class escape_bash_streambuf : public std::streambuf {
        std::ostream &out;
    public:
        escape_bash_streambuf(std::ostream &);

        static bool dangerous(char_type);

    protected:
        virtual std::streamsize xsputn(const char_type *, std::streamsize);
        virtual int_type overflow(int_type);
    };

    // *****

    escape_bash_streambuf buf;

public:
    escape_bash(std::ostream &);
};

#endif
