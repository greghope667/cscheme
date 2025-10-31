#include "builtin.hpp"
#include "sxi.hpp"
#include "../match.hpp"

using namespace sxi;

static SXI make_struct_type(int argc, SXI* argv) {
    if (argc < 1)
        invalid_arguments("make-struct-type", span(argv, argc));

    auto name = as<Symbol>(argv[0]);

    auto st = new StructType{ .name = name, .field_names = {} };

    st->field_names.reserve(argc-1);
    for (int i=1; i<argc; i++)
        st->field_names.push(as<Symbol>(argv[i]));
    return wrap(st);
}

static SXI make_struct(int argc, SXI* argv) {
    if (argc < 1)
        invalid_arguments("make-struct", span(argv, argc));

    auto type = as<StructType>(argv[0]);
    auto field_count = type->field_names.length;

    if (argc != 1 + field_count)
        invalid_arguments("make-struct", span(argv, argc));

    auto s = new StructInstance{ .type = type, .fields = {} };

    s->fields.reserve(field_count);
    s->fields.length = field_count;
    memcpy(s->fields.data, &argv[1], field_count*sizeof(SXI));
    return wrap(s);
}

static int get_struct_entry_index(SXI entry, StructType* st) {
    match (entry) {
        case_int(i) return i;
        case_sym(sym) {
            for (int i=0; i<st->field_names.length; i++)
                if (sym == st->field_names[i])
                    return i;
            error_f("unknown field name %s", symbol_name(sym));
        }
        default:
            error(entry, "bad object for indexing struct");
    }
}

static SXI struct_ref(int argc, SXI* argv) {
    SXI_CHECK_ARITY(2, 3);
    auto si = as<StructInstance>(argv[0]);
    if (argc == 3 && wrap(si->type) != argv[2])
        invalid_arguments("struct-ref", span(argv, argc));

    auto index = get_struct_entry_index(argv[1], si->type);
    return si->fields[index];
}

static SXI struct_set(int argc, SXI* argv) {
    SXI_CHECK_ARITY(3, 4);
    auto si = as<StructInstance>(argv[0]);
    if (argc == 4 && wrap(si->type) != argv[3])
        invalid_arguments("struct-ref", span(argv, argc));

    auto index = get_struct_entry_index(argv[1], si->type);
    si->fields[index] = argv[2];
    return wrap(si);
}

static SXI struct_typeof(SXI s) {
    return instance<StructInstance>(s) ? wrap(as<StructInstance>(s)->type) : c_false;
}

static const function_1_def def1s[] = {
    { "struct-type?", [](SXI s) { return wrap_bool(instance<StructType>(s)); } },
    { "struct-instance?", [](SXI s) { return wrap_bool(instance<StructInstance>(s)); } },
    { "struct-typeof", struct_typeof },
};

static const function_n_def defns[] = {
    { "make-struct-type", make_struct_type },
    { "make-struct", make_struct },
    { "struct-ref", struct_ref },
    { "struct-set!", struct_set },
};

const builtin_lib sxi::builtin_lib_struct(def1s, defns);
