﻿#include <m_pd.h>
#include <g_canvas.h>
#include <libplatform/libplatform.h>
#include <v8.h>
#include <v8-version-string.h>
#if WIN32
#include <Windows.h>
#include <process.h>
#include <comdef.h>
#endif
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>

using namespace std;

static t_class* js_class;
static t_class* js_inlet_class;
static unique_ptr<v8::Platform> js_platform;
static v8::Isolate* js_isolate;
static unordered_set<v8::Persistent<v8::Object>*> jsobjects;
static v8::Eternal<v8::Object>* js_global = nullptr;

struct _js_inlet;

typedef struct _js
{
    t_object x_obj;
    t_canvas* canvas;
    string dir;
    string path;
    vector<_js_inlet*> inlets;
    vector<_outlet*> outlets;
    v8::Persistent<v8::Context>* context;
    vector<t_atom> args;
    int inlet = 0;
    string messagename;
} t_js;

typedef struct _js_inlet
{
    t_class* pd;
    t_js* owner;
    int index;
    _inlet* inlet;
} t_js_inlet;

typedef struct _js_weakcallbackinfo
{
    t_js* x;
    v8::Persistent<v8::Object>* jso;
} t_js_weakcallbackinfo;

static string js_object_to_string(v8::Isolate* isolate, v8::Local<v8::Value> value)
{
    v8::String::Utf8Value utf8_value(isolate, value);
    return utf8_value.length() > 0 ? string(*utf8_value) : string();
}

// Reads a file into a v8 string.
static v8::MaybeLocal<v8::String> js_readfile(v8::Isolate* isolate, const char *name) {
    FILE* file = fopen(name, "rb");
    if (file == NULL) return v8::MaybeLocal<v8::String>();

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    auto chars = std::make_unique<char[]>(size + 1);
    chars.get()[size] = '\0';
    for (size_t i = 0; i < size;) {
        i += fread(&chars.get()[i], 1, size - i, file);
        if (ferror(file)) {
            fclose(file);
            return v8::MaybeLocal<v8::String>();
        }
    }
    fclose(file);
    v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
        isolate, chars.get(), v8::NewStringType::kNormal, static_cast<int>(size));
    return result;
}

static string js_get_exception_msg(v8::Isolate* isolate, const v8::TryCatch* try_catch) {
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    const char* exception_string = *exception;
    v8::Local<v8::Message> message = try_catch->Message();
    ostringstream os;
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        os << exception_string << "\n";
    }
    else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate,
            message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        const char* filename_string = *filename;
        int linenum = message->GetLineNumber(context).FromJust();
        os << filename_string << ":" << linenum << ": " << exception_string << "\n";
        // Print line of source code.
        v8::String::Utf8Value sourceline(
            isolate, message->GetSourceLine(context).ToLocalChecked());
        const char* sourceline_string = *sourceline;
        os << sourceline_string << "\n";
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            os << " ";
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            os << "^";
        }
        os << "\n";
        v8::Local<v8::Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
            stack_trace_string->IsString() &&
            v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0)
        {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char* stack_trace_str = *stack_trace;
            os << stack_trace_str << "\n";
        }
    }

    return os.str();
}

static v8::MaybeLocal<v8::Value> js_marshal_atom(const t_atom* atom)
{
    t_atomtype type = atom->a_type;

    if (type == A_FLOAT)
    {
        return v8::Number::New(js_isolate, atom_getfloat(atom));
    }
    else if (type == A_SYMBOL)
    {
        v8::Local<v8::String> s;
        if (v8::String::NewFromUtf8(js_isolate, atom_getsymbol(atom)->s_name).ToLocal(&s))
            return s;
    }

    return v8::Local<v8::Value>();
}

static v8::MaybeLocal<v8::Value> js_marshal_object(const t_atom* a, const t_js* x)
{
    string ps(atom_getsymbol(a)->s_name);
    try
    {
        auto p = stoull(ps);
        auto jso = reinterpret_cast<v8::Persistent<v8::Object>*>(p);
        if (jsobjects.find(jso) != jsobjects.end())
            return jso->Get(js_isolate);
    }
    catch (...) {}

    return v8::MaybeLocal<v8::Value>();
}

