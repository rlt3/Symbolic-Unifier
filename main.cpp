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
    std::string rep;
    rep = match('?');
    rep += name();
    return data.var(rep);
}

Datum * expr (Data &data);

/*
 * Parse a function and all of its arguments into a Datum list.
 * <func> := <name>([<expr> [, <expr> ...])
 */
Datum *
func (Data &data)
{
    Datum *parent = data.list(name());

    match('(');

    /* if its an empty list, just go ahead and exit */
    if (look() == ')')
        goto exit;

    /* parse the first expression in the argument list */
    parent->add_child(expr(data));
    /* skip so look() won't be extraneous whitespace */
    skipwhitespace();

    /* parse all existing arguments next */
    if (look() == ',') {
        do {
            match(',');
            parent->add_child(expr(data));
            skipwhitespace();
        } while (look() == ',');
    }

exit:
    match(')');
    return parent;
}

/*
 * Read the next expression which is a function with zero, one, or more
 * expressions inside or simply a variable.
 * <expr> := <func> | <var>
 */
Datum *
expr (Data &data)
{
    Datum *d;
    skipwhitespace();

    if (look() == '?')
        d = var(data);
    else
        d = func(data);

    return d;
}

/*
 * <unification> := <expr> = <expr>
 */
void
unification (Data &data)
{
    Datum *pat1, *pat2;

    pat1 = expr(data);
    skipwhitespace();
    match('=');
    pat2 = expr(data);

    printf("pat1: '%s'\n", pat1->representation().c_str());
    printf("pat2: '%s'\n", pat2->representation().c_str());
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
