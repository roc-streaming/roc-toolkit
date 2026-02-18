#! /usr/bin/env python3

def print_template(keys, template):
    template = template.lstrip('\n')
    template = template.rstrip()
    print(template.format(**keys))

def print_header():
    print_template({}, '''
// THIS FILE IS AUTO-GENERATED. DO NOT EDIT!
// See atomic_ops_gen.py

#ifndef ROC_CORE_ATOMIC_OPS_H_
#define ROC_CORE_ATOMIC_OPS_H_

#include "roc_core/stddefs.h"

#include <atomic_ops.h>

namespace roc {{
namespace core {{

//! Atomic operations.
class AtomicOps {{
public:
    #if defined(AO_HAVE_nop_read)
    static inline void fence_acquire() {{
        AO_nop_read();
    }}
    #endif

    #if defined(AO_HAVE_nop_write)
    static inline void fence_release() {{
        AO_nop_write();
    }}
    #endif

    #if defined(AO_HAVE_nop_full)
    static inline void fence_seq_cst() {{
        AO_nop_full();
    }}
    #endif
''')

def print_load(keys):
    print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type} const& var) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        return ({var_type})AO{ao_family}_load{ao_fence}(({ao_type} const*)&var);
    }}
    ''')

def print_store(keys):
    print_template(keys, '''
    static inline void {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        AO{ao_family}_store{ao_fence}(
            ({ao_type}*)&var,
            ({ao_type})static_cast<{var_type}>(val));
    }}
    ''')

def print_exchange(keys):
    if keys['fence_name'] == 'seq_cst':
        print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        {ao_type} curr = AO{ao_family}_load{ao_fence}(({ao_type}*)&var);
        {ao_type} prev;
        do {{
            prev = curr;
            curr = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
                ({ao_type}*)&var,
                prev,
                ({ao_type})static_cast<{var_type}>(val));
            AO_nop{ao_fence}();
        }} while (curr != prev);

        return ({var_type})curr;
    }}
        ''')
    else:
        print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        {ao_type} curr = AO{ao_family}_load(({ao_type}*)&var);
        {ao_type} prev;
        do {{
            prev = curr;
            curr = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
                ({ao_type}*)&var,
                prev,
                ({ao_type})static_cast<{var_type}>(val));
        }} while (curr != prev);

        return ({var_type})curr;
    }}
        ''')

def print_compare_exchange(keys):
    if keys['fence_name'] == 'seq_cst':
        print_template(keys, '''
    static inline bool {op_name}_{fence_name}({var_type}& var, {var_type}& exp, {val_type} des) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        AO_nop{ao_fence}();
        {ao_type} old = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
            ({ao_type}*)&var,
            ({ao_type})exp,
            ({ao_type})static_cast<{var_type}>(des));
        AO_nop{ao_fence}();

        const bool success = ({ao_type})exp == old;
        exp = ({var_type})old;

        return success;
    }}
        ''')
    else:
        print_template(keys, '''
    static inline bool {op_name}_{fence_name}({var_type}& var, {var_type}& exp, {val_type} des) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        {ao_type} old = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
            ({ao_type}*)&var,
            ({ao_type})exp,
            ({ao_type})static_cast<{var_type}>(des));

        const bool success = ({ao_type})exp == old;
        exp = ({var_type})old;

        return success;
    }}
        ''')

def print_fetch_add(keys):
    print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        return ({var_type})AO{ao_family}_fetch_and_add{ao_fence}(
            ({ao_type}*)&var,
            ({ao_type})static_cast<{arith_type}>(val));
    }}
    ''')

def print_fetch_sub(keys):
    print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        return ({var_type})AO{ao_family}_fetch_and_add{ao_fence}(
            ({ao_type}*)&var,
            ({ao_type})static_cast<{arith_type}>(-val));
    }}
    ''')

def print_fetch_bitop(keys):
    if keys['fence_name'] == 'seq_cst':
        print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        {ao_type} curr = AO{ao_family}_load{ao_fence}(({ao_type}*)&var);
        {ao_type} prev;
        do {{
            prev = curr;
            curr = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
                ({ao_type}*)&var,
                prev,
                prev {bit_operator} ({ao_type})static_cast<{arith_type}>(val));
            AO_nop{ao_fence}();
        }} while (curr != prev);

        return ({var_type})curr;
    }}
        ''')
    else:
        print_template(keys, '''
    static inline {var_type} {op_name}_{fence_name}({var_type}& var, {val_type} val) {{
        static_assert(sizeof({var_type}) == sizeof({ao_type}), "type mismatch");

        {ao_type} curr = AO{ao_family}_load(({ao_type}*)&var);
        {ao_type} prev;
        do {{
            prev = curr;
            curr = AO{ao_family}_fetch_compare_and_swap{ao_fence}(
                ({ao_type}*)&var,
                prev,
                prev {bit_operator} ({ao_type})static_cast<{arith_type}>(val));
        }} while (curr != prev);

        return ({var_type})curr;
    }}
        ''')

