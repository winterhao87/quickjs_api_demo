#include <quickjs/quickjs.h>
#include <quickjs/quickjs-libc.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif

// js pass an integer
static JSValue set_int(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 1);

  int64_t val;
  if (JS_ToInt64(js_ctx, &val, argv[0])) {
    printf("set_int: JS_ToInt64 fail\n");
  } else {
    printf("set_int: JS_ToInt64: %ld\n", val);
  }

  return JS_UNDEFINED;
}

// return an integer to js
static JSValue get_int(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 0);
  int64_t val = 1024000;

  return JS_NewInt64(js_ctx, val);
}

// js pass a string parameter.
static JSValue set_str(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 1);

  const char *val = JS_ToCString(js_ctx, argv[0]);
  if (val == NULL) {
    printf("set_str: JS_ToCString fail\n");
  } else {
    printf("set_str: JS_ToCString: %s\n", val);
    JS_FreeCString(js_ctx, val);
  }

  return JS_UNDEFINED;
}

// return string to js
static JSValue get_str(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 0);

  const char *val = "hello js from get_str";
  return JS_NewString(js_ctx, val);
}

// js pass an array
static JSValue set_array(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 1);

  if (!JS_IsArray(js_ctx, argv[0])) {
    printf("JS_IsArray fail\n");
    return JS_ThrowTypeError(js_ctx, "argv[0] not array");
  }

  JSObject *obj = JS_VALUE_GET_OBJ(argv[0]);
  assert(obj);

  // uint32_t count = obj->u.array.count;
  // JSValue iter = JS_GetIterator(js_ctx, argv[0]);

  // 1. get array's length
  JSValue length = JS_GetPropertyStr(js_ctx, argv[0], "length");
  if (JS_IsException(length)) {
    printf("set_array: setJS_GetProperty JS_ATOM_length fail\n");
    return JS_ThrowInternalError(js_ctx, "array without length property");
  }

  uint32_t len = 0;
  if (JS_ToUint32(js_ctx, &len, length)) {
    printf("set_array: JS_ToUint32 fail\n");
    return JS_UNDEFINED;
  }
  JS_FreeValue(js_ctx, length);
  printf("set_array: len=%u\n", len);

  // 2. foreach array by index
  uint32_t i = 0;
  for (i = 0; i < len; ++i) {
    JSValue val = JS_GetPropertyUint32(js_ctx, argv[0], i);
    if (JS_IsException(val)) {
      printf("set_array: JS_GetPropertyUint32 fail, i=%u\n", i);
      return JS_UNDEFINED;
    }

    const char *val_str = JS_ToCString(js_ctx, val);
    if (val_str == NULL) {
      printf("set_array: JS_ToCString fail, i=%u\n", i);
      return JS_UNDEFINED;
    }

    printf("set_array: [%u] = %s\n", i, val_str);
    JS_FreeCString(js_ctx, val_str);
    JS_FreeValue(js_ctx, val);
  }

  return JS_UNDEFINED;
}

// return an array to js
static JSValue get_array(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 0);

  JSValue arr = JS_NewArray(js_ctx);

  uint32_t idx = 0;
  for (idx = 0; idx < 3; ++idx) {
    JS_SetPropertyUint32(js_ctx, arr, idx, JS_NewUint32(js_ctx, idx * 10));
  }

  // array with diffrent data type.
  JS_SetPropertyUint32(js_ctx, arr, idx, JS_NewString(js_ctx, "hello from get_array"));

  return arr;
}

