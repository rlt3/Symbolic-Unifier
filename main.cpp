#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cctype>
#include <cstdarg>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>

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
    Nil,  /* A value that represents the void that an unbound Var points to */
    List, /* A list of Data objects */
    Atom, /* An actual value: numbers, a string, etc. */
    Var   /* A variable which can be assigned Data (which is just Data) */
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
    /* Create a Variable linked to some other Data (likely `nulldata') */
    Data (DataType type, std::string *name, Data* val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Var);
        slot.data = val;
    }

    /* Create Nil and Atom types. Their values are their representation */
    Data (DataType type, std::string *name, std::string *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Atom || datatype == Nil);
        slot.string = val;
    }

    /* Create a list of other Data */
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

    static
    const char *
    typestr (DataType type)
    {
        switch (type) {
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
     * Update existing Data objects.
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

    /* 
     * Returns the representation of this Data object when parsed in (e.g. ?x,
     * 5, foo). Because all information exists at parse time, all Vars will be
     * set to atomic type or `Nil'. So, the representation of the unified
     * pattern can be seen by calling `name` on each Data object.
     */
    const char *
    name ()
    {
        return representation->c_str();
    }

    /*
     * All vars which point to `Nil' are considered unbound.
     */
    bool
    is_bound ()
    {
        assert(datatype == Var);
        return (this->var()->type() != Nil);
    }

    bool
    is_atomic ()
    {
        return (type() == List || type() == Nil || type() == Atom);
    }

    /*
     * Get the value of a Data object. All atomic types are their own values
     * All unbound Vars will return themselves (which `name' returns its own
     * representation, e.g. ?x or ?foo). Otherwise, following the linked-list
     * of vars will return an atomic (non-nil) object.
     */
    Data *
    value ()
    {
        Data *d = this;
        if (d->is_atomic())
            return d;
        while (d->type() == Var) {
            if (!d->is_bound())
                return d;
            d = d->var();
        }
        return d;
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
    list (std::string name)
    {
        return this->data(List, name);
    }

    Data *
    var (std::string name)
    {
        return this->data(Var, name);
    }

protected:
    /* Only allocate a particular string once. Return its pointer otherwise */
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

    /* Only allocate Data once (based on name). Otherwise return its pointer */
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
 * Find the next whitespace delimited substring in the string given and set the
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
Data *
parse_arg (Intern &intern, char *arg)
{
    int n;      /* position of substring in arg */
    int len;    /* maximum length of arg string */
    int offset; /* total number of bytes to advance n for next substring */
    std::string name;
    Data *parent, *child;

    len = strlen(arg);
    parent = intern.list(arg);

    for (n = 0; n < len;) {
        name = next_string(arg + n, &offset);
        assert(!name.empty());

        if (name[0] == '?')
            child = intern.var(name);
        else
            child = intern.atom(name);

        parent->add_child(child);
        n = n + offset;
    }

    return parent;
}

void
unify (Data *a, Data *b);

void
extend (Data *a, Data *b)
{
    assert(a->type() == Var);

    /* if `a' has a value, unify with that linked value */
    if (a->is_bound()) {
        unify(a->var(), b);
    }
    /* 
     * Do the same for `b' if it is a Var. We do this here because we skip the
     * extend on `b' in `unify' implicitly using chained if/elseif statements.
     */
    else if (b->type() == Var) {
        //if (occurs(a, b))
        //    error("Relation `%s' -> `%s' is recursive", a->name(), b->name());
        if (b->is_bound())
            unify(a, b->var());
        else
            a->set(b);
    }
    /* else it's simply a variable and an atom, e.g. ?x = 5 */
    else {
        a->set(b);
    }
}

void
unify (Data *a, Data *b)
{
    unsigned int i;

    /* the two data objects are the same object, e.g. ?x = ?x or 5 = 5 */
    if (a == b) {
        return;
    }

    /* check for Vars on each side, but only set left side if both are Vars */
    else if (a->type() == Var) {
        extend(a, b);
    }
    else if (b->type() == Var) {
        extend(b, a);
    }

    /* when given two patterns to unify */
    else if (a->type() == List && b->type() == List) {
        if (a->list().size() != b->list().size())
            error("Patterns must be of same length");

        for (i = 0; i < a->list().size(); i++)
            unify(a->list()[i], b->list()[i]);
    }

    else {
        error("Cannot unify '%s' and '%s'", a->name(), b->name());
    }
}

int
main (int argc, char **argv)
{
    Intern intern;
    Data *p1, *p2;
    unsigned int i;

    if (argc < 3)
        error("Must have at least 2 patterns to test");

    for (i = 2; i < argc; i++) {
        /* parse the first two at the same time */
        if (i == 2) {
            p1 = parse_arg(intern, argv[1]);
            p2 = parse_arg(intern, argv[2]);
        }
        /* parse only one because last arg is already parsed and unified */
        else {
            p1 = p2;
            p2 = parse_arg(intern, argv[i]);
        }
        unify(p1, p2);
    }

    for (i = 0; i < p2->list().size(); i++) {
        printf("%s", p2->list()[i]->value()->name());
        if (i == p2->list().size() - 1)
            printf("\n");
        else
            printf(" ");
    }

    return 0;
}