static vector<v8::Local<v8::Value>> js_marshal_args(int argc, const t_atom* argv, const t_js* x)
{
    vector<v8::Local<v8::Value>> args;

    for (int i = 0; i < argc; i++)
    {
        v8::Local<v8::Value> val;
        auto atom = argv[i];

        if (atom.a_type == A_SYMBOL && atom.a_w.w_symbol == gensym("jsobject")
            && i < (argc - 1) && argv[i + 1].a_type == A_SYMBOL
            && js_marshal_object(&argv[i + 1], x).ToLocal(&val))
        {
            args.push_back(val);
            i++;
        }

        if (js_marshal_atom(&(argv[i])).ToLocal(&val))
        {
            args.push_back(val);
        }
    }

    return args;
}

static void js_get(v8::Local<v8::Name> property,
    const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(info.Data());
    auto x = (t_js*)f->Value();
    auto name = js_object_to_string(js_isolate, property);

    if (name == "inlets")
    {
        info.GetReturnValue().Set((int32_t)(x->inlets.size()));
    }
    else if (name == "outlets")
    {
        info.GetReturnValue().Set((int32_t)x->outlets.size());
    }
    else if (name == "inlet")
    {
        info.GetReturnValue().Set(x->inlet);
    }
    else if (name == "messagename")
    {
        v8::Local<v8::String> messagename;
        if (v8::String::NewFromUtf8(js_isolate, x->messagename.c_str()).ToLocal(&messagename))
        {
            info.GetReturnValue().Set(messagename);
        }
    }
    else if (name == "jsarguments")
    {
        vector<v8::Local<v8::Value>> args = js_marshal_args((int)x->args.size(), x->args.data(), x);
        info.GetReturnValue().Set(v8::Array::New(js_isolate, args.data(), args.size()));
    }
    else if (name == "__global__")
    {
        info.GetReturnValue().Set(js_global->Get(js_isolate));
    }
}

static void js_set_inlets(t_js* x, int inlets)
{
    if (inlets < 1) inlets = 1;

    if ((int)x->inlets.size() > inlets)
    {
        for (auto i = inlets; i < (int)x->inlets.size(); i++)
        {
            inlet_free(x->inlets[i]->inlet);
        }

        x->inlets.resize(inlets);
    }
    else if ((int)x->inlets.size() < inlets)
    {
        for (auto i = (int)x->inlets.size(); i < inlets; i++)
        {
            auto inlet = (t_js_inlet*)getbytes(sizeof(t_js_inlet));
            inlet->pd = js_inlet_class;
            inlet->owner = x;
            inlet->index = i;
            inlet->inlet = inlet_new(&x->x_obj, &inlet->pd, 0, 0);
            x->inlets.push_back(inlet);
        }
    }
}

static void js_set_outlets(t_js* x, int outlets)
{
    if (outlets < 0) outlets = 0;

    if ((int)x->outlets.size() > outlets)
    {
        for (int i = outlets; i < (int)x->outlets.size(); i++)
        {
            outlet_free(x->outlets[i]);
        }

        x->outlets.resize(outlets);
    }
    else if ((int)x->outlets.size() < outlets)
    {
        for (auto i = (int)x->outlets.size(); i < outlets; i++)
        {
            auto outlet = outlet_new(&x->x_obj, 0);
            x->outlets.push_back(outlet);
        }
    }
}

static void js_set(v8::Local<v8::Name> property, v8::Local<v8::Value> value,
    const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(info.Data());
    auto x = (t_js*)f->Value();
    auto name = js_object_to_string(js_isolate, property);

    if (name == "inlets")
    {
        int inlets;

        if (value->Int32Value(x->context->Get(js_isolate)).To(&inlets))
        {
            js_set_inlets(x, inlets);
        }
    }
    else if (name == "outlets")
    {
        int outlets;

        if (value->Int32Value(x->context->Get(js_isolate)).To(&outlets))
        {
            js_set_outlets(x, outlets);
        }
    }
}

static void js_post(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);

    for (int i = 0; i < args.Length(); i++)
    {
        v8::Local<v8::Value> arg = args[i];
        v8::String::Utf8Value value(isolate, arg);
        if (i == 0)
            startpost("%s", *value);
        else
            poststring(*value);
    }

    endpost();
}

