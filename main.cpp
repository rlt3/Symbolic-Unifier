#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>

template <typename T>
std::string
to_string (T val)
{
    std::stringstream stream;
    stream << val;
    return stream.str();
}

/*
 * TODO:
 *  - Come up with `Frame' data-structure which should hold the value for each
 *  variable. So, ?x foo ?y should have two frames (each being empty) and one
 *  value.
 */

void
error (const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    putc('\n', stderr);
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
class Data {
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

    const char *
    name ()
    {
        return representation->c_str();
    }

    /*
     * Get the value of the data determined by its type.
     */

    const char *
    value ()
    {
        assert(datatype == Atom);
        return slot.string->c_str();
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

/*
 * Handle all the allocations of all parts of Data including strings and lists.
 */
class Intern {
public:
    Intern ()
    {
        nulldata = this->data(Nil, "nil");
    }

    Data *
    atom (std::string name)
    {
        return this->data(Atom, name);
    }

    Data *
    list ()
    {
        /* Each list has a unique name */
        static std::string name("list:");
        static int i = 0;
        return this->data(List, name + to_string(i++));
    }

    Data *
    var (std::string name)
    {
        return this->data(Var, name);
    }

protected:
    std::string*
    string (std::string str)
    {
        std::list<std::string>::iterator it;
        it = std::find(strings.begin(), strings.end(), str);
        if (it != strings.end())
            return &(*it);
        strings.push_back(str);
        return &strings.back();
    }

    Data *
    data (DataType type, std::string str)
    {
        std::map<std::string*, Data*>::iterator it;
        std::string *name = this->string(str);

        /* Find the data if it already exists (same name) */
        it = mappings.find(name);
        if (it != mappings.end())
            return it->second;

        switch (type) {
        case List:
            lists.push_back(std::vector<Data*>());
            datums.push_back(Data(type, name, &lists.back()));
            break;

        case Var:
            datums.push_back(Data(type, name, this->nulldata));
            break;

        case Atom:
            datums.push_back(Data(type, name, name));
            break;

        case Nil:
            datums.push_back(Data(type, name, name));
            break;

        default:
            assert(type == Nil);
            break;
        }

        mappings.insert(std::pair<std::string*, Data*>(name, &datums.back()));
        return &datums.back();
    }

private:
    std::list<std::vector<Data*> > lists;
    std::list<std::string> strings;
    std::list<Data> datums;
    std::map<std::string*, Data*> mappings;
    Data *nulldata;
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
    while (skip < length && isspace(str[skip]))
        skip++;

    /* Find pos of next whitespace or '\0' */
    pos = 0;
    while (skip + pos < length) {
        if (isspace(str[skip + pos]))
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
parse_args (Intern &intern, std::vector<Data*> &patterns, int argc, char **argv)
{
    int i;      /* argv index */
    int n;      /* position of substring in argv[i] */
    int len;    /* maximum length of argv[i] string */
    int offset; /* total number of bytes to advance n for next substring */
    std::string name;
    Data *parent, *child;

    for (i = 1; i < argc; i++) {
        len = strlen(argv[i]);
        parent = intern.list();
        patterns.push_back(parent);

        for (n = 0; n < len;) {
            name = next_string(argv[i] + n, &offset);
            assert(!name.empty());
            if (name[0] == '?')
                child = intern.var(name);
            else
                child = intern.atom(name);
            parent->add_child(child);
            n = n + offset;
        }
    }
}

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
extend (Data *a, Data *b)
{
    printf("%s (%p) == %s\n", a->name(), a, value(a)->name());

    /* if the pointers are the same, they are the same Data object */
    if (a == b)
        return;

    /* cannot assign values to these types. They are constant */
    if (a->type() == List)
        return;
    if (a->type() == Atom)
        return;

    /*
     * TODO: Need to work on value of variable as it is defined in `set'. So, 
     * if ?x = ?y and ?y = 5 that means ?x isn't a variable anymore but an 
     * atomic value.
     */

/* if `b` is a list and has `a' inside of it */
//if (depends_on(b, a))
//  exit_failure("%s and %s have a circular definition", a->name(), b->name());

    if (a->type() == Atom && b->type() == Atom)
        error("Cannot assign '%s' to '%s'", a->value(), b->value());

    /* 
     * TODO: Need to set variable of variable. E.g. ?x = ?y, then if ?x is set
     * to 5, that actually means ?y is set to 5 as well. They are linked.
     */
    a->set(b);
}

void
unify (Data *alist, Data *blist)
{
    Data *a, *b;
    unsigned int i;

    if (alist->list().size() != blist->list().size())
        error("Patterns must be of same length");

    for (i = 0; i < alist->list().size(); i++) {
        a = alist->list()[i];
        b = blist->list()[i];
        extend(a, b);
        extend(b, a);
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


int
main (int argc, char **argv)
{
    Intern intern;
    std::vector<Data*> patterns;

    parse_args(intern, patterns, argc, argv);

    if (patterns.size() < 2)
        error("Must have at least 2 patterns to test");

    unify(patterns[0], patterns[1]);

    unsigned int i, k;
    for (k = 0; k < patterns.size(); k++) {
        printf("\n");
        for (i = 0; i < patterns[k]->list().size(); i++) {
            printf("%s (%p) == %s\n", 
                        patterns[k]->list()[i]->name(),
                        patterns[k]->list()[i],
                        value(patterns[k]->list()[i])->value());
        }
    }

    return 0;
}
