// Copyright 2024 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/api/api-inl.h"
#include "test/unittests/test-utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace v8 {
namespace internal {

using DecoratorsTest = TestJSDecoratorsWithNativeContext;

TEST_F(DecoratorsTest, AddInitializer) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);
  DirectHandle<ArrayList> extra_class_initializers = factory->NewArrayList(0);
  Handle<JSFunction> add_initializer_fun =
      factory->NewDecoratorAddInitializerFunction(isolate,
                                                  extra_class_initializers);
  Handle<Object> callable = RunJS<Object>("(function(){})");
  Handle<Object> args[1] = {callable};
  MaybeHandle<Object> result =
      Execution::Call(isolate, add_initializer_fun,
                      ReadOnlyRoots(isolate).undefined_value_handle(), 1, args);
  CHECK(!result.is_null());
  callable = RunJS<Object>("(new Proxy(function(){}, {}))");
  args[0] = callable;
  result =
      Execution::Call(isolate, add_initializer_fun,
                      ReadOnlyRoots(isolate).undefined_value_handle(), 1, args);
  CHECK(!result.is_null());
  callable = RunJS<Object>("(class C{})");
  args[0] = callable;
  result =
      Execution::Call(isolate, add_initializer_fun,
                      ReadOnlyRoots(isolate).undefined_value_handle(), 1, args);
  CHECK(!result.is_null());

  // addInitializer throws when the argument is not callable.
  Handle<JSObject> non_callable = RunJS<JSObject>("({})");
  args[0] = non_callable;
  result =
      Execution::Call(isolate, add_initializer_fun,
                      ReadOnlyRoots(isolate).undefined_value_handle(), 1, args);
  CHECK(result.is_null());
  CHECK(isolate->has_exception());
  isolate->clear_exception();

  // addInitializer throws when it's context is a native context.
  add_initializer_fun->set_context(*isolate->native_context(), kReleaseStore);
  args[0] = callable;
  result =
      Execution::Call(isolate, add_initializer_fun,
                      ReadOnlyRoots(isolate).undefined_value_handle(), 1, args);
  CHECK(result.is_null());
  CHECK(isolate->has_exception());
}

TEST_F(DecoratorsTest, DecoratorAccessObject) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);
  Handle<JSDecoratorAccessObject> access_object =
      factory->NewJSDecoratorAccessObject(
          factory->NewStringFromStaticChars("x"));
  Maybe<bool> global_set_result = context()->Global()->Set(
      context(), NewString("access"), ToApiHandle<Value>(access_object));
  CHECK(global_set_result.FromJust());
  RunJS("var y = {x: 1};");
  DirectHandle<Object> result = RunJS<Object>("access.get(y)");
  CHECK_EQ(*result, *factory->NewNumber(1));
  result = RunJS<Object>("access.has(y)");
  CHECK_EQ(*result, *factory->true_value());
  result = RunJS<Object>("access.set(y, 2);");
  CHECK_EQ(*result, *factory->undefined_value());
  result = RunJS<Object>("y.x");
  CHECK_EQ(*result, *factory->NewNumber(2));
  result = RunJS<Object>("access.get(y)");
  CHECK_EQ(*result, *factory->NewNumber(2));

  RunJS("y = {x:1}; var z = {}; z.__proto__ = y;");
  result = RunJS<Object>("access.get(z)");
  CHECK_EQ(*result, *factory->NewNumber(1));
  result = RunJS<Object>("access.has(z)");
  CHECK_EQ(*result, *factory->true_value());
  result = RunJS<Object>("access.set(z, 2);");
  CHECK_EQ(*result, *factory->undefined_value());
  result = RunJS<Object>("z.x");
  CHECK_EQ(*result, *factory->NewNumber(2));
  result = RunJS<Object>("access.get(z)");
  CHECK_EQ(*result, *factory->NewNumber(2));

  RunJS("var w = {}");
  result = RunJS<Object>("access.has(w)");
  CHECK_EQ(*result, *factory->false_value());
  result = RunJS<Object>("access.set(w, 2);");
  CHECK_EQ(*result, *factory->undefined_value());
  result = RunJS<Object>("w.x");
  CHECK_EQ(*result, *factory->NewNumber(2));
  result = RunJS<Object>("access.get(w)");
  CHECK_EQ(*result, *factory->NewNumber(2));
}

TEST_F(DecoratorsTest, DecoratorAccessObjectPrivateName) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  DirectHandle<String> name = factory->NewStringFromStaticChars("#x");
  Handle<Name> private_symbol = factory->NewPrivateNameSymbol(name);
  Handle<JSDecoratorAccessObject> access_object =
      factory->NewJSDecoratorAccessObject(private_symbol);

  Maybe<bool> global_set_result = context()->Global()->Set(
      context(), NewString("access"), ToApiHandle<Value>(access_object));
  CHECK(global_set_result.FromJust());

  Handle<JSReceiver> object = RunJS<JSReceiver>("var object = ({}); object");
  Maybe<bool> crate_property_result = JSReceiver::CreateDataProperty(
      isolate, object, private_symbol, factory->true_value(), Just(kDontThrow));
  CHECK(crate_property_result.FromJust());

  Handle<Object> result = RunJS<Object>("access.get(object)");
  CHECK_EQ(*result, *factory->true_value());
  result = RunJS<Object>("access.has(object)");
  CHECK_EQ(*result, *factory->true_value());
  result = RunJS<Object>("access.set(object, false);");
  CHECK_EQ(*result, *factory->undefined_value());
  result = RunJS<Object>("access.get(object)");
  CHECK_EQ(*result, *factory->false_value());

  // Private lookups don't go up in the prototype chain.
  // get and set should throw if the private name is not found in the object,
  // has should return false.
  RunJS("var child = {}; child.__proto__ = object;");
  MaybeLocal<Value> try_result = TryRunJS("access.get(child)");
  CHECK(try_result.IsEmpty());
  Handle<Object> has_result = RunJS<Object>("access.has(child)");
  CHECK_EQ(*has_result, *factory->false_value());
  try_result = TryRunJS("access.set(child, true);");
  CHECK(try_result.IsEmpty());

  // Cannot use the accessor to acccess private properties from other objects,
  // even if their names have the same description.
  RunJS("class C {static #x = 1;}");
  try_result = TryRunJS("access.get(C)");
  CHECK(try_result.IsEmpty());
  has_result = RunJS<Object>("access.has(C)");
  CHECK_EQ(*has_result, *factory->false_value());
  try_result = TryRunJS("access.set(C, 2);");
  CHECK(try_result.IsEmpty());

  // Private lookups don't go through proxies.
  RunJS("var proxy = new Proxy(object, {});");
  try_result = TryRunJS("access.get(proxy)");
  CHECK(try_result.IsEmpty());
  has_result = RunJS<Object>("access.has(proxy)");
  CHECK_EQ(*has_result, *factory->false_value());
  try_result = TryRunJS("access.set(proxy, true);");
  CHECK(try_result.IsEmpty());
}