static void js_cpost(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);

    string str;

    for (int i = 0; i < args.Length(); i++)
    {
        v8::Local<v8::Value> arg = args[i];
        v8::String::Utf8Value value(isolate, arg);
        if (i > 0) str.append(" ");
        str.append(*value);
    }

    printf("%s\n", str.c_str());
}

static void js_error(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(args.Data());
    const t_js* x = (t_js*)f->Value();

    string err;

    for (int i = 0; i < args.Length(); i++)
    {
        v8::Local<v8::Value> arg = args[i];
        v8::String::Utf8Value value(isolate, arg);
        if (i > 0) err.append(" ");
        err.append(*value);
    }

    pd_error(x, "%s", err.c_str());
}

static tuple<string, intptr_t> js_unmarshal_string(v8::Isolate* isolate, v8::Local<v8::Value> value, t_js* x = NULL)
{
    if (value->IsString() || value->IsStringObject())
    {
        return std::make_tuple(js_object_to_string(isolate, value), 0);
    }
    else if (value->IsObject())
    {
        // check if we already have a persistent handle to this object in the set
        for (auto elem : jsobjects)
        {
            if (value == elem->Get(isolate))
            {
                return std::make_tuple(string("jsobject"), reinterpret_cast<intptr_t>(elem));
            }
        }

        auto o = v8::Local<v8::Object>::Cast(value);
        auto jso = new v8::Persistent<v8::Object>(js_isolate, o);
        auto wcbi = new t_js_weakcallbackinfo();

        wcbi->x = x;
        wcbi->jso = jso;

        jsobjects.insert(jso);

        jso->SetWeak(wcbi, [](const v8::WeakCallbackInfo<t_js_weakcallbackinfo>& data)
        {
            auto wcbi = data.GetParameter();
            if (wcbi != nullptr)
            {
                jsobjects.erase(wcbi->jso);
                if (wcbi->jso != nullptr)
                    delete wcbi->jso;
                delete wcbi;
            }
        }, v8::WeakCallbackType::kParameter);

        return std::make_tuple(string("jsobject"), reinterpret_cast<intptr_t>(jso));
    }

    return std::make_tuple(string(), 0);
}

static vector<t_atom> js_unmarshal_arg(v8::Local<v8::Value> arg, v8::Isolate* isolate, v8::Local<v8::Context> context, t_js* x)
{
    vector<t_atom> args;

    if (arg->IsNumber())
    {
        double num;
        if (arg->NumberValue(context).To(&num))
        {
            t_atom a = {};
            SETFLOAT(&a, (t_float)num);
            args.push_back(a);
        }
    }
    else if (arg->IsArray())
    {
        auto array = v8::Local<v8::Array>::Cast(arg);
        for (uint32_t i = 0; i < array->Length(); i++)
        {
            v8::Local<v8::Value> subval;
            if (array->Get(context, i).ToLocal(&subval))
            {
                auto subs = js_unmarshal_arg(subval, isolate, context, x);
                args.insert(args.end(), subs.begin(), subs.end());
            }
        }
    }
    else
    {
        auto t = js_unmarshal_string(isolate, arg, x);

        t_atom a = {};
        SETSYMBOL(&a, gensym(get<0>(t).c_str()));
        args.push_back(a);

        auto p = get<1>(t);

        if (p > 0)
        {
            t_atom ap = {};
            SETSYMBOL(&ap, gensym(to_string(p).c_str()));
            args.push_back(ap);
        }
    }

    return args;
}

static vector<t_atom> js_unmarshal_args(vector<v8::Local<v8::Value>> &args, v8::Isolate* isolate, v8::Local<v8::Context> context, t_js* x)
{
    vector<t_atom> argv;

    for (int i = 0; i < (int)args.size(); i++)
    {
        v8::Local<v8::Value> arg = args[i];
        auto uma = js_unmarshal_arg(arg, isolate, context, x);
        argv.insert(argv.end(), uma.begin(), uma.end());
    }

    return argv;
}

static t_symbol* js_get_type(vector<t_atom> &argv)
{
    auto &first = argv[0];
    t_symbol* type = NULL;

    if (argv.size() == 1)
    {
        switch (first.a_type)
        {
        case A_FLOAT:
            type = &s_float;
            break;
        case A_SYMBOL:
            if (first.a_w.w_symbol == gensym("bang"))
                type = &s_bang;
            else
                type = first.a_w.w_symbol;
            break;
        default:
            type = NULL;
            break;
        }
    }
    else
    {
        if (first.a_type != A_SYMBOL || first.a_w.w_symbol == gensym("list"))
            type = &s_list;
        else
            type = first.a_w.w_symbol;
    }

    return type;
}

