#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

/*
 * TODO:
 *  - Come up with `Frame' data-structure which should hold the value for each
 *  variable. So, ?x foo ?y should have two frames (each being empty) and one
 *  value.
 *  - Make a function which parses out all of our substrings into variables or
 *  values.
 *  - Figure out if I need to intern strings or variables. I think the same
 *  variable name is instantiated singularly. So, if trying to unify (?x foo)
 *  and (foo ?x) then ?x would be a single frame and its value is 'foo'.
 */

/*
 * Find the next whitespace delimited substring n the string given and set the
 * offset of the beginning of the next potential variable.
 */
std::string
next_string (std::string str, int *offset)
{
    const int length = str.length();
    int skip;
    int pos;

    /* Skip all whitespace */
    skip = 0;
    while (skip < length && str[skip] == ' ')
        skip++;

    /* Find pos of next whitespace or '\0' */
    pos = 0;
    while (skip + pos < length) {
        if (str[skip + pos] == ' ')
            break;
        pos++;
    }

    *offset = skip + pos;
    return str.substr(skip, pos);
}

/*
 * Parse whitespace-delimited arguments (including those passed as quoted
 * strings) and push them into the list given by reference.
 */
void
parse_args (std::vector<std::string> &list, int argc, char **argv)
{
    int i;      /* argv index */
    int n;      /* position of substring in argv[i] */
    int len;    /* maximum length of argv[i] string */
    int offset; /* total number of bytes to advance n for next substring */

    for (i = 1; i < argc; i++) {
        len = strlen(argv[i]);
        for (n = 0; n < len;) {
            list.push_back(next_string(argv[i] + n, &offset));
            n += n + offset;
        }
    }
}

int
main (int argc, char **argv)
{
    std::vector<std::string> list;
    int i;

    parse_args(list, argc, argv);
    for (i = 0; i < list.size(); i++)
        printf("%s\n", list[i].c_str());
    return 0;
}
