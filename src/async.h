#ifndef NODE_SQLITE3_SRC_ASYNC_H
#define NODE_SQLITE3_SRC_ASYNC_H

#include <napi.h>
#include <uv.h>

#include "threading.h"

#if defined(NODE_SQLITE3_BOOST_THREADING)
#include <boost/thread/mutex.hpp>
#endif


// Generic uv_async handler.
template <class Item, class Parent> class Async {
    typedef void (*Callback)(Parent* parent, Item* item);

protected:
    uv_async_t watcher;
    NODE_SQLITE3_MUTEX_t
    std::vector<Item*> data;
    Callback callback;
public:
    Parent* parent;

public:
    Async(Parent* parent_, Callback cb_)
        : callback(cb_), parent(parent_) {
        watcher.data = this;
        NODE_SQLITE3_MUTEX_INIT
        uv_loop_t *loop;
        napi_get_uv_event_loop(parent_->Env(), &loop);
        uv_async_init(loop, &watcher, reinterpret_cast<uv_async_cb>(listener));
    }

    static void listener(uv_async_t* handle) {
        Async* async = static_cast<Async*>(handle->data);
        std::vector<Item*> rows;
        NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
        rows.swap(async->data);
        NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)
        for (unsigned int i = 0, size = rows.size(); i < size; i++) {
            async->callback(async->parent, rows[i]);
        }
    }

    static void close(uv_handle_t* handle) {
        assert(handle != NULL);
        assert(handle->data != NULL);
        Async* async = static_cast<Async*>(handle->data);
        delete async;
    }

    void finish() {
        // Need to call the listener again to ensure all items have been
        // processed. Is this a bug in uv_async? Feels like uv_close
        // should handle that.
        listener(&watcher);
        uv_close((uv_handle_t*)&watcher, close);
    }

    void add(Item* item) {
        NODE_SQLITE3_MUTEX_LOCK(&mutex);
        data.push_back(item);
        NODE_SQLITE3_MUTEX_UNLOCK(&mutex)
    }

    void send() {
        uv_async_send(&watcher);
    }

    void send(Item* item) {
        add(item);
        send();
    }

    ~Async() {
        NODE_SQLITE3_MUTEX_DESTROY
    }
};

#endif
