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
    return in_string(c, "(),?");
}

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

std::string
name ()
{
    std::string s;

    printf("NAME: LOOK: %c\n", look());

    /* Skip all whitespace */
    while (isspace(look()))
        next();

    printf("NAME: LOOK-WHITE: %c\n", look());

    /* Read the whitespace-delimited string */
    while (!(is_reserved(look()) || isspace(look()) || look() == EOF)) {
        s += look();
        next();
    }

    printf("NAME: RETURN: '%s'\n", s.c_str());

    return s;
}

std::string
expr ()
{
    std::string representation;

    if (look() == '?') {
        representation = match('?');
        representation += name();
    } else {
        representation = name();
        representation += match('(');
        representation += expr();
        representation += match(')');
    }

    return representation;
}

void
parse (Data &data)
{
    printf("expr: '%s'\n", expr().c_str());
    //std::string s = name();
    //while (!s.empty()) {
    //    printf("%s\n", s.c_str());
    //    if (LOOK == '(')
    //        printf("(");
    //    if (LOOK == ')')
    //        printf(")");
    //    s = name();
    //}
}

int
main (int argc, char **argv)
{
    Data data;
    //Datum *p1, *p2;
    //unsigned int i;
    
    parse(data);

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