static void js_outlet_args(_outlet* outlet, vector<v8::Local<v8::Value>> &args, t_js* x)
{
    v8::Isolate* isolate = js_isolate;
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    auto argv = js_unmarshal_args(args, isolate, context, x);

    if (!argv.empty())
    {
        auto type = js_get_type(argv);

        if (type != NULL)
        {
            if (type == argv[0].a_w.w_symbol)
                argv.erase(argv.begin());

            outlet_anything(outlet, type, (int)argv.size(), argv.data());
        }
    }
}

static void js_outlet(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(args.Data());
    auto x = (t_js*)f->Value();
    int32_t outlet_num;

    if (args[0]->Int32Value(context).To(&outlet_num)
        && outlet_num < (int32_t)x->outlets.size())
    {
        _outlet* outlet = x->outlets[outlet_num];
        vector<v8::Local<v8::Value>> argv;

        for (int i = 1; i < args.Length(); i++)
        {
            v8::Local<v8::Value> arg = args[i];
            argv.push_back(arg);
        }

        if (!argv.empty())
        {
            js_outlet_args(outlet, argv, x);
        }
    }
}

static void js_messnamed(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::String> symbolString;
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(args.Data());
    auto x = (t_js*)f->Value();

    if (args[0]->ToString(context).ToLocal(&symbolString))
    {
        auto symbol = js_object_to_string(js_isolate, symbolString);
        t_symbol *sym = gensym(symbol.c_str());

        if (sym->s_thing != NULL)
        {
            vector<t_atom> argv;

            for (int i = 1; i < args.Length(); i++)
            {
                v8::Local<v8::Value> arg = args[i];
                auto uma = js_unmarshal_arg(arg, isolate, context, x);
                argv.insert(argv.end(), uma.begin(), uma.end());
            }

            if (!argv.empty())
            {
                auto type = js_get_type(argv);

                if (type != NULL)
                {
                    if (type == argv[0].a_w.w_symbol)
                        argv.erase(argv.begin());

                    pd_typedmess(sym->s_thing, type, (int)argv.size(), argv.data());
                }
            }
        }
    }
}

struct js_file
{
    string path;
    string dir;
};

static js_file js_getfile(const t_js *x, const char* script_name)
{
    js_file result;
    const t_symbol* canvas_dir = canvas_getdir(x->canvas);
    char dirresult[MAXPDSTRING];
    char* nameresult;
    int fd = open_via_path(canvas_dir->s_name, script_name, "", dirresult, &nameresult, sizeof(dirresult), 1);

    if (fd < 0)
    {
        pd_error(&x->x_obj, "Script file '%s' not found.", script_name);
        return result;
    }

    sys_close(fd);

    result.dir = string(dirresult);
    result.path = string(dirresult);
    result.path.append("/");
    result.path.append(nameresult);

    return result;
}

static t_js* js_load(t_js* x, const char* script_name, bool create_context, const v8::Local<v8::Object>* global);

static void js_include(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) return;

    v8::Isolate* isolate = args.GetIsolate();
    v8::HandleScope scope(isolate);
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(args.Data());
    auto x = (t_js*)f->Value();

    if (args.Length() > 0 && args[0]->IsString())
    {
        auto script_name = js_object_to_string(isolate, args[0]);
        v8::Local<v8::Object> global;
        if (args.Length() > 1 && args[1]->IsObject())
            global = v8::Local<v8::Object>::Cast(args[1]);
        js_load(x, script_name.c_str(), false, (global.IsEmpty() ? NULL : &global));
    }
}

static void js_require(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    v8::Isolate* isolate = args.GetIsolate();
    v8::EscapableHandleScope scope(isolate);
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    v8::Local<v8::External> f = v8::Local<v8::External>::Cast(args.Data());
    auto x = (t_js*)f->Value();

    if (args.Length() > 0 && args[0]->IsString())
    {
        // pass { exports: {} };
        auto script_name = js_object_to_string(isolate, args[0]);
        v8::Local<v8::Object> o = v8::Object::New(isolate);
        auto exportsKey = v8::String::NewFromUtf8Literal(isolate, "exports");
        if (!(o->Set(context, exportsKey, v8::Object::New(isolate)).IsNothing())
            && js_load(x, script_name.c_str(), false, &o) != NULL)
        {
            v8::Local<v8::Value> exports;
            if (o->Get(context, exportsKey).ToLocal(&exports))
            {
                args.GetReturnValue().Set(scope.Escape(exports));
                return;
            }
        }
    }

    args.GetReturnValue().SetUndefined();
}

