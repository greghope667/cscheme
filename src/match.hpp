#pragma once

#define match(form) \
    if (SXI _match = form; true) switch(_match._tag)

#define case_val(T,v) \
    case tt::tag<T>: if (T v=as<T>(_match); true)

#define case_ptr(T,v) \
    case tt::tag<T>: if (T* v=as<T>(_match); true)

#define case_const(constant) case_val(sxi_constant,constant)
#define case_int(i) case_val(integer,i)

#define case_pair(pair) case_ptr(Pair,pair)
#define case_pair2(car, cdr) \
    case SXI_TAG_pair: if (auto [car, cdr]=*as<Pair>(_match); true)

#define case_sym(sym) case_ptr(Symbol,sym)
#define case_env(env) case_ptr(Environment,env)
#define case_lambda(lambda) case_ptr(Lambda,lambda)
