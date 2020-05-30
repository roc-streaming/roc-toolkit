#! /usr/bin/python3

def print_header():
    print('''
/*
 * THIS FILE IS AUTO-GENERATED USING `atomic_ops_gen.py'.
 */

#ifndef ROC_CORE_ATOMIC_OPS_H_
#define ROC_CORE_ATOMIC_OPS_H_

#include "roc_core/stddefs.h"

#include <atomic_ops.h>

namespace roc {
namespace core {

//! Atomic operations.
class AtomicOps {
public:
    #ifdef AO_HAVE_nop_read
    //! Acquire memory barrier.
    static inline void fence_acquire() {
        AO_nop_read();
    }
    #endif // AO_HAVE_nop_read

    #ifdef AO_HAVE_nop_write
    //! Release memory barrier.
    static inline void fence_release() {
        AO_nop_write();
    }
    #endif // AO_HAVE_nop_write

    #ifdef AO_HAVE_nop_full
    //! Full memory barrier.
    static inline void fence_seq_cst() {
        AO_nop_full();
    }
    #endif // AO_HAVE_nop_full
'''.strip())

def print_load(keys):
    print('''
    #ifdef AO_HAVE{family}_load{fence_impl}
    //! Atomic load ({fence_name} barrier).
    {qualifier} {type_func} load_{fence_func}({type_func} const& var) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        return ({type_func})AO{family}_load{fence_impl}(({type_impl} const*)&var);
    }}
    #endif // AO_HAVE{family}_load{fence_impl}
'''.rstrip().format(**keys))

def print_store(keys):
    print('''
    #ifdef AO_HAVE{family}_store{fence_impl}
    //! Atomic store ({fence_name} barrier).
    {qualifier} void store_{fence_func}({type_func}& var, {type_func} val) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        AO{family}_store{fence_impl}(({type_impl}*)&var, ({type_impl})val);
    }}
    #endif // AO_HAVE{family}_store{fence_impl}
'''.rstrip().format(**keys))

def print_exchange(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_compare_and_swap{fence_impl}
    //! Atomic exchange ({fence_name} barrier).
    {qualifier} {type_func} exchange_{fence_func}({type_func}& var, {type_func} val) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        {type_impl} curr = AO{family}_load(({type_impl}*)&var);
        {type_impl} prev;
        do {{
            prev = curr;
            curr = AO{family}_fetch_compare_and_swap{fence_impl}(({type_impl}*)&var, prev,
                                                         ({type_impl})val);
        }} while (curr != prev);
        return ({type_func})curr;
    }}
    #endif // AO_HAVE{family}_fetch_compare_and_swap{fence_impl}
'''.rstrip().format(**keys))

def print_exchange_manual_fence(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_compare_and_swap
    //! Atomic exchange ({fence_name} barrier).
    {qualifier} {type_func} exchange_{fence_func}({type_func}& var, {type_func} val) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        AO_nop{fence_impl}();
        {type_impl} curr = AO{family}_load(({type_impl}*)&var);
        {type_impl} prev;
        do {{
            prev = curr;
            curr = AO{family}_fetch_compare_and_swap(({type_impl}*)&var, prev,
                                                         ({type_impl})val);
            AO_nop{fence_impl}();
        }} while (curr != prev);
        return ({type_func})curr;
    }}
    #endif // AO_HAVE{family}_fetch_compare_and_swap
'''.rstrip().format(**keys))

def print_compare_exchange(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_compare_and_swap{fence_impl}
    //! Atomic compare-and-swap ({fence_name} barrier).
    {qualifier} bool compare_exchange_{fence_func}(
          {type_func}& var, {type_func}& exp, {type_func} des) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        {type_impl} old = AO{family}_fetch_compare_and_swap{fence_impl}(
          ({type_impl}*)&var, ({type_impl})exp, ({type_impl})des);
        const bool ret = (({type_impl})exp == old);
        exp = ({type_func})old;
        return ret;
    }}
    #endif // AO_HAVE{family}_fetch_compare_and_swap{fence_impl}
'''.rstrip().format(**keys))

def print_compare_exchange_manual_fence(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_compare_and_swap
    //! Atomic compare-and-swap ({fence_name} barrier).
    {qualifier} bool compare_exchange_{fence_func}(
          {type_func}& var, {type_func}& exp, {type_func} des) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        AO_nop{fence_impl}();
        {type_impl} old = AO{family}_fetch_compare_and_swap(
          ({type_impl}*)&var, ({type_impl})exp, ({type_impl})des);
        const bool ret = (({type_impl})exp == old);
        if (ret) {{
            AO_nop{fence_impl}();
        }}
        exp = ({type_func})old;
        return ret;
    }}
    #endif // AO_HAVE{family}_fetch_compare_and_swap
'''.rstrip().format(**keys))

