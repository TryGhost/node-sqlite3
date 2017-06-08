#ifndef NODE_SQLITE3_SRC_MACROS_H
#define NODE_SQLITE3_SRC_MACROS_H

const char* sqlite_code_string(int code);
const char* sqlite_authorizer_string(int type);


#define REQUIRE_ARGUMENTS(n)                                                   \
    if (info.Length() < (n)) {                                                 \
        return Napi::TypeError::New(env, "Expected " #n "arguments").ThrowAsJavaScriptException();                \
    }


#define REQUIRE_ARGUMENT_EXTERNAL(i, var)                                      \
    if (info.Length() <= (i) || !info[i].IsExternal()) {                      \
        return Napi::TypeError::New(env, "Argument " #i " invalid").ThrowAsJavaScriptException();                 \
    }                                                                          \
    Napi::External var = Napi::External::Cast(info[i]);


#define REQUIRE_ARGUMENT_FUNCTION(i, var)                                      \
    if (info.Length() <= (i) || !info[i].IsFunction()) {                      \
        return Napi::TypeError::New(env, "Argument " #i " must be a function").ThrowAsJavaScriptException();      \
    }                                                                          \
    Napi::Function var = Napi::Function::Cast(info[i]);


#define REQUIRE_ARGUMENT_STRING(i, var)                                        \
    if (info.Length() <= (i) || !info[i].IsString()) {                        \
        return Napi::TypeError::New(env, "Argument " #i " must be a string").ThrowAsJavaScriptException();        \
    }                                                                          \
    std::string var = info[i].As<Napi::String>();


#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                                     \
    Napi::Function var;                                                       \
    if (info.Length() > i && !info[i].IsUndefined()) {                        \
        if (!info[i].IsFunction()) {                                          \
            return Napi::TypeError::New(env, "Argument " #i " must be a function").ThrowAsJavaScriptException();  \
        }                                                                      \
        var = Napi::Function::Cast(info[i]);                                  \
    }


#define OPTIONAL_ARGUMENT_INTEGER(i, var, default)                             \
    int var;                                                                   \
    if (info.Length() <= (i)) {                                                \
        var = (default);                                                       \
    }                                                                          \
    else if (info[i]-.IsNumber()) {                                             \
        var = info[i].As<Napi::Number>().Int32Value();                            \
    }                                                                          \
    else {                                                                     \
        return Napi::TypeError::New(env, "Argument " #i " must be an integer").ThrowAsJavaScriptException();      \
    }


#define DEFINE_CONSTANT_INTEGER(target, constant, name)                        \
    target->DefineProperty(                                                     \
        Napi::New(env, #name),                                      \
        Napi::Number::New(env, constant),                                           \
        static_cast<napi_property_attributes>(napi_enumerable | napi_configurable)                  \
    );

#define DEFINE_CONSTANT_STRING(target, constant, name)                         \
    target->DefineProperty(                                                     \
        Napi::New(env, #name),                                      \
        Napi::New(env, constant),                                   \
        static_cast<napi_property_attributes>(napi_enumerable | napi_configurable)                  \
    );


#define NODE_SET_GETTER(target, name, function)                                \
    Napi::SetAccessor((target)->InstanceTemplate(),                             \
        Napi::New(env, name), (function));

#define GET_STRING(source, name, property)                                     \
    std::string name = (source,                                      \
        Napi::New(env).Get(prop.As<Napi::String>()));

#define GET_INTEGER(source, name, prop)                                        \
    int name = Napi::To<int>((source,                                   \
        Napi::New(env).Get(property)));

#define EXCEPTION(msg, errno, name)                                            \
    Napi::Value name = Exception::Error(                                      \
        String::Concat(                                                        \
            String::Concat(                                                    \
                Napi::New(env, sqlite_code_string(errno)),          \
                Napi::String::New(env, ": ")                                \
            ),                                                                 \
            (msg)                                                              \
        )                                                                      \
    );                                                                         \
    Napi::Object name ##_obj = name.As<Napi::Object>();                             \
    (name ##_obj).Set( Napi::String::New(env, "errno"), Napi::New(env, errno));\
    (name ##_obj).Set( Napi::String::New(env, "code"),                   \
        Napi::New(env, sqlite_code_string(errno)));


#define EMIT_EVENT(obj, argc, argv)                                            \
    TRY_CATCH_CALL((obj),                                                      \
        (obj,                                                          \
            Napi::String::New(env).Get("emit")).As<Napi::Function>(),\
        argc, argv                                                             \
    );

#define TRY_CATCH_CALL(context, callback, argc, argv)                          \
    (callback).MakeCallback((context), (argc), (argv))

#define WORK_DEFINITION(name)                                                  \
    static Napi::Value name(const Napi::CallbackInfo& info);                                                   \
    static void Work_Begin##name(Baton* baton);                                \
    static void Work_##name(uv_work_t* req);                                   \
    static void Work_After##name(uv_work_t* req);

#define STATEMENT_BEGIN(type)                                                  \
    assert(baton);                                                             \
    assert(baton->stmt);                                                       \
    assert(!baton->stmt->locked);                                              \
    assert(!baton->stmt->finalized);                                           \
    assert(baton->stmt->prepared);                                             \
    baton->stmt->locked = true;                                                \
    baton->stmt->db->pending++;                                                \
    int status = uv_queue_work(uv_default_loop(),                              \
        &baton->request,                                                       \
        Work_##type, reinterpret_cast<uv_after_work_cb>(Work_After##type));    \
    assert(status == 0);

#define STATEMENT_INIT(type)                                                   \
    type* baton = static_cast<type*>(req->data);                               \
    Statement* stmt = baton->stmt;

#define STATEMENT_END()                                                        \
    assert(stmt->locked);                                                      \
    assert(stmt->db->pending);                                                 \
    stmt->locked = false;                                                      \
    stmt->db->pending--;                                                       \
    stmt->Process();                                                           \
    stmt->db->Process();                                                       \
    delete baton;

#define DELETE_FIELD(field)                                                    \
    if (field != NULL) {                                                       \
        switch ((field)->type) {                                               \
            case SQLITE_INTEGER: delete (Values::Integer*)(field); break;      \
            case SQLITE_FLOAT:   delete (Values::Float*)(field); break;        \
            case SQLITE_TEXT:    delete (Values::Text*)(field); break;         \
            case SQLITE_BLOB:    delete (Values::Blob*)(field); break;         \
            case SQLITE_NULL:    delete (Values::Null*)(field); break;         \
        }                                                                      \
    }

#endif
