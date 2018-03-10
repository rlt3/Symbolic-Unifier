#include <cstdlib>
#include <cstdio>
#include <cstring>
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

enum DataType {
    List,  /* A list of Data */
    Atom,  /* An actual value: numbers, a string, etc. */
    Var    /* A variable which can be assigned Data (which is just Data) */
};

/*
 * A structure that can hold different types of data determined by DataType.
 */
struct Data {
    DataType type;
    void *data;

    /*
     * The internal, hardset values of strings and lists are returned as a copy
     * and not a reference or pointer. But the values of Data frames can be set
     * and reset and otherwise 'messed with' so returned pointers make sense.
     * This all means that all core data needs to be allocated separately (if 
     * at all, can just let c++ handle allocations and return addresses of 
     * everything).
     */

    std::string
    string () {
        if (type != Atom)
            return std::string();
        return *(static_cast<std::string*>(data));
    }

    std::vector<Data*>
    list () {
        if (type != List)
            return std::vector<Data*>();
        return *(static_cast<std::vector<Data*>*>(data));
    }

    Data*
    var () {
        if (type != Var)
            return NULL;
        return static_cast<Data*>(data);
    }
};

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
    unsigned int i;

    parse_args(list, argc, argv);
    for (i = 0; i < list.size(); i++)
        printf("%s\n", list[i].c_str());
    return 0;
}