static JSValue foreach_obj(JSContext *js_ctx, JSValueConst obj)
{
  // 1. get all enumable property name.
  uint32_t len, i;
  JSPropertyEnum *tab;
  if (JS_GetOwnPropertyNames(js_ctx, &tab, &len, obj,
                             JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0) {
    return JS_ThrowTypeError(js_ctx, "JS_GetOwnPropertyNames fail");
  }
  printf("foreach_obj: GetOwnPropertyNames ok, len=%u\n", len);

  // 2. foreach object by property name.
  for (i = 0; i < len; i++) {
    JSValue val = JS_GetProperty(js_ctx, obj, tab[i].atom);
    if (JS_IsException(val)) {
      printf("foreach_object: JS_GetProperty is exception, idx=%u\n", i);
      return JS_UNDEFINED;
    }

    const char *val_str = JS_ToCString(js_ctx, val);
    if (val_str == NULL) {
      printf("foreach_object: JS_ToString fail, idx=%u\n", i);
      return JS_UNDEFINED;
    }

    const char *key_str = JS_AtomToCString(js_ctx, tab[i].atom);
    if (key_str == NULL) {
      printf("foreach_object: JS_AtomToCString fail, idx=%u\n", i);
      return JS_UNDEFINED;
    }

    // ignore js function.
    int32_t tag = JS_VALUE_GET_TAG(val);
    printf("foreach_object: %s = %s, is_object=%d, tag=%d\n", key_str, val_str, JS_IsObject(val), tag);
    if (JS_IsObject(val) && (!JS_IsFunction(js_ctx, val))) {
      foreach_obj(js_ctx, val);
    }

    JS_FreeCString(js_ctx, val_str);
    JS_FreeCString(js_ctx, key_str);
    JS_FreeValue(js_ctx, val);
  }

  for(i = 0; i < len; i++) {
    JS_FreeAtom(js_ctx, tab[i].atom);
  }
  js_free(js_ctx, tab);

  return JS_UNDEFINED;
}

static JSValue set_object(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 1);

  return foreach_obj(js_ctx, argv[0]);
}

static JSValue get_object(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 0);
  JSValue obj = JS_NewObject(js_ctx);

  JS_SetPropertyStr(js_ctx, obj, "age", JS_NewUint32(js_ctx, 100));
  JS_SetPropertyStr(js_ctx, obj, "name", JS_NewString(js_ctx, "winsenye"));
  return obj;
}

// call js function
static JSValue eval_js_func(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc > 1);
  assert(JS_IsFunction(js_ctx, argv[0]));

  return JS_Call(js_ctx, argv[0], this_val, argc - 1, argv + 1);
}

// @Note: 演示如何操作自定义类型
static JSClassID js_test_req_class_id;

typedef struct JSTestRequest {
  JSValue method;
  JSValue url;

  int is_alive;
} JSTestRequest;

static void js_test_req_finalizer(JSRuntime *rt, JSValue val)
{
  printf("enter js_test_req_finalizer\n");
  JSTestRequest *req = JS_GetOpaque(val, js_test_req_class_id);
  if (req && req->is_alive) {
    printf("js_test_req_finalizer do\n");
    req->is_alive = 0;
    JS_FreeValueRT(rt, req->method);
    JS_FreeValueRT(rt, req->url);
    js_free_rt(rt, req);
  }
}

// @Note: As we have own method and url from js, we should provide mark callback for gc.
static void js_test_req_mark(JSRuntime *rt, JSValueConst val,
                             JS_MarkFunc *mark_func)
{
  printf("enter js_test_req_mark\n");
  JSTestRequest *req = JS_GetOpaque(val, js_test_req_class_id);
  if (req && req->is_alive) {
    printf("js_test_req_mark do\n");
    JS_MarkValue(rt, req->method, mark_func);
    JS_MarkValue(rt, req->url, mark_func);
  }
}


static JSClassDef js_test_req_class = {
  "TestReq",
  .finalizer = js_test_req_finalizer,
  .gc_mark = js_test_req_mark,
}; 

// @Note: TestReq constructor 
static JSValue js_test_req_ctor(JSContext *ctx,
                             JSValueConst new_target,
                             int argc, JSValueConst *argv)
{
  JSTestRequest *req;

  printf("js_test_req_ctor begin\n");
  
  // 1. initial opaque data
  req = js_mallocz(ctx, sizeof(*req));
  if (!req)
    return JS_EXCEPTION;
  req->method = JS_DupValue(ctx, argv[0]);
  req->url = JS_DupValue(ctx, argv[1]);
  req->is_alive = 1;

  // 2. get proto
  /* using new_target to get the prototype is necessary when the class is extended. */
  JSValue proto;
  proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  if (JS_IsException(proto))
    goto fail;

  // 3. new object with proto and class_id
  JSValue obj = JS_NewObjectProtoClass(ctx, proto, js_test_req_class_id);
  JS_FreeValue(ctx, proto);

  if (JS_IsException(obj))
    goto fail;

  // 4. Associate with opaque data
  JS_SetOpaque(obj, req);
  printf("js_test_req_ctor end\n");
  return obj;

fail:
  js_free(ctx, req);
  JS_FreeValue(ctx, obj);
  return JS_EXCEPTION;
}


