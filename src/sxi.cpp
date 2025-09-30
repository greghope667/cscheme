#include "string.hpp"
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
        default:
            return false;
    }
}
