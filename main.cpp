#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <list>

/*
 * TODO:
 *  - Come up with `Frame' data-structure which should hold the value for each
 *  variable. So, ?x foo ?y should have two frames (each being empty) and one
 *  value.
 *  - Make a function which parses out all of our substrings into variables or
 *  values.
 *  - Leverage C++ to not have new/delete except in List vector scenario. This
 *  means `vector' will need to be changed to a std::list.
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
     * Set a variable's slot to some other variable.
     */
    void
    set (Data *data)
    {
        assert(datatype == Var);
        slot.data = data;
    }

    /*
     * Add a child to a given List.
     */
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

    for (i = 0; i < a->list().size(); i++) {
        printf("%s %s\n", a->list()[i]->name().c_str(),
                          b->list()[i]->name().c_str());
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

            /* TODO: only intern not existing strings */
            intern_str.push_back(next_string(argv[i] + n, &offset));

            if (intern_str.back()[0] == '?')
                intern_dat.push_back(Data(Var, &intern_str.back()));
            else
                intern_dat.push_back(Data(Atom, &intern_str.back(), &intern_str.back()));
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
