#ifndef DATA_CPP
#define DATA_CPP

#include <algorithm>
#include "Datum.cpp"

/*
 * This class owns all memory allocated for any type of Datum. Each Datum is
 * allocated based on its representation when parsed, e.g. when parsing 5 and
 * 5, we allocated only once and both 5's use the same Datum.
 */

class Data {
public:
    Data ()
    {
        /* a set constant which represents nothing */
        NILDATUM = this->datum(Nil, "nil");
    }

    Datum *
    atom (std::string name)
    {
        return this->datum(Atom, name);
    }

    Datum *
    list (std::string name)
    {
        return this->datum(List, name);
    }

    Datum *
    var (std::string name)
    {
        return this->datum(Var, name);
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

    /* Allocate Datum once (based on name). Otherwise return its pointer */
    Datum *
    datum (DatumType type, std::string str)
    {
        std::map<std::string*, Datum*>::iterator it;
        std::string *name = this->string(str);

        /* Return the datum if it already exists */
        it = mappings.find(name);
        if (it != mappings.end())
            return it->second;

        switch (type) {
        case List:
            lists.push_back(std::vector<Datum*>());
            datums.push_back(Datum(type, name, &lists.back()));
            break;

        case Var:
            datums.push_back(Datum(type, name, this->NILDATUM));
            break;

        case Atom:
            datums.push_back(Datum(type, name, name));
            break;

        case Nil:
            datums.push_back(Datum(type, name, name));
            break;

        default:
            assert(type == Nil);
            break;
        }

        mappings.insert(std::pair<std::string*, Datum*>(name, &datums.back()));
        return &datums.back();
    }

private:
    std::list<std::vector<Datum*> > lists;
    std::list<std::string> strings;
    std::list<Datum> datums;
    std::map<std::string*, Datum*> mappings;
    Datum *NILDATUM;
};

#endif
