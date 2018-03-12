#include <cstdlib>
#include <cstdio>
#include <cstring>
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
    DataType type;
    void *data;

    /*
     * TODO: To avoid any problems with erasure here, make it impossible to set
     * `type' except through the constructor and also have `data' passed
     * through the constructor. We can guard the `getters` in the normal way
     * to protect against programmer errors but have a more stable situation
     * in C++'s environment.
     */

    Data (const Data &d)
        : type(d.type)
        , data(d.data)
    { }

    Data (DataType type)
        : type(type)
        , data(NULL)
    {
        if (type == List)
            data = (void*) new std::vector<Data*>;
    }

    ~Data ()
    {
        std::vector<Data*>* list;
        unsigned int i;

        if (type == List) {
            list = static_cast<std::vector<Data*>*>(data);
            for (i = 0; i < list->size(); i++)
                delete (*list)[i];
            delete list;
        }
    }

    std::string
    string ()
    {
        if (type != Atom)
            exit_error("Cannot get `string' from non-Atom type");
        if (!data)
            exit_error("Cannot get `string' from NULL data");
        return *(static_cast<std::string*>(data));
    }

    std::vector<Data*>
    list ()
    {
        if (type != List)
            exit_error("Cannot get `list' from non-List type");
        return *(static_cast<std::vector<Data*>*>(data));
    }

    Data*
    var ()
    {
        if (type != Var)
            exit_error("Cannot get `var' from non-Var type");
        if (!data)
            exit_error("Cannot get `var' from NULL data");
        return static_cast<Data*>(data);
    }

    void
    add_child (Data *val)
    {
        if (type != List)
            exit_error("Cannot add_child to non-List type");
        static_cast<std::vector<Data*>*>(data)->push_back(val);
    }

    void
    set (Data *val)
    {
        if (type != Var)
            exit_error("Cannot set `Data` to non-Var type");
        data = static_cast<void*>(val);
    }

    void
    set (std::string &val)
    {
        if (type != Atom)
            exit_error("Cannot set `string` to non-Atom type");
        data = static_cast<void*>(&val);
    }
};

void
unify (Data *a, Data *b)
{
    std::vector<Data*> children;
    unsigned int i;

    children = a->list();
    for (i = 0; i < children.size(); i++)
        printf("%s\n", children[i]->string().c_str());
    printf("\n");

    children = b->list();
    for (i = 0; i < children.size(); i++)
        printf("%s\n", children[i]->string().c_str());

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
parse_args (std::list<std::string> &interned,
            std::vector<Data*> &patterns,
            int argc,
            char **argv)
{
    int i;      /* argv index */
    int n;      /* position of substring in argv[i] */
    int len;    /* maximum length of argv[i] string */
    int offset; /* total number of bytes to advance n for next substring */
    Data *parent, *child;

    for (i = 1; i < argc; i++) {
        len = strlen(argv[i]);

        /* for each string, it is a new list */
        parent = new Data(List);
        patterns.push_back(parent);

        for (n = 0; n < len;) {
            /* the string's only owner is `interned' */
            /* TODO: only intern not existing strings */
            interned.push_back(next_string(argv[i] + n, &offset));

            /* make new Atomic Data using string as data */
            /* TODO: if str[0] is ? then it is a variable */
            child = new Data(Atom);
            child->set(interned.back());
            parent->add_child(child);

            n = n + offset;
        }
    }
}

int
main (int argc, char **argv)
{
    /*
     * A vector cannot guarantee valid pointers to strings if more strings are
     * inserted. We use a list instead so we can have strings in single place.
     */
    std::list<std::string> interned;
    std::vector<Data*> patterns;
    unsigned int i;

    parse_args(interned, patterns, argc, argv);

    if (patterns.size() < 2)
        exit_error("Must have at least 2 patterns to test");

    unify(patterns[0], patterns[1]);

    for (i = 0; i < patterns.size(); i++)
        delete patterns[i];

    return 0;
}
