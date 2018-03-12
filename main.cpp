#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <list>

/*
 * TODO:
 *  - Come up with `Frame' data-structure which should hold the value for each
 *  variable. So, ?x foo ?y should have two frames (each being empty) and one
 *  value.
 */

void
exit_error (std::string err)
{
    fprintf(stderr, "%s\n", err.c_str());
    exit(1);
}

enum DataType {
    List,  /* A list of Data */
    Atom,  /* An actual value: numbers, a string, etc. */
    Var    /* A variable which can be assigned Data (which is just Data) */
};

/*
 * A structure that can hold different types of data determined by DataType.
 */
struct Data {
private:
    DataType datatype;
    std::string *representation;
    union {
        Data *data;
        std::string *string;
        std::vector<Data*> *list;
    } slot;

public:
    Data (DataType type, std::string *name)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Var);
        slot.data = NULL;
    }

    Data (DataType type, std::string *name, std::string *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Atom);
        slot.string = val;
    }

    Data (DataType type, std::vector<Data*> *val)
        : datatype(type)
        , representation(NULL)
    {
        assert(datatype == List);
        slot.list = val;
    }

    DataType
    type ()
    {
        return datatype;
    }

    const std::string&
    name ()
    {
        assert(datatype != List);
        return *representation;
    }

    /*
     * Get the value of the data determined by its type.
     */

    const std::string&
    string ()
    {
        assert(datatype == Atom);
        return *slot.string;
    }

    const std::vector<Data*>&
    list ()
    {
        assert(datatype == List);
        return *slot.list;
    }

    Data*
    var ()
    {
        assert(datatype == Var);
        return slot.data;
    }

    /*
     * Set a variable's slot to some other Data (which could be any type).
     */
    void
    set (Data *data)
    {
        assert(datatype == Var);
        slot.data = data;
    }

    void
    add_child (Data *val)
    {
        assert(datatype == List);
        slot.list->push_back(val);
    }
};

void
unify (Data *a, Data *b)
{
    unsigned int i;

    if (a->list().size() != b->list().size())
        exit_error("Patterns must be of same length");

    for (i = 0; i < a->list().size(); i++) {
        printf("%s %s == %d\n", a->list()[i]->name().c_str(),
                          b->list()[i]->name().c_str(),
                          (a->list()[i]->name() == b->list()[i]->name()));
    }

    /*
     * 1. Get a[i] and b[i].
     * 2. if a[i] is a var and b[i] is a list, if a[i] appears in b[i], error
     *    this is recursive.
     * 3. Do the same as 2, except switch b and a.
     * 4. Set a[i] = b[i].
     * 5. Set b[i] = a[i]. Useful for ?x = ?y type scenarios.
     * 6. Test equality of a[i] and b[i] if they are atoms. If they aren't the
     *    same then we have a simple erroneous assertion: 1 == 2.
     */
}

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
 * Add only non-existing strings to the list. Returns a pointer to an existing
 * string if `str' is in the list.
 */
std::string *
intern_string (std::list<std::string> &intern, std::string str)
{
    std::list<std::string>::iterator it;
    it = std::find(intern.begin(), intern.end(), str);
    if (it != intern.end())
        return &(*it);
    intern.push_back(str);
    return &intern.back();
}

/*
 * Parse whitespace-delimited arguments (including those passed as quoted
 * strings) and push them into the list given by reference.
 */
void
parse_args (std::list<std::vector<Data*> > &intern_lst,
            std::list<std::string> &intern_str,
            std::list<Data> &intern_dat,
            std::vector<Data*> &patterns,
            int argc,
            char **argv)
{
    int i;      /* argv index */
    int n;      /* position of substring in argv[i] */
    int len;    /* maximum length of argv[i] string */
    int offset; /* total number of bytes to advance n for next substring */
    std::string *s;

    for (i = 1; i < argc; i++) {
        len = strlen(argv[i]);

        /* 
         * A bit of indirection. We create new instances of a Vector and Data
         * by adding them to the list. Then we get the address of those to use
         * for handling the individual pieces of data.
         */
        intern_lst.push_back(std::vector<Data*>());
        intern_dat.push_back(Data(List, &intern_lst.back()));
        patterns.push_back(&intern_dat.back());

        for (n = 0; n < len;) {
            /*
             * We do the same here as above. Create a new instance for a string
             * to add to the Data which is created as a new instance as well.
             */

            s = intern_string(intern_str, next_string(argv[i] + n, &offset));

            assert(!s->empty());
            if ((*s)[0] == '?')
                intern_dat.push_back(Data(Var, s));
            else
                intern_dat.push_back(Data(Atom, s, s));
            patterns.back()->add_child(&intern_dat.back());

            n = n + offset;
        }
    }
}

int
main (int argc, char **argv)
{
    /*
     * These lists hold our data in a single place that can be referenced 
     * safely by pointers throughout insertion and deletion. This lets our Data
     * use pointers without having to worry about pointer corruption.
     */
    std::list<std::vector<Data*> > intern_lst;
    std::list<std::string> intern_str;
    std::list<Data> intern_dat;
    std::vector<Data*> patterns;

    parse_args(intern_lst, intern_str, intern_dat, patterns, argc, argv);

    if (patterns.size() < 2)
        exit_error("Must have at least 2 patterns to test");

    unify(patterns[0], patterns[1]);

    return 0;
}