// return TestReq Object to js.
static JSValue get_req(JSContext *js_ctx, JSValueConst this_val,
      int argc, JSValueConst *argv)
{
  assert(argc == 2);

  if (!JS_IsString(argv[0]) || !JS_IsString(argv[1])) {
    return JS_ThrowTypeError(js_ctx, "not string");
  }

  // 1. new an object with class_id, we already set the proto at js_inject_all()
  JSValue obj = JS_NewObjectClass(js_ctx, js_test_req_class_id);
  if (JS_IsException(obj)) {
    printf("get_req: JS_NewObjectClass fail\n");
    return obj;
  }

  // 2. new an opaque data and save it with JS_SetOpaque
  JSTestRequest *req = js_mallocz(js_ctx, sizeof(JSTestRequest));
  if (req == NULL) {
    JS_FreeValue(js_ctx, obj);
    return JS_EXCEPTION;
  }

  req->is_alive = 1;
  req->method = JS_DupValue(js_ctx, argv[0]);
  req->url = JS_DupValue(js_ctx, argv[1]);

  JS_SetOpaque(obj, req);
  return obj;
}

// get the property by name for TestReq Object
static JSValue js_test_req_get(JSContext *js_ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv)
{
  assert(argc == 1);
  JSTestRequest *req = JS_GetOpaque2(js_ctx, this_val, js_test_req_class_id);
  if (req == NULL) {
    return JS_ThrowTypeError(js_ctx, "JS_GetOpaque NULL");
  }

  if (!req->is_alive) {
    return JS_ThrowTypeError(js_ctx, "req not alive");
  }

  const char *key_str = JS_ToCString(js_ctx, argv[0]);
  if (key_str == NULL) {
    return JS_ThrowTypeError(js_ctx, "not string");
  }

  if (strcmp(key_str, "method") == 0) {
    JS_FreeCString(js_ctx, key_str);
    return JS_DupValue(js_ctx, req->method);
  } else if (strcmp(key_str, "url") == 0) {
    JS_FreeCString(js_ctx, key_str);
    return JS_DupValue(js_ctx, req->url);
  } else {
    return JS_ThrowTypeError(js_ctx, "invalid key: '%s'", key_str);
  }
}