TEST_F(DecoratorsTest, DecoratorAccessCalledNonObject) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);
  Handle<JSDecoratorAccessObject> access_object =
      factory->NewJSDecoratorAccessObject(
          factory->NewStringFromStaticChars("x"));
  Maybe<bool> global_set_result = context()->Global()->Set(
      context(), NewString("access"), ToApiHandle<Value>(access_object));
  CHECK(global_set_result.FromJust());

  // Decorator access functions should throw if called on non-receiver.
  MaybeLocal<Value> try_result = TryRunJS("access.get(undefined)");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(undefined, true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(undefined);");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get(null)");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(null, true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(null);");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get(true)");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(true, true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(true);");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get('invalid')");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set('invalid', true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has('invalid');");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get(Symbol.split)");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(Symbol.split, true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(Symbol.split);");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get(0)");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(0, true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(0);");
  CHECK(try_result.IsEmpty());

  try_result = TryRunJS("access.get(BigInt(1))");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.set(BigInt(1), true);");
  CHECK(try_result.IsEmpty());
  try_result = TryRunJS("access.has(BigInt(1));");
  CHECK(try_result.IsEmpty());
}

TEST_F(DecoratorsTest, ClassDecoratorContextObject) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);
  DirectHandle<ArrayList> extra_class_initializers = factory->NewArrayList(0);
  Handle<JSFunction> add_initializer_fun =
      factory->NewDecoratorAddInitializerFunction(isolate,
                                                  extra_class_initializers);
  DirectHandle<String> name = factory->NewStringFromStaticChars("C");
  Handle<JSClassDecoratorContextObject> context_object =
      factory->NewJSClassDecoratorContextObject(name, add_initializer_fun);
  Maybe<bool> result = context()->Global()->Set(
      context(), NewString("context"), ToApiHandle<Value>(context_object));
  CHECK(result.FromJust());
  DirectHandle<String> class_string_result = RunJS<String>("(context.kind)");
  DirectHandle<String> name_result = RunJS<String>("(context.name)");
  DirectHandle<JSFunction> add_initializer_result =
      RunJS<JSFunction>("(context.addInitializer)");
  CHECK_EQ(*class_string_result, *factory->class_string());
  CHECK_EQ(*name_result, *name);
  CHECK_EQ(*add_initializer_result, *add_initializer_fun);
}

TEST_F(DecoratorsTest, ClassElementDecoratorContextObject) {
  Isolate* isolate = i_isolate();
  Factory* factory = isolate->factory();
  HandleScope scope(isolate);
  DirectHandle<ArrayList> extra_class_initializers = factory->NewArrayList(0);
  Handle<JSFunction> add_initializer_fun =
      factory->NewDecoratorAddInitializerFunction(isolate,
                                                  extra_class_initializers);
  DirectHandle<String> name = factory->NewStringFromStaticChars("x");
  Handle<JSDecoratorAccessObject> access_object =
      factory->NewJSDecoratorAccessObject(name);
  Handle<JSClassElementDecoratorContextObject> context_object =
      factory->NewJSClassElementDecoratorContextObject(
          factory->field_string(), access_object, factory->false_value(),
          factory->true_value(), name, add_initializer_fun);
  Maybe<bool> result = context()->Global()->Set(
      context(), NewString("context"), ToApiHandle<Value>(context_object));
  CHECK(result.FromJust());
  DirectHandle<String> kind_string_result = RunJS<String>("(context.kind)");
  DirectHandle<JSObject> access_result = RunJS<JSObject>("(context.access)");
  DirectHandle<Object> is_static_result = RunJS<Object>("(context.static)");
  DirectHandle<Object> is_private_result = RunJS<Object>("(context.private)");
  DirectHandle<String> name_result = RunJS<String>("(context.name)");
  DirectHandle<JSFunction> add_initializer_result =
      RunJS<JSFunction>("(context.addInitializer)");
  CHECK_EQ(*kind_string_result, *factory->field_string());
  CHECK_EQ(*access_result, *access_object);
  CHECK_EQ(*is_static_result, *factory->false_value());
  CHECK_EQ(*is_private_result, *factory->true_value());
  CHECK_EQ(*name_result, *factory->NewStringFromStaticChars("x"));
  CHECK_EQ(*add_initializer_result, *add_initializer_fun);
}

}  // namespace internal
}  // namespace v8
