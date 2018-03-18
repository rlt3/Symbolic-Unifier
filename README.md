# A Symbolic Unifier

This program is a symbolic unifier meant to unify two expressions into one 
expression if possible. If the unification isn't possible then the program will
output why it wasn't possible to `stderr` and fail. Otherwise, the program will
output the unified expression on `stdout` and return successfully.

## How do I use it?

Simply clone the repository and run `make` in the directory to compile the
program. There's no dependencies and was written conforming to C++98.

The program accepts a string of characters on `stdin` or as the first argument
and parses them. The program accepts single expressions, e.g. `f(?x)`, and will
output them without error. To unify two expressions assert them with `=`, e.g.
`f(?x) = f(5)`. That will return `f(5)`.

Variables are defined as any constant prefixed with a `?`. A variable function
isn't not allowed, e.g. `?x()`. Constants are any value that's not a function,
e.g. `5' or 'a'. And a function is just a name with parenthesis wrapping
arguments, e.g. `f(?x)`, 'g(?y, f(?z))', `h()`.

Don't forget to give the entire expression as a string, e.g. `./unify "f(?x)"`.

## Why?

I needed this unifier for a larger project. I also really wanted to build a
parser for a somewhat mathematical language.

I really enjoyed how the specific data isn't parsed for anything other than a
pointer value. For the expression 'f(?x, 5) = f(?y, ?x)` the values `f`,
`?x`, `?y` and '5' all have a different pointer values, but they are chained so
that `?y` becomes the value of `?x' and `?x` becomes `5'. There's no parsing
different for explicit data types like integers. Simply test for equality with
pointers.