OPS = [
    {
        'op_name': 'load',
        'op_ao': 'load',
        'op_fn': print_load,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'store',
        'op_ao': 'store',
        'op_fn': print_store,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'exchange',
        'op_ao': 'fetch_compare_and_swap',
        'op_fn': print_exchange,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'compare_exchange',
        'op_ao': 'fetch_compare_and_swap',
        'op_fn': print_compare_exchange,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'acquire_relaxed', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'release_relaxed', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'acq_rel_relaxed', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst_relaxed', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'fetch_add',
        'op_ao': 'fetch_and_add',
        'op_fn': print_fetch_add,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'fetch_sub',
        'op_ao': 'fetch_and_add',
        'op_fn': print_fetch_sub,
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'fetch_and',
        'op_ao': 'fetch_compare_and_swap',
        'op_fn': print_fetch_bitop,
        'bit_operator': '&',
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'fetch_or',
        'op_ao': 'fetch_compare_and_swap',
        'op_fn': print_fetch_bitop,
        'bit_operator': '|',
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
    {
        'op_name': 'fetch_xor',
        'op_ao': 'fetch_compare_and_swap',
        'op_fn': print_fetch_bitop,
        'bit_operator': '^',
        'supported_fences': [
            { 'fence_name': 'relaxed', 'ao_fence': '' },
            { 'fence_name': 'acquire', 'ao_fence': '_acquire' },
            { 'fence_name': 'release', 'ao_fence': '_release' },
            { 'fence_name': 'acq_rel', 'ao_fence': '_full' },
            { 'fence_name': 'seq_cst', 'ao_fence': '_full' },
        ]
    },
]

TYPES = [
    {
        'ao_family': '_char',
        'ao_type': 'unsigned char',
        'var_type': 'unsigned char',
        'arith_type': 'unsigned char',
    },
    {
        'ao_family': '_char',
        'ao_type': 'unsigned char',
        'var_type': 'signed char',
        'arith_type': 'signed char',
    },
    {
        'ao_family': '_short',
        'ao_type': 'unsigned short',
        'var_type': 'unsigned short',
        'arith_type': 'unsigned short',
    },
    {
        'ao_family': '_short',
        'ao_type': 'unsigned short',
        'var_type': 'short',
        'arith_type': 'short',
    },
    {
        'guard': 'defined(AO_T_IS_INT)',
        'ao_family': '',
        'ao_type': 'AO_t',
        'var_type': 'unsigned int',
        'arith_type': 'unsigned int',
    },
    {
        'guard': 'defined(AO_T_IS_INT)',
        'ao_family': '',
        'ao_type': 'AO_t',
        'var_type': 'int',
        'arith_type': 'int',
    },
    {
        'guard': '!defined(AO_T_IS_INT)',
        'ao_family': '_int',
        'ao_type': 'unsigned int',
        'var_type': 'unsigned int',
        'arith_type': 'unsigned int',
    },
    {
        'guard': '!defined(AO_T_IS_INT)',
        'ao_family': '_int',
        'ao_type': 'unsigned int',
        'var_type': 'int',
        'arith_type': 'int',
    },
    {
        'ao_family': '',
        'ao_type': 'AO_t',
        'var_type': 'size_t',
        'arith_type': 'size_t',
    },
    {
        'ao_family': '',
        'ao_type': 'AO_t',
        'var_type': 'ssize_t',
        'arith_type': 'ssize_t',
    },
]

def print_op_begin(keys):
    if keys.get('guard'):
        print_template(keys, '''
    #if defined(AO_HAVE{ao_family}_{op_ao}{ao_fence}) && {guard}
        ''')
    else:
        print_template(keys, '''
    #if defined(AO_HAVE{ao_family}_{op_ao}{ao_fence})
        ''')

    if not keys.get('ptr_type'):
        print_template(keys, '''
    template <class {val_type} = {var_type}>
        ''')
    else:
        print_template(keys, '''
    template <class {ptr_type}, class {val_type} = {var_type}>
        ''')

def print_op_end(keys):
    print_template(keys, '''
    #endif
    ''')

def print_int_ops():
    for op_keys in OPS:
        for fence_keys in op_keys['supported_fences']:
            for type_keys in TYPES:
                keys = op_keys.copy()
                keys.update(fence_keys)
                keys.update(type_keys)
                keys.update({
                    'val_type': 'T',
                })

                print()
                print_op_begin(keys)
                keys['op_fn'](keys)
                print_op_end(keys)

def print_ptr_ops():
    for op_keys in OPS:
        if op_keys.get('bit_operator'):
            continue
        for fence_keys in op_keys['supported_fences']:
            keys = op_keys.copy()
            keys.update(fence_keys)
            keys.update({
                'ao_family': '',
                'ao_type': 'AO_t',
                'ptr_type': 'P',
                'var_type': 'P*',
                'val_type': 'T',
                'arith_type': 'ptrdiff_t',
            })

            print()
            print_op_begin(keys)
            keys['op_fn'](keys)
            print_op_end(keys)

def print_footer():
    print_template({}, '''
}};

}} // namespace core
}} // namespace roc

#endif // ROC_CORE_ATOMIC_OPS_H_
''')

print_header()
print_int_ops()
print_ptr_ops()
print_footer()
