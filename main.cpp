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

    const char *
    name ()
    {
        return representation->c_str();
    }

    /*
     * Get the value of the data determined by its type.
     */

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

    bool
    is_bound ()
    {
        assert(datatype == Var);
        return (this->var()->type() != Nil);
    }

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

/*
 * Get the value of a Data object. All atomic types are their own value. Vars
 * which point to `Nil' are considered unbound. On the walk through all linked
 * vars, if we run into `Nil' then that entire chain is unbound. Otherwise the
 * atomic value at the end is the value.
 */
Data *
value (Data *d)
{
    if (d->type() == List || d->type() == Nil || d->type() == Atom)
        return d;
    while (d->type() == Var) {
        if (!d->is_bound())
            return d;
        d = d->var();
    }
    return d;
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
                        value(patterns[k]->list()[i])->name());
        }
    }

    return 0;
}
