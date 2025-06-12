# IMS

使用的库
- Boost 1.88
- fmt

## Log System
异步日志系统



实现将日志记录的实现和配置分离。仿照log4j设计，

### 辅助函数

### LogQueue

日志输出的队列



### Logger

每一个logger都应该有一个Level

假设Logger M具有q级的level，发出一个level==p的日志记录请求，只有满足p>=q时请求才会被处理。

请求方法

```cpp
void unknown(message);
void debug(message);
void info(message);
void warn(message);
void error(message);
void fatal(message);

void log(level, message);
```

### LogEvent
应该存在获取当前时间点的方法

`std::chrono::system_clock::time_point` 创建时默认值为 1970-01-01 08:00:00
初始化为：`std::chrono::system_clock::now()`
### LogAppender

日志记录输出的目的地，应该有file, console, email

每个Logger可以有多个Appender，但是相同的Appender只会被添加一次



### LogLayout

指定输出格式，将Layout与Appender关联到一起实现。



### Logger Mgr

管理一个系统的logger，作为一个全局的logger？


## 锁的使用
轻量级的锁不需要使用unique_ptr进行资源管理



