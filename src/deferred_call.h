#ifndef NODE_SQLITE3_SRC_DEFERRED_CALL_H
#define NODE_SQLITE3_SRC_DEFERRED_CALL_H

#include <v8.h>

using namespace v8;

namespace Deferred {

enum Mode {
    Concurrent,
    Exclusive
};

template <typename T> class Call {
public:
    Persistent<Function> callback_;
    Persistent<Object> receiver_;
    enum Mode mode_;
    T data_;
    int argc_;
    Persistent<Value>* argv_;

public:
    Call(const Arguments& args, const T data, Mode mode = Concurrent) {
        callback_ = Persistent<Function>::New(args.Callee());
        receiver_ = Persistent<Object>::New(args.This());
        mode_ = mode;
        data_ = data;
        argc_ = args.Length();
        argv_ = new Persistent<Value>[argc_];
        for (int i = 0; i < argc_; i++) {
            argv_[i] = Persistent<Value>::New(args[i]);
        }
    }

    ~Call() {
        for (int i = 0; i < argc_; i++) {
            argv_[i].Dispose();
        }
        delete[] argv_;
        callback_.Dispose();
        receiver_.Dispose();
    }

    inline Mode Mode() {
        return mode_;
    }

    inline const T Data() {
        return data_;
    }

    inline Local<Value> Invoke() {
        return callback_->Call(receiver_, argc_, argv_);
    }
};

}

#endif
