/*
 * Copyright (c) 2021-2022, Linus Groh <linusg@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibJS/Runtime/NativeFunction.h>

namespace JS {

class JS_API ShadowRealmConstructor final : public NativeFunction {
    JS_OBJECT(ShadowRealmConstructor, NativeFunction);
    GC_DECLARE_ALLOCATOR(ShadowRealmConstructor);

public:
    virtual void initialize(Realm&) override;
    virtual ~ShadowRealmConstructor() override = default;

    virtual ThrowCompletionOr<Value> call() override;
    virtual ThrowCompletionOr<GC::Ref<Object>> construct(FunctionObject& new_target) override;

private:
    explicit ShadowRealmConstructor(Realm&);

    virtual bool has_constructor() const override { return true; }
};

}
