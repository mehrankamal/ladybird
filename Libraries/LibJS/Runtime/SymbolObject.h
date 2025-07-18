/*
 * Copyright (c) 2020, Matthew Olsson <mattco@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibJS/Runtime/Object.h>
#include <LibJS/Runtime/Symbol.h>

namespace JS {

class JS_API SymbolObject : public Object {
    JS_OBJECT(SymbolObject, Object);
    GC_DECLARE_ALLOCATOR(SymbolObject);

public:
    static GC::Ref<SymbolObject> create(Realm&, Symbol&);

    virtual ~SymbolObject() override = default;

    Symbol& primitive_symbol() { return m_symbol; }
    Symbol const& primitive_symbol() const { return m_symbol; }

private:
    SymbolObject(Symbol&, Object& prototype);

    virtual void visit_edges(Visitor&) override;

    GC::Ref<Symbol> m_symbol;
};

}
