#include "gc.hpp"
#include "sxi.hpp"
#include "code.hpp"
#include "match.hpp"

using namespace sxi;

// Code builder with dynamically growing buffers to store
// function data. Information not needed at runtime is then
// discarded with finish()

struct Builder {
    static constexpr int OFFSET_MAX = INT16_MAX;
    vector<opcode> instructions;
    vector<ProtoLambda> lambdas;
    vector<Symbol*> symbols;
    vector<SXI> literals;

    template <typename T>
    void _append_object(vector<T>& array, T obj) {
        int i = array.length;
        if (i >= OFFSET_MAX)
            error("compile limit error: too many objects in function");
        array.push(obj);
        append((opcode)i);
    }

    template <typename T>
    void _append_object_check_dup(vector<T>& array, T obj) {
        for (int i=0; i<array.length; i++) {
            if (obj == array[i])
                return append((opcode)i);
        }
        return _append_object(array, obj);
    }

    void append(opcode op) {
        if (instructions.length >= OFFSET_MAX)
            error("compile limit error: too many instructions in function");
        instructions.push(op);
    }

    void append(int16_t op) {
        append((opcode)op);
    }

    void append(Symbol* sym) {
        _append_object_check_dup(symbols, sym);
    }

    void append(ProtoLambda l) {
        _append_object(lambdas, l);
    }

    void append(SXI s) {
        _append_object_check_dup(literals, s);
    }

    template <typename...Ts>
    void appends(Ts...ts) {
        (append(ts), ...);
    }

    Code* finish() {
        auto take = []<typename T>(vector<T>& v) {
            v.shrink_to_fit();
            T* ptr = v.data;
            v = {};
            return ptr;
        };

        Code* c = gc_alloc<Code>();
        c->insns_len = instructions.length;
        c->insns = take(instructions);

        c->symbols_len = symbols.length;
        c->symbols = take(symbols);

        c->lambdas_len = lambdas.length;
        c->lambdas = take(lambdas);

        c->literals_len = literals.length;
        c->literals = take(literals);
        return c;
    }

    void dealloc() {
        instructions.dealloc();
        symbols.dealloc();
        lambdas.dealloc();
        literals.dealloc();
    }
};

/// Recursive compiler functions ///

static Code* compile_to_code(SXI expr, int csp);

static void compile_expr    (Builder*, bool is_tail, int csp, SXI expr);
static void compile_list    (Builder*, bool is_tail, int csp, Pair* list);
static void compile_literal (Builder*, bool is_tail, int csp, SXI literal);
static void compile_symbol  (Builder*, bool is_tail, int csp, Symbol* sym);

static void compile_if      (Builder*, bool is_tail, int begin, int end);
static void compile_begin   (Builder*, bool is_tail, int begin, int end);
static void compile_define  (Builder*, bool is_tail, int begin, int end);
static void compile_set     (Builder*, bool is_tail, int begin, int end);
static void compile_quote   (Builder*, bool is_tail, int begin, int end);
static void compile_lambda  (Builder*, bool is_tail, int begin, int end);

struct special_form { 
    Symbol* name; 
    void (*compile)(Builder*, bool is_tail, int begin, int end); 
};
static special_form special_forms[] = {
    { make_symbol("if"),     compile_if     },
    { make_symbol("begin"),  compile_begin  },
    { make_symbol("define"), compile_define },
    { make_symbol("set!"),   compile_set    },
    { make_symbol("quote"),  compile_quote  },
    { make_symbol("lambda"), compile_lambda },
};

// Stack for flattening lists into arrays during the compile process

static SXI* compile_stack;
static int compile_stack_length;

static int flatten_list(int sp, Pair* list) {
    if (compile_stack_length == 0) {
        compile_stack = (SXI*)malloc(4 * sizeof(SXI));
        compile_stack_length = 4;
    }

    for (;;) {
        if (sp == compile_stack_length) {
            compile_stack_length *= 2;
            compile_stack = (SXI*)realloc(
                compile_stack, 
                compile_stack_length * sizeof(SXI)
            );
        }
        auto [first, rest] = *list;
        compile_stack[sp++] = first;

        if (rest == c_null)
            return sp;

        if (not instance<Pair>(rest))
            error("compile syntax error: invalid list");

        list = as<Pair>(rest);
    }
}

static Formals* parse_formals(SXI arg_list) {
    auto formals = gc_alloc<Formals>();
    *formals = { .names = {}, .is_variadic = false };

    for (;;) {
        match(arg_list) {
            case_pair2(head, rest) {
                if (not instance<Symbol>(head)) 
                    error("compile syntax error: invalid formals");
                formals->names.push(as<Symbol>(head));
                arg_list = rest;
                break;
            }
            case_sym(sym) {
                formals->names.push(sym);
                formals->is_variadic = true;
                return formals;
            }
            default: {
                if (arg_list == c_null)
                    return formals;
                error("compile syntax error: invalid formals");
            }
        }
    }
}

