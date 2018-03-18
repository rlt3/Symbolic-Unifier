#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdarg>
#include <sstream>

#include "Datum.cpp"
#include "Data.cpp"

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

void
unify (Datum *a, Datum *b);

/*
 * Test whether `var' occurs in `expression'.
 */
bool
occurs (Datum *var, Datum *expression)
{
    unsigned int i;

    if (expression->type() == Var) {
        if (var == expression)
            return true;
        return occurs(var, expression->var());
    }
    else if (expression->type() == List) {
        for (i = 0; i < expression->list()->size(); i++)
            if (occurs(var, expression->list()->at(i)))
                return true;
    }

    return false;
}

/*
 * Extend the value of `a' with the the value of `b'.
 */
void
extend (Datum *a, Datum *b)
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
    else if (occurs(a, b)) {
        error("Relation `%s' -> `%s' is recursive", 
                a->representation().c_str(), b->representation().c_str());
    }
    /* else it's simply a variable and an atom, e.g. ?x = 5 */
    else {
        a->set(b);
    }
}

/*
 * Unify two distinct datum objects of different types.
 */
void
unify (Datum *a, Datum *b)
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
        if (a->list()->size() != b->list()->size())
            error("Patterns must be of same length");

        for (i = 0; i < a->list()->size(); i++)
            unify(a->list()->at(i), b->list()->at(i));
    }

    else {
        error("Cannot unify '%s' and '%s'",
                a->representation().c_str(), b->representation().c_str());
    }
}

/* 
 * The following functions are all the parser which is a look-ahead recursive
 * descent parser. LOOK is the lookahead token which is affected only by 
 * `next' and read only by `look()'. INPUT is simply the stream of characters
 * that is read with `next'.
 */

static int LOOK = EOF - 1;
static std::stringstream INPUT;

int
next ()
{
    LOOK = INPUT.get();
    return LOOK;
}

int
look ()
{
    if (LOOK == EOF - 1)
        return next();
    else
        return LOOK;
}

void
skipwhitespace ()
{
    while (isspace(look()))
        next();
}

bool
in_string (const int c, std::string s)
{
    for (unsigned i = 0; i < s.size(); i++)
        if (s[i] == c)
            return true;
    return false;
}

bool
is_reserved (const int c)
{
    return in_string(c, "?(,)=");
}


/*
 * Assert a match, exiting the program with an error if assertion fails.
 */
std::string
match (int c)
{
    if (look() != c)
        error("Expected `%c' got `%c' instead", c, look());
    next();
    return std::string(1, c);
}

/* 
 * Read the next whitespace delimited string.
 * <name> := ([^?(,)=]a-zA-Z)+
 */
std::string
name ()
{
    std::string s;
    skipwhitespace();

    /* Read the whitespace-delimited string */
    while (!(is_reserved(look()) || isspace(look()) || look() == EOF)) {
        s += look();
        next();
    }

    return s;
}

/*
 * <var> := ?<name>
 */
Datum *
var (Data &data)
{
    match('?');
    return data.var(name());
}

Datum * expr (Data &data);

/*
 * Parse a function and all of its arguments into a Datum list.
 * To allow functions of the same name but which point to different Datum
 * object we parse both the function name and also its representation (the name
 * followed by all the representations of the arguments). Then we intern based
 * off its representation rather than its name.
 * <func> := <name>([<expr> [, <expr> ...])
 */
Datum *
func (Data &data, std::string name)
{
    std::vector<Datum*> children;
    std::string rep;
    unsigned int i;
    Datum *parent, *child;

    rep = name;
    rep += match('(');

    /* if its an empty list, just go ahead and exit */
    if (look() == ')') {
        rep += match(')');
        return data.list(name, rep);
    }

    /* parse all existing arguments next */
    do {
        if (look() == ',') /* for first argument in list */
            match(',');
        child = expr(data);
        rep += child->representation();
        children.push_back(child);
        /* skip so look() won't be extraneous whitespace */
        skipwhitespace();
    } while (look() == ',');

    rep += match(')');
    parent = data.list(name, rep);

    for (i = 0; i < children.size(); i++)
        parent->add_child(children.at(i));

    return parent;
}

/*
 * Read the next expression which is a function with zero, one, or more
 * expressions inside or simply a variable.
 * <expr> := <var> | <func> | <name>
 */
Datum *
expr (Data &data)
{
    std::string rep;
    skipwhitespace();

    if (look() == '?') {
        return var(data);
    } else {
        rep = name();
        if (look() == '(')
            return func(data, rep);
        else
            return data.atom(rep);
    }
}

/*
 * <unification> := <expr> = <expr>
 */
void
unification (Data &data)
{
    Datum *a, *b;

    a = expr(data);
    skipwhitespace();

    if (look() != '=')
        goto done;

    match('=');
    b = expr(data);

    unify(a, b);

done:
    printf("%s\n", a->value()->representation().c_str());
}

int
main (int argc, char **argv)
{
    Data data;

    if (argc > 1)
        INPUT.str(std::string(argv[1]));
    else
        INPUT << std::cin.rdbuf();

    unification(data);

    return 0;
}