def print_add_fetch(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_and_add{fence_impl}
    //! Atomic add-and-fetch ({fence_name} barrier).
    {qualifier} {type_func} add_fetch_{fence_func}({type_func}& var, {type_arg} val) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        return ({type_func})(
            AO{family}_fetch_and_add{fence_impl}(({type_impl}*)&var, ({type_impl})val)
              + ({type_impl})val);
    }}
    #endif // AO_HAVE{family}_fetch_and_add{fence_impl}
'''.rstrip().format(**keys))

def print_sub_fetch(keys):
    print('''
    #ifdef AO_HAVE{family}_fetch_and_add{fence_impl}
    //! Atomic sub-and-fetch ({fence_name} barrier).
    {qualifier} {type_func} sub_fetch_{fence_func}({type_func}& var, {type_arg} val) {{
        struct type_check {{
            int f : sizeof({type_func}) == sizeof({type_impl}) ? 1 : -1;
        }};
        return ({type_func})(
            AO{family}_fetch_and_add{fence_impl}(({type_impl}*)&var, ({type_impl})-val)
              - ({type_impl})val);
    }}
    #endif // AO_HAVE{family}_fetch_and_add{fence_impl}
'''.rstrip().format(**keys))

def print_methods(**kw):
    print('''
    // overloads for {type_func}
'''.rstrip().format(**kw))

    kw['fence_name'] = 'no'
    kw['fence_func'] = 'relaxed'
    kw['fence_impl'] = ''
    print_load(kw)
    print_store(kw)
    print_exchange(kw)
    print_compare_exchange(kw)
    print_add_fetch(kw)
    print_sub_fetch(kw)

    kw['fence_name'] = 'acquire'
    kw['fence_func'] = 'acquire'
    kw['fence_impl'] = '_acquire'
    print_load(kw)
    print_exchange(kw)
    print_compare_exchange(kw)
    print_add_fetch(kw)
    print_sub_fetch(kw)

    kw['fence_name'] = 'release'
    kw['fence_func'] = 'release'
    kw['fence_impl'] = '_release'
    print_store(kw)
    print_exchange(kw)
    print_compare_exchange(kw)
    print_add_fetch(kw)
    print_sub_fetch(kw)

    kw['fence_name'] = 'acquire-release'
    kw['fence_func'] = 'acq_rel'
    kw['fence_impl'] = '_full'
    print_exchange(kw)
    print_compare_exchange(kw)

    kw['fence_name'] = 'full'
    kw['fence_func'] = 'seq_cst'
    kw['fence_impl'] = '_full'
    print_load(kw)
    print_store(kw)
    print_exchange_manual_fence(kw)
    print_compare_exchange_manual_fence(kw)
    print_add_fetch(kw)
    print_sub_fetch(kw)

def print_footer():
    print('''
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_OPS_H_
'''.strip())

print_header()

print_methods(
    qualifier='static inline',
    family='_char',
    type_func='unsigned char',
    type_arg='unsigned char',
    type_impl='unsigned char')

print_methods(
    qualifier='static inline',
    family='_char',
    type_func='char',
    type_arg='char',
    type_impl='unsigned char')

print_methods(
    qualifier='static inline',
    family='_short',
    type_func='unsigned short',
    type_arg='unsigned short',
    type_impl='unsigned short')

print_methods(
    qualifier='static inline',
    family='_short',
    type_func='short',
    type_arg='short',
    type_impl='unsigned short')

print()
print('#ifndef AO_T_IS_INT')

print_methods(
    qualifier='static inline',
    family='_int',
    type_func='unsigned int',
    type_arg='unsigned int',
    type_impl='unsigned int')

print_methods(
    qualifier='static inline',
    family='_int',
    type_func='int',
    type_arg='int',
    type_impl='unsigned int')

print()
print('#endif // AO_T_IS_INT')

print_methods(
    qualifier='static inline',
    family='',
    type_func='size_t',
    type_arg='size_t',
    type_impl='AO_t')

print_methods(
    qualifier='static inline',
    family='',
    type_func='ssize_t',
    type_arg='ssize_t',
    type_impl='AO_t')

print_methods(
    qualifier='template <class T> static inline',
    family='',
    type_func='T*',
    type_arg='ptrdiff_t',
    type_impl='AO_t')

print_footer()
