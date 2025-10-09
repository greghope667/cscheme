#include "sxi.hpp"
#include "tl.hpp"

namespace sxi {

struct StructType {
    Symbol* name;
    vector<Symbol*> field_names;
};
template <> struct tt::traits<StructType> : tt::tagged<SXI_TAG_struct_type>, tt::boxed {};

struct StructInstance {
    StructType* type;
    vector<SXI> fields;
};
template <> struct tt::traits<StructInstance> : tt::tagged<SXI_TAG_struct_instance>, tt::boxed {};

};
