#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdarg>

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

Datum *
expr (Data &data);

bool
in_string (const int c, std::string s)
{
    for (unsigned i = 0; i < s.size(); i++)
        if (s[i] == c)
            return true;
    return false;
}

/* 
 * The parser is a generic recursive descent, look-ahead parser. This is the
 * lookahead token. Only `next' affects the lookahead token and does so by
 * simplying returning the next character from the stream.
 */
static int LOOK = -2;

int
next ()
{
    LOOK = getc(stdin);
    return LOOK;
}

int
look ()
{
    if (LOOK == -2)
        return next();
    else
        return LOOK;
}

std::string
match (int c)
{
    if (look() != c)
        error("Expected `%c' got `%c' instead", c, look());
    next();
    return std::string(1, c);
}

void
skipwhitespace ()
{
    while (isspace(look()))
        next();
}

bool
is_reserved (const int c)
{
    return in_string(c, "?(,)=");
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

Datum *
var (Data &data)
{
    std::string rep;
    rep = match('?');
    rep += name();
    return data.var(rep);
}

/*
 * Parse a function and all of its arguments into a Datum list.
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
 * <expr> := <name>([<expr> [, <expr> ...])
 *         | ?<name>
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
    //Datum *p1, *p2;
    //unsigned int i;
    
    unification(data);

    //if (argc < 3)
    //    error("Must have at least 2 patterns to test");

    //for (i = 2; i < argc; i++) {
    //    /* parse the first two at the same time */
    //    if (i == 2) {
    //        p1 = parse_arg(intern, argv[1]);
    //        p2 = parse_arg(intern, argv[2]);
    //    }
    //    /* parse only one because last arg is already parsed and unified */
    //    else {
    //        p1 = p2;
    //        p2 = parse_arg(intern, argv[i]);
    //    }
    //    unify(p1, p2);
    //}

    //for (i = 0; i < p2->list().size(); i++) {
    //    printf("%s", p2->list()[i]->value()->name());
    //    if (i == p2->list().size() - 1)
    //        printf("\n");
    //    else
    //        printf(" ");
    //}

    return 0;
}
