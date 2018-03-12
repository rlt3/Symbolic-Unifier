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
    Nil,
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
    Data (DataType type, std::string *name, Data* val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Var);
        slot.data = val;
    }

    Data (DataType type, std::string *name, std::string *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Atom || datatype == Nil);
        slot.string = val;
    }

    Data (DataType type, std::string *name, std::vector<Data*> *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == List);
        slot.list = val;
    }

    DataType
    type ()
    {
        return datatype;
    }

    std::string
    typestr ()
    {
        switch (datatype) {
        case List:
            return "List";
        case Var:
            return "Var";
        case Nil:
            return "Nil";
        case Atom:
            return "Atom";
        default:
            assert(0);
        }
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

Data *
value (Data *d)
{
    if (d->type() == List || d->type() == Atom)
        return d;

    while (d->type() == Var)
        d = d->var();
    return d;
}

void
unify (Data *alist, Data *blist)
{
    Data *a, *b;
    unsigned int i;

    if (alist->list().size() != blist->list().size())
        exit_error("Patterns must be of same length");

    for (i = 0; i < alist->list().size(); i++) {
        a = alist->list()[i];
        b = blist->list()[i];
        printf("%s (%p) == %s\n", 
                a->name().c_str(), a, value(a)->name().c_str());
        printf("%s (%p) == %s\n", 
                b->name().c_str(), b, value(b)->name().c_str());
        printf("\n");
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

Data *nulldata = NULL;

Data *
intern_data (std::list<std::vector<Data*> > &intern_lst,
             std::list<std::string> &intern_str,
             std::list<Data> &intern_dat,
             DataType type,
             std::string name)
{
    std::string *str = intern_string(intern_str, name);

    switch (type) {
    case List:
        intern_lst.push_back(std::vector<Data*>());
        intern_dat.push_back(Data(List, str, &intern_lst.back()));
        break;

    case Var:
        assert(nulldata);
        intern_dat.push_back(Data(Var, str, nulldata));
        break;

    case Atom:
        intern_dat.push_back(Data(Atom, str, str));
        break;

    case Nil:
        intern_dat.push_back(Data(Nil, str, str));
        break;

    default:
        assert(type == Nil);
    }

    return &intern_dat.back();
}

Data *
intern_list (std::list<std::vector<Data*> > &lst,
             std::list<std::string> &str,
             std::list<Data> &dat)
{
    static int i = 0;
    static std::string l("list:");
    return intern_data(lst, str, dat, List, l + std::to_string(i++));
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
    std::string name;
    Data *parent, *child;

    for (i = 1; i < argc; i++) {
        len = strlen(argv[i]);

        /* 
         * A bit of indirection. We create new instances of a Vector and Data
         * by adding them to the list. Then we get the address of those to use
         * for handling the individual pieces of data.
         */
        parent = intern_list(intern_lst, intern_str, intern_dat);
        patterns.push_back(parent);

        for (n = 0; n < len;) {
            /*
             * We do the same here as above. Create a new instance for a string
             * to add to the Data which is created as a new instance as well.
             */

            name = next_string(argv[i] + n, &offset);
            assert(!name.empty());
            if (name[0] == '?')
                child = intern_data(intern_lst, intern_str, intern_dat, Var, name);
            else
                child = intern_data(intern_lst, intern_str, intern_dat, Atom, name);
            parent->add_child(child);

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

    nulldata = intern_data(intern_lst, intern_str, intern_dat, Nil, "nil");
    parse_args(intern_lst, intern_str, intern_dat, patterns, argc, argv);

    if (patterns.size() < 2)
        exit_error("Must have at least 2 patterns to test");

    unify(patterns[0], patterns[1]);

    return 0;
}
