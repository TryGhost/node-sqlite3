#ifndef NODE_SQLITE3_SRC_ASYNC_H
#define NODE_SQLITE3_SRC_ASYNC_H

#include "threading.h"

#if defined(NODE_SQLITE3_BOOST_THREADING)
#include <boost/thread/mutex.hpp>
#endif


// Generic ev_async handler.
template <class Item, class Parent> class Async {
    typedef void (*Callback)(Parent* parent, Item* item);

protected:
    ev_async watcher;
    NODE_SQLITE3_MUTEX_t
    std::vector<Item*> data;
    Callback callback;
public:
    Parent* parent;

public:
    inline Async(Parent* parent_, Callback cb_)
        : mutex(),callback(cb_), parent(parent_) {
        watcher.data = this;
        ev_async_init(&watcher, listener);
        ev_async_start(EV_DEFAULT_UC_ &watcher);
        NODE_SQLITE3_MUTEX_INIT
    }

    static void listener(EV_P_ ev_async *w, int revents) {
        Async* async = static_cast<Async*>(w->data);
        std::vector<Item*> rows;
        NODE_SQLITE3_MUTEX_LOCK(&async->mutex)
        rows.swap(async->data);
        NODE_SQLITE3_MUTEX_UNLOCK(&async->mutex)
        for (unsigned int i = 0, size = rows.size(); i < size; i++) {
            ev_unref(EV_DEFAULT_UC);
            async->callback(async->parent, rows[i]);
        }
    }

    inline void add(Item* item) {
        // Make sure node runs long enough to deliver the messages.
        ev_ref(EV_DEFAULT_UC);
        NODE_SQLITE3_MUTEX_LOCK(&mutex)
        data.push_back(item);
        NODE_SQLITE3_MUTEX_UNLOCK(&mutex)
    }

    inline void send() {
        ev_async_send(EV_DEFAULT_ &watcher);
    }

    inline void send(Item* item) {
        add(item);
        send();
    }

    inline ~Async() {
        ev_invoke(EV_DEFAULT_UC_ &watcher, ev_async_pending(&watcher));
        NODE_SQLITE3_MUTEX_DESTROY
        ev_async_stop(EV_DEFAULT_UC_ &watcher);
    }
};

#endif