static t_js *js_load(t_js* x, const char *script_name = NULL, bool create_context = true, const v8::Local<v8::Object> *global = NULL)
{
    string path;

    if (script_name != NULL)
    {
        auto file = js_getfile(x, script_name);

        if (file.dir.size() != 0)
        {
            if (create_context)
            {
                x->dir = file.dir;
                x->path = file.path;
            }

            path = file.path;
        }
    }
    else
    {
        path = x->path;
    }

    {
        v8::Isolate::Scope isolate_scope(js_isolate);

        v8::HandleScope handle_scope(js_isolate);
        v8::Local<v8::Context> context;

        if (create_context)
        {
            v8::Local<v8::ObjectTemplate> global_templ = v8::ObjectTemplate::New(js_isolate);
            global_templ->SetHandler(v8::NamedPropertyHandlerConfiguration(js_get, js_set, nullptr, nullptr, nullptr, v8::External::New(js_isolate, x)));
            global_templ->Set(js_isolate, "post", v8::FunctionTemplate::New(js_isolate, js_post));
            global_templ->Set(js_isolate, "error", v8::FunctionTemplate::New(js_isolate, js_error, v8::External::New(js_isolate, x)));
            global_templ->Set(js_isolate, "cpost", v8::FunctionTemplate::New(js_isolate, js_cpost));
            global_templ->Set(js_isolate, "outlet", v8::FunctionTemplate::New(js_isolate, js_outlet, v8::External::New(js_isolate, x)));
            global_templ->Set(js_isolate, "include", v8::FunctionTemplate::New(js_isolate, js_include, v8::External::New(js_isolate, x)));
            global_templ->Set(js_isolate, "require", v8::FunctionTemplate::New(js_isolate, js_require, v8::External::New(js_isolate, x)));
            global_templ->Set(js_isolate, "messnamed", v8::FunctionTemplate::New(js_isolate, js_messnamed, v8::External::New(js_isolate, x)));

            context = v8::Context::New(js_isolate, nullptr, global_templ);
            if (x->context == nullptr)
                x->context = new v8::Persistent<v8::Context>(js_isolate, context);
            else
                x->context->Reset(js_isolate, context);

            context->SetSecurityToken(v8::Int32::New(js_isolate, 0));
        }
        else
        {
            context = x->context->Get(js_isolate);
        }

        v8::Context::Scope context_scope(context);
        {
            if (js_global == nullptr)
            {
                js_global = new v8::Eternal<v8::Object>(js_isolate, v8::Object::New(js_isolate));
            }

            v8::Local<v8::String> source;

            if (path.empty())
                return x;

            if (!js_readfile(js_isolate, path.c_str()).ToLocal(&source))
            {
                pd_error(&x->x_obj, "Error reading '%s'.", path.c_str());
                return x;
            }

            v8::TryCatch trycatch(js_isolate);
            v8::Local<v8::String> file_name = v8::String::NewFromUtf8(js_isolate, path.c_str(), v8::NewStringType::kNormal).ToLocalChecked();
            v8::ScriptOrigin origin(file_name);

            if (global == NULL)
            {
                v8::Local<v8::Script> script;

                if (!v8::Script::Compile(context, source, &origin).ToLocal(&script))
                {
                    pd_error(&x->x_obj, "Error compiling '%s':\n%s", path.c_str(), js_get_exception_msg(js_isolate, &trycatch).c_str());
                    return x;
                }

                v8::Local<v8::Value> result;
                if (!script->Run(context).ToLocal(&result))
                {
                    pd_error(&x->x_obj, "Error running '%s':\n%s", path.c_str(), js_get_exception_msg(js_isolate, &trycatch).c_str());
                    return x;
                }
            }
            else
            {
                v8::Local<v8::Function> function;
                v8::ScriptCompiler::Source src(source, origin);
                v8::Local<v8::Object> args[] = { *global };

                if (!v8::ScriptCompiler::CompileFunctionInContext(context, &src, 0, NULL, 1, args).ToLocal(&function))
                {
                    pd_error(&x->x_obj, "Error compiling '%s':\n%s", path.c_str(), js_get_exception_msg(js_isolate, &trycatch).c_str());
                    return x;
                }

                v8::Local<v8::Value> result;
                if (!function->Call(context, *global, 0, NULL).ToLocal(&result))
                {
                    pd_error(&x->x_obj, "Error running '%s':\n%s", path.c_str(), js_get_exception_msg(js_isolate, &trycatch).c_str());
                    return x;
                }
            }
        }
    }

    return x;
}

