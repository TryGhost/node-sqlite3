#ifndef NODE_SQLITE3_SRC_ASYNC_H
#define NODE_SQLITE3_SRC_ASYNC_H


// Generic uv_async handler.
template <class Item, class Parent> class Async {
    typedef void (*Callback)(Parent* parent, Item* item);

protected:
    uv_async_t watcher;
    pthread_mutex_t mutex;
    std::vector<Item*> data;
    Callback callback;
public:
    Parent* parent;

public:
    Async(Parent* parent_, Callback cb_)
        : callback(cb_), parent(parent_) {
        watcher.data = this;
        pthread_mutex_init(&mutex, NULL);
        uv_async_init(Loop(), &watcher, listener);
    }

    static void listener(uv_async_t* handle, int status) {
        Async* async = static_cast<Async*>(handle->data);
        std::vector<Item*> rows;
        pthread_mutex_lock(&async->mutex);
        rows.swap(async->data);
        pthread_mutex_unlock(&async->mutex);
        for (unsigned int i = 0, size = rows.size(); i < size; i++) {
            uv_unref(Loop());
            async->callback(async->parent, rows[i]);
        }
    }

    static void close(uv_handle_t* handle) {
        assert(handle != NULL);
        assert(handle->data != NULL);
        Async* async = static_cast<Async*>(handle->data);
        delete async;
        handle->data = NULL;
    }

    void finish() {
        // Need to call the listener again to ensure all items have been
        // processed. Is this a bug in uv_async? Feels like uv_close
        // should handle that.
        listener(&watcher, 0);
        uv_close((uv_handle_t*)&watcher, close);
    }

    void add(Item* item) {
        // Make sure node runs long enough to deliver the messages.
        uv_ref(Loop());
        pthread_mutex_lock(&mutex);
        data.push_back(item);
        pthread_mutex_unlock(&mutex);
    }

    void send() {
        uv_async_send(&watcher);
    }

    void send(Item* item) {
        add(item);
        send();
    }

    ~Async() {
        pthread_mutex_destroy(&mutex);
    }
};

#endif