// Active release the TestReq Object.
static JSValue js_test_req_close(JSContext *js_ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv)
{
  assert(argc == 0);
  JSTestRequest *req = JS_GetOpaque2(js_ctx, this_val, js_test_req_class_id);
  if (req == NULL) {
    return JS_ThrowTypeError(js_ctx, "JS_GetOpaque NULL");
  }

  if (!req->is_alive) {
    return JS_ThrowTypeError(js_ctx, "req not alive");
  }

  JS_FreeValue(js_ctx, req->method);
  JS_FreeValue(js_ctx, req->url);
  req->is_alive = 0;
  js_free(js_ctx, req);

  printf("js_test_req_close\n");
  JS_SetOpaque(this_val, NULL);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_test_req_proto_funcs[] = {
  JS_CFUNC_DEF("get", 1, js_test_req_get),
  JS_CFUNC_DEF("close", 0, js_test_req_close),
};

static void js_inject_api(JSContext *js_ctx)
{
  JSValue global = JS_GetGlobalObject(js_ctx);

  JS_SetPropertyStr(js_ctx, global, "set_int", JS_NewCFunction(js_ctx, set_int, "set_int", 1));
  JS_SetPropertyStr(js_ctx, global, "get_int", JS_NewCFunction(js_ctx, get_int, "get_int", 0));

  JS_SetPropertyStr(js_ctx, global, "set_str", JS_NewCFunction(js_ctx, set_str, "set_str", 1));
  JS_SetPropertyStr(js_ctx, global, "get_str", JS_NewCFunction(js_ctx, get_str, "get_str", 0));

  JS_SetPropertyStr(js_ctx, global, "set_array", JS_NewCFunction(js_ctx, set_array, "set_array", 1));
  JS_SetPropertyStr(js_ctx, global, "get_array", JS_NewCFunction(js_ctx, get_array, "get_array", 0));

  JS_SetPropertyStr(js_ctx, global, "set_object", JS_NewCFunction(js_ctx, set_object, "set_object", 1));
  JS_SetPropertyStr(js_ctx, global, "get_object", JS_NewCFunction(js_ctx, get_object, "get_object", 0));

  JS_SetPropertyStr(js_ctx, global, "eval_js_func", JS_NewCFunction(js_ctx, eval_js_func, "eval_js_func", 2));


  // Alloc a class_id
  JS_NewClassID(&js_test_req_class_id);
  assert(JS_IsRegisteredClass(JS_GetRuntime(js_ctx), js_test_req_class_id) == 0);
  // New a Class
  JS_NewClass(JS_GetRuntime(js_ctx), js_test_req_class_id, &js_test_req_class);

  JSValue proto = JS_NewObject(js_ctx);
  JS_SetPropertyFunctionList(js_ctx, proto, js_test_req_proto_funcs, countof(js_test_req_proto_funcs));
  // set proto for the class
  JS_SetClassProto(js_ctx, js_test_req_class_id, proto);

  JS_SetPropertyStr(js_ctx, global, "get_req", JS_NewCFunction(js_ctx, get_req, "get_req", 0));

  JSValue req_class = JS_NewCFunction2(js_ctx, js_test_req_ctor, "TestReq", 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(js_ctx, req_class, proto); // set proto for the constructor function.
  JS_SetPropertyStr(js_ctx, global, "TestReq", req_class);


  JS_FreeValue(js_ctx, global);
}

int main(int argc, char **argv)
{
  JSRuntime *js_rt;
  JSContext *js_ctx;

  js_rt = JS_NewRuntime();
  assert(js_rt);
  JS_SetMemoryLimit(js_rt, 1024 * 1024 * 2);
  JS_SetMaxStackSize(js_rt, 1024 * 1024);

  js_ctx = JS_NewContext(js_rt);
  assert(js_ctx);

  // js_std_init_handlers(js_rt);

  /* loader for ES6 modules */
  JS_SetModuleLoaderFunc(js_rt, NULL, js_module_loader, NULL);

  JS_SetHostPromiseRejectionTracker(js_rt, js_std_promise_rejection_tracker, NULL);

  js_std_add_helpers(js_ctx, 0, NULL);

  /* system modules */
  js_init_module_std(js_ctx, "std");
  // js_init_module_os(js_ctx, "os");

  js_inject_api(js_ctx);

  const char *js_file_name = "./api.js";
  size_t js_file_len = 0;
  uint8_t *js_file_data = js_load_file(js_ctx, &js_file_len, js_file_name);
  if (js_file_data == NULL) {
    printf("js_load_file fail\n");
    return -1;
  }

  // @Note: we can cache the compile result.
  int eval_flags = JS_EVAL_TYPE_MODULE;
  JSValue val = JS_Eval(js_ctx, (const char *)js_file_data, js_file_len, js_file_name, eval_flags | JS_EVAL_FLAG_COMPILE_ONLY);
  if (JS_IsException(val)) {
    printf("compile fail:\n");
    js_std_dump_error(js_ctx);
    return -1;
  }

  js_module_set_import_meta(js_ctx, val, 1, 1);
  val = JS_EvalFunction(js_ctx, val);
  if (JS_IsException(val)) {
    printf("eval fail:\n");
    js_std_dump_error(js_ctx);
    return -1;
  }

  const char *val_str = JS_ToCString(js_ctx, val);
  if (val_str) {
    printf("eval done: %s\n", val_str);
    JS_FreeCString(js_ctx, val_str);
  } else {
    printf("eval done\n");
  }
  JS_FreeValue(js_ctx, val);
  js_free(js_ctx, js_file_data);
  js_std_loop(js_ctx);


  //  js_std_free_handlers(js_rt);
  JS_FreeContext(js_ctx);
  JS_FreeRuntime(js_rt);
  return 0;
}