# pipe

```cpp
template <size_t SIZE_ = 4096>
class pipe
{
public:
    pipe();

    pipe(const pipe &) = delete;
    pipe(const pipe &&) = delete;
    pipe &operator=(const pipe &) = delete;

    void send(const void *data, size_t size);

    bool try_send(const void *data, size_t size)

    void recv(void *data, size_t size);

    bool try_recv(void *data, size_t size);

    void async_send(const void *data, size_t size, boost::asio::yield_context yield);

    void async_recv(void *data, size_t size, boost::asio::yield_context yield);

    size_t size() const;
}
```