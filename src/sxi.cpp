#include "string.hpp"
#include "struct.hpp"
#include "match.hpp"

using namespace sxi;

char* sxi::string_data(String* s, int* len) {
    if (len) *len = s->length();
    return s->data();
}

bool sxi::equal(SXI l, SXI r) {
    if (l == r)
        return true;
    match(l) {
        case_pair(p) {
            return instance<Pair>(r) &&
                equal(p->first, car(r)) &&
                equal(p->second, cdr(r));
        }
        case_ptr(String, s) {
            return instance<String>(r) &&
                s->length() == as<String>(r)->length() &&
                memcmp(s->data(), as<String>(r)->data(), s->length()) == 0;
        }
        case_ptr(StructInstance, li) {
            if (not instance<StructInstance>(r)) return false;
            auto ri = as<StructInstance>(r);
            if (li->type != ri->type) return false;
            for (int i=0; i<li->fields.length; i++)
                if (not equal(li->fields[i], ri->fields[i]))
                    return false;
            return true;
        }
        default:
            return false;
    }
}
