#ifndef DATUM_CPP
#define DATUM_CPP

#include <cassert>
#include <string>
#include <vector>
#include <list>
#include <map>

enum DatumType {
    Nil,  /* A value that represents the void that an unbound Var points to */
    List, /* A list of Datum objects */
    Atom, /* An actual value: numbers, a string, etc. */
    Var   /* A variable which can be assigned Datum (which is just Datum) */
};

/*
 * A structure that can hold different types of data determined by DatumType.
 */
class Datum {
private:
    DatumType datatype;
    std::string *representation;
    union {
        Datum *datum;
        std::string *string;
        std::vector<Datum*> *list;
    } slot;

public:
    /* Create a Variable linked to some other Datum (likely `NILDATUM') */
    Datum (DatumType type, std::string *name, Datum* val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Var);
        slot.datum = val;
    }

    /* Create Nil and Atom types. Their values are their representation */
    Datum (DatumType type, std::string *name, std::string *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == Atom || datatype == Nil);
        slot.string = val;
    }

    /* Create a list of other Datum */
    Datum (DatumType type, std::string *name, std::vector<Datum*> *val)
        : datatype(type)
        , representation(name)
    {
        assert(datatype == List);
        slot.list = val;
    }

    DatumType
    type ()
    {
        return datatype;
    }

    static
    const char *
    typestr (DatumType type)
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

    const std::vector<Datum*>&
    list ()
    {
        assert(datatype == List);
        return *slot.list;
    }

    Datum*
    var ()
    {
        assert(datatype == Var);
        return slot.datum;
    }

    /*
     * Update existing Datum objects.
     */

    void
    set (Datum *data)
    {
        assert(datatype == Var);
        slot.datum = data;
    }

    void
    add_child (Datum *val)
    {
        assert(datatype == List);
        slot.list->push_back(val);
    }

    /* 
     * Returns the representation of this Datum object when parsed in (e.g. ?x,
     * 5, foo). Because all information exists at parse time, all Vars will be
     * set to atomic type or `Nil'. So, the representation of the unified
     * pattern can be seen by calling `name` on each Datum object.
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
     * Get the value of a Datum object. All atomic types are their own values
     * All unbound Vars will return themselves (which `name' returns its own
     * representation, e.g. ?x or ?foo). Otherwise, following the linked-list
     * of vars will return an atomic (non-nil) object.
     */
    Datum *
    value ()
    {
        Datum *d = this;
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


#endif
