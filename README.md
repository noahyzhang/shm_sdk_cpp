
# 共享内存和信号量的封装

### 一、共享内存的封装

定义了一个可扩展的格式，即共享内存的格式为：头部+数组
其中头部和数组中节点都是模版，使用者可以自定义其中内容。比如如下定义

```c++
struct ARRAY_SHM_HEADER {
    uint32_t version;
    uint32_t cur_node_count;
    uint32_t max_node_count;
    uint32_t header_crc_val;
    uint64_t time_ns;
};

struct DataNode {
    uint32_t tid;
    uint32_t arena_id;
    uint32_t allocated_kb;
    uint32_t deallocated_kb;
};
```

则共享内存中数据的格式即为：| ARRAY_SHM_HEADER | DataNode | DataNode | ... | DataNode |

### 二、信号量的封装

将复杂的信号量操作简单化，进程之间只需要通过 lock、unlock 接口
即可实现一元的进程锁，用于多进程之间的同步

### 三、简单使用

见 examples 目录中的 sample 目录中的例子
