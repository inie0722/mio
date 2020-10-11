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

## 性能
这里采用Linux pipe 进行性能对比, 尝试发生不同大小的数据各1万次，计算出平均每次发送的耗时，单位 纳秒 ns<br>

__write__<br>
| 数据大小/byte| 64 | 128 | 256 | 512 | 1024 |
|---------|----|------|----|------|------|
| mio pipe |33 | 97 | 114 | 150 | 207 |
| linux pipe | 2274 | 2077 | 2021 | 1905 | 1679 |

__read__<br>
| 数据大小/byte| 64 | 128 | 256 | 512 | 1024 |
|---------|----|------|----|------|------|
| mio pipe | 30 | 93 | 111 | 148 | 204 |
| linux pipe | 2233 | 2042 | 1988 | 1913 | 1861 |