static void js_free(t_js* x)
{
    if (x->context != nullptr)
    {
        x->context->Reset();
        delete x->context;
        x->context = nullptr;
    }

    x->~t_js();
}

#if WIN32
static void js_menu_open(t_js* x)
{
    if (!x->path.empty())
    {
        ShellExecute(0, 0, x->path.c_str(), 0, 0, SW_SHOW);
        auto error = GetLastError();

        if (error != 0)
        {
            _com_error error(error);
            LPCTSTR errorText = error.ErrorMessage();
            pd_error(&x->x_obj, "%s: %s", x->path.c_str(), errorText);
        }
    }
}
#endif

static void js_anything(t_js_inlet* inlet, const t_symbol* s, int argc, const t_atom* argv)
{
    const char* name = s == &s_float ? "msg_float" : s->s_name;
    auto msgname = string(name);
    auto x = inlet->owner;
    v8::HandleScope handle_scope(js_isolate);
    auto context = x->context->Get(js_isolate);
    v8::Context::Scope context_scope(context);

    if (msgname == "compile")
    {
        if (argc > 0 && argv[0].a_type == A_SYMBOL)
        {
            x->args.clear();
            x->args.insert(x->args.end(), argv, &argv[argc]);
            js_set_inlets(x, 1);
            js_set_outlets(x, 1);
            js_load(x, atom_getsymbol(&argv[0])->s_name);
        }
        else
        {
            js_load(x);
        }
    }
    else if (msgname == "setprop")
    {
        v8::Local<v8::Value> propName;

        if (argc > 1 && js_marshal_atom(&argv[0]).ToLocal(&propName) && propName->IsName())
        {
            vector<v8::Local<v8::Value>> args = js_marshal_args(argc - 1, &argv[1], x);
            v8::Local<v8::Value> val;

            if (args.size() > 1)
                val = v8::Array::New(js_isolate, args.data(), args.size());
            else
                val = args[0];

            bool result;

            if (!context->Global()->Set(context, propName, val).To(&result))
            {
                pd_error(&x->x_obj, "Error setting property '%s'.", js_object_to_string(js_isolate, propName).c_str());
            }
        }
    }
    else if (msgname == "getprop")
    {
        v8::Local<v8::Value> propName;

        if (argc > 0 && js_marshal_atom(&argv[0]).ToLocal(&propName) && propName->IsName())
        {
            v8::Local<v8::Value> val;

            if (context->Global()->Get(context, propName).ToLocal(&val) && !val->IsUndefined()
                && !x->outlets.empty())
            {
                vector<v8::Local<v8::Value>> args;
                args.push_back(val);
                js_outlet_args(x->outlets[0], args, x);
            }
        }
    }
    else if (msgname == "delprop")
    {
        v8::Local<v8::Value> propName;

        if (argc > 0 && js_marshal_atom(&argv[0]).ToLocal(&propName) && propName->IsName()
            && !context->Global()->Delete(context, v8::Local<v8::Name>::Cast(propName)).IsNothing())
                return;
    }
#if WIN32
    else if (msgname == "open")
    {
        js_menu_open(x);
    }
#endif
    else
    {
        v8::Local<v8::String> funcName;

        if (v8::String::NewFromUtf8(js_isolate, name).ToLocal(&funcName))
        {
            auto fallback = msgname != "loadbang";
            v8::Local<v8::Value> funcVal;
            auto hasFunc = context->Global()->Get(context, funcName).ToLocal(&funcVal) && funcVal->IsFunction();

            if (!hasFunc && fallback && v8::String::NewFromUtf8(js_isolate, "anything").ToLocal(&funcName))
                hasFunc = context->Global()->Get(context, funcName).ToLocal(&funcVal) && funcVal->IsFunction();

            if (hasFunc)
            {
                vector<v8::Local<v8::Value>> args;
                auto argi = 0;

                if (msgname == "jsobject")
                {
                    v8::Local<v8::Value> val;

                    if (argc >= 1 && argv[0].a_type == A_SYMBOL
                        && js_marshal_object(&argv[0], x).ToLocal(&val))
                    {
                        args.push_back(val);
                        argi++;
                    }
                }

                vector<v8::Local<v8::Value>> margs = js_marshal_args(argc + argi, &argv[argi], x);

                args.insert(args.end(), margs.begin(), margs.end());

                v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(funcVal);
                v8::TryCatch trycatch(js_isolate);
                v8::Local<v8::Value> result;
                x->inlet = inlet->index;
                x->messagename = msgname;

                v8::Local<v8::String> privateName;
                v8::Local<v8::Value> privateVal;
                int32_t isPrivate;
                if (!v8::String::NewFromUtf8(js_isolate, "private").ToLocal(&privateName)
                    || !func->GetRealNamedProperty(context, privateName).ToLocal(&privateVal)
                    || !privateVal->Int32Value(context).To(&isPrivate)
                    || isPrivate != 1)
                {
                    if (!func->Call(context, context->Global(), (int)args.size(), args.data()).ToLocal(&result))
                    {
                        pd_error(&x->x_obj, "Error calling '%s':\n%s", name, js_get_exception_msg(js_isolate, &trycatch).c_str());
                    }
                }
                else
                {
                    pd_error(&x->x_obj, "Function '%s' is private.", name);
                }
            }
            else if (fallback)
            {
                pd_error(&x->x_obj, "Function '%s' does not exist.", name);
            }
        }
    }
}

static void js_loadbang(t_js* x, t_floatarg action)
{
    if (action == LB_LOAD)
    {
        t_js_inlet inlet = {};
        inlet.index = 0;
        inlet.owner = x;
        js_anything(&inlet, gensym("loadbang"), 0, NULL);
    }
}

static t_js* js_new(const t_symbol*, int argc, t_atom* argv)
{
    auto x = (t_js*)pd_new(js_class);
    new(x) t_js; // C++ placement new

    x->context = nullptr;
    x->path = "";
    x->canvas = canvas_getcurrent();
    x->args.clear();
    x->args.insert(x->args.end(), argv, &argv[argc]);

    const char* script_name = argc >= 1 && argv[0].a_type == A_SYMBOL ? argv[0].a_w.w_symbol->s_name : nullptr;

    js_set_inlets(x, 1);
    js_set_outlets(x, 1);

    if (js_load(x, script_name) == NULL)
        return NULL;

    return x;
}

extern "C" void js_setup(void)
{
    t_class* c = NULL;

#ifdef WIN32_SHARED
    char js_path[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&js_setup, &hm) == 0)
    {
        int ret = GetLastError();
        error("GetModuleHandle failed, error = %d\n", ret);
        return;
    }
    if (GetModuleFileName(hm, js_path, sizeof(js_path)) == 0)
    {
        int ret = GetLastError();
        error("GetModuleFileName failed, error = %d\n", ret);
        return;
    }

    v8::V8::InitializeICUDefaultLocation(js_path);
    v8::V8::InitializeExternalStartupData(js_path);
#endif

    js_platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(js_platform.get());
    v8::V8::Initialize();

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    js_isolate = v8::Isolate::New(create_params);

    c = class_new(gensym("js-inlet"), 0, 0, sizeof(t_js_inlet), CLASS_PD, A_NULL);
    if (c)
    {
        class_addanything(c, (t_method)js_anything);
    }
    js_inlet_class = c;

    c = class_new(gensym("js"), (t_newmethod)js_new, (t_method)js_free, sizeof(t_js), CLASS_NOINLET, A_GIMME, 0);
    class_addmethod(c, (t_method)js_loadbang, gensym("loadbang"), A_DEFFLOAT, 0);
#if WIN32
    class_addmethod(c, (t_method)js_menu_open, gensym("menu-open"), A_NULL);
#endif

    js_class = c;

    post("pdjs version " V8_S(VERSION) " (v8 version " V8_VERSION_STRING ")");
}