static void compile_expr(Builder* code, bool is_tail, int csp, SXI expr) {
    match(expr) {
        case_sym(sym)   return compile_symbol(code, is_tail, csp, sym);
        case_pair(pair) return compile_list(code, is_tail, csp, pair);
        default:        return compile_literal(code, is_tail, csp, expr);
    }
}

static void compile_literal(Builder* code, bool is_tail, int, SXI literal) {
    code->appends(op_literal, literal);
    if (is_tail)
        code->append(op_ret);
}

static void compile_symbol(Builder* code, bool is_tail, int, Symbol* sym) {
    code->appends(op_lookup, sym);
    if (is_tail)
        code->append(op_ret);
}

static void compile_list(Builder* code, bool is_tail, int csp, Pair* list) {
    int begin = csp, end = flatten_list(csp, list);

    if (instance<Symbol>(compile_stack[begin])) {
        auto sym = as<Symbol>(compile_stack[begin]);
        for (auto [name, compile_fn] : special_forms) {
            if (sym == name)
                return compile_fn(code, is_tail, begin, end);
        }
    }

    if (end - begin > INT16_MAX)
        error("compile limit error: list too long");

    if (not is_tail)
        code->append(op_alloc_cont);

    code->appends(op_alloc_stack, (int16_t)(end - begin));

    for (int i=begin; i<end; i++) {
        compile_expr(code, false, end, compile_stack[i]);
        code->appends(op_push);
    }

    code->append(is_tail ? op_tailcall : op_call);
}

static void compile_if(Builder* code, bool is_tail, int begin, int end) {
    // (if test consequent alternate)
    // (if test consequent)
    SXI alternate = [&](){
        switch (end - begin) {
            case 3: return c_void;
            case 4: return compile_stack[begin+3];
            default: error("compile syntax error: bad if form");
        }
    }();

    // test
    compile_expr(code, false, end, compile_stack[begin+1]);

    // =false, branch to alternate
    code->appends(op_branch0, INT16_MAX);
    int branch0 = code->instructions.length - 1;

    // =true, consequent
    compile_expr(code, is_tail, end, compile_stack[begin+2]);
    if (not is_tail) {
        // branch to exit
        code->appends(op_branch, INT16_MAX);
    }
    int branch = code->instructions.length - 1;

    code->instructions[branch0] =
        (opcode)(code->instructions.length - branch0 + 1);

    // alternate
    compile_expr(code, is_tail, end, alternate);

    if (not is_tail) {
        code->instructions[branch] = (opcode)(code->instructions.length - branch + 1);
    }
}

static void compile_begin(Builder* code, bool is_tail, int begin, int end) {
    // (begin exprs...)
    if (end - begin == 1)
        // zero length begin form is sxi extension
        return compile_literal(code, is_tail, end, c_void);
    for (int i=begin+1; i<end-1; i++) {
        compile_expr(code, false, end, compile_stack[i]);
    }
    compile_expr(code, is_tail, end, compile_stack[end-1]);
}

static void compile_define(Builder* code, bool is_tail, int begin, int end) {
    // (define identifier expr)
    if (end - begin != 3)
        error("compile syntax error: bad define form");

    compile_expr(code, false, end, compile_stack[begin+2]);

    if (not instance<Symbol>(compile_stack[begin+1]))
        error("compile syntax error: define identifier not a symbol");

    code->appends(op_define, as<Symbol>(compile_stack[begin+1]));

    if (is_tail) code->append(op_ret);
}

static void compile_set(Builder* code, bool is_tail, int begin, int end) {
    // (set! identifier expr)
    if (end - begin != 3)
        error("compile syntax error: bad set! form");

    compile_expr(code, false, end, compile_stack[begin+2]);

    if (not instance<Symbol>(compile_stack[begin+1]))
        error("compile syntax error: set! identifier not a symbol");

    code->appends(op_set, as<Symbol>(compile_stack[begin+1]));

    if (is_tail) code->append(op_ret);
}

static void compile_quote(Builder* code, bool is_tail, int begin, int end) {
    // (quote datum)
    if (end - begin != 2)
        error("compile syntax error: bad quote form");

    compile_literal(code, is_tail, end, compile_stack[begin+1]);
}

static void compile_lambda(Builder* code, bool is_tail, int begin, int end) {
    // (lambda (<formals>) . body)
    if (end - begin < 2)
        error("compile syntax error: bad lambda form");

    auto formals = parse_formals(compile_stack[begin+1]);

    auto code_lambda = [begin, end](){
        Builder code{};
        try {
            compile_begin(&code, true, begin+1, end);
            return code.finish();
        } catch (...) {
            code.dealloc();
            throw;
        }
    }();

    code->appends(op_lambda, ProtoLambda{code_lambda, formals});

    if (is_tail)
        code->append(op_ret);
}

static Code* compile_to_code(SXI expr, int csp) {
    Builder code{};
    try {
        compile_expr(&code, true, csp, expr);
        return code.finish();
    } catch (...) {
        code.dealloc();
        throw;
    }
}

Thunk* sxi::compile(SXI expr, Environment* env) {
    auto code = compile_to_code(expr, 0);
    auto thunk = gc_alloc<Thunk>();
    *thunk = { .code = code, .env = env };
    return thunk;
}