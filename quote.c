/* git-junction
 * Copyright (c) 2016-2017 by Pauli Saksa
 *
 * Licensed under The MIT License, see file LICENSE.txt in this source tree.
 *
 *********************************************************
 *
 * FOR YOUR INFORMATION:
 *
 * I have blatantly copied the following lines of code from git source code,
 * from a file which can (at the time of writing) be found from URL
 * https://github.com/git/git/blob/master/quote.c . That code is licensed with
 * GPL2. I am not a lawyer, but I may (or may not) be breaking some laws by
 * copying GPL2-licensed code and re-licensing it with MIT licence. I'm going to
 * disregard that because of three reasons:
 *
 * 1) As it states in the beginning of GPL2 license:
 *
 *      "--- the GNU General Public License is intended to guarantee your
 *      freedom to share and change free software--to make sure the software is
 *      free for all its users. ---"
 *
 * For my part, I think I have abided by that intention of sharing software by
 * releasing my code with an open source license.
 *
 * 2) This software does not aim to replace git, nor any of its functions
 *
 * These lines are just a tiny piece of git source code. By copying these lines,
 * I'm not aiming to replace git. Instead, I'm working towards the goal of
 * producing a larger environment for git to operate in.
 *
 * 3) I have only copied necessary parts for maintaining interoperability
 *
 * The parts I have copied are completely meaningless in the sense of what git
 * is and what it does, but it is a crucial part for making a desired
 * interaction with git.
 *
 *
 * Whether I am working with or against the letter of the law, I'm going to
 * disregard the possible legal threats because of the three reasons stated
 * above. I take licenses quite seriously -- not because of the possible legal
 * ramifications, but because I want to respect people's wishes over their work.
 *
 * I believe I am working towards the spirit of git and wishes of the people
 * behind it even if I wasn't operating completely by the letter of the law by
 * re-licensing these few lines of code.
 *                                                --Pauli Saksa, 13.5.2016
 */

#include <stdlib.h>
#include <ctype.h>

int need_bs_quote(char c)
{
    return (c == '\'' || c == '!');
}

static char *sq_dequote_step(char *arg, char **next)
{
    char *dst = arg;
    char *src = arg;
    char c;

    if (*src != '\'')
        return NULL;
    for (;;) {
        c = *++src;
        if (!c)
            return NULL;
        if (c != '\'') {
            *dst++ = c;
            continue;
        }
        /* We stepped out of sq */
        switch (*++src) {
        case '\0':
            *dst = 0;
            if (next)
                *next = NULL;
            return arg;
        case '\\':
            c = *++src;
            if (need_bs_quote(c) && *++src == '\'') {
                *dst++ = c;
                continue;
            }
            /* Fallthrough */
        default:
            if (!next || !isspace(*src))
                return NULL;
            do {
                c = *++src;
            } while (isspace(c));
            *dst = 0;
            *next = src;
            return arg;
        }
    }
}

char *sq_dequote(char *arg)
{
    return sq_dequote_step(arg, NULL);
}
