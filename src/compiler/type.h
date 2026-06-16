#ifndef TICK_TYPE_H
#define TICK_TYPE_H

#include "../core/string.h"
#include "../core/dynamic_array.h"

namespace Tick {

/// How a value is held. This is the single source of truth for cost and aliasing.
enum class Ownership {
    VALUE,   // owned value (stack / inline)
    REF,     // borrow: non-owning reference (a pointer, lifetime-checked)
    SHARED,  // shared ownership, reference counted
    WEAK     // non-owning reference to a shared object
};

enum class TypeKind {
    VOID,
    BOOL,
    INT,        // signed/unsigned, sized
    FLOAT,      // f32 / f64
    STR,
    NAMED,      // struct / enum / interface by name
    ARRAY,      // dynamic array T[]
    FIXED_ARRAY,// T[N]
    DYN,        // dyn Interface (fat pointer)
    PTR,        // raw pointer (unsafe / FFI only)
    FUNC        // function value type
};

/// A fully structured type. Replaces v1's stringly-typed approach so that
/// ownership and cost are explicit and analyzable.
struct TypeRef {
    TypeKind kind;
    Ownership ownership;

    // INT
    int int_bits;       // 8/16/32/64
    bool int_unsigned;
    // FLOAT
    int float_bits;     // 32/64
    // NAMED / DYN
    String name;
    // ARRAY / FIXED_ARRAY / OPTIONAL / RESULT / PTR: element/inner type
    TypeRef* inner;
    // FIXED_ARRAY
    int fixed_size;
    // FUNC
    DynamicArray<TypeRef*> param_types;
    TypeRef* return_type;
    // mutability for REF (ref var)
    bool ref_mutable;

    TypeRef(TypeKind k)
        : kind(k), ownership(Ownership::VALUE),
          int_bits(0), int_unsigned(false), float_bits(0),
          inner(nullptr), fixed_size(0), return_type(nullptr), ref_mutable(false) {}

    ~TypeRef() {
        delete inner;
        delete return_type;
        for (size_t i = 0; i < param_types.size(); i++) delete param_types[i];
    }

    TypeRef* clone() const {
        TypeRef* t = new TypeRef(kind);
        t->ownership = ownership;
        t->int_bits = int_bits;
        t->int_unsigned = int_unsigned;
        t->float_bits = float_bits;
        t->name = name;
        t->fixed_size = fixed_size;
        t->ref_mutable = ref_mutable;
        if (inner) t->inner = inner->clone();
        if (return_type) t->return_type = return_type->clone();
        for (size_t i = 0; i < param_types.size(); i++) t->param_types.push(param_types[i]->clone());
        return t;
    }

    bool is_heap_owned() const {
        return ownership == Ownership::SHARED ||
               kind == TypeKind::ARRAY || kind == TypeKind::STR;
    }
};

}

#endif
