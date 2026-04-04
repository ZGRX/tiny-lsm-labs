项目目录结构

tiny-lsm/
├── doc/           # 项目文档
├── example/       # 示例程序
├── include/       # 公共头文件
├── sdk/           # SDK 接口
├── server/        # 服务器端实现
├── src/           # 核心源代码
├── test/          # 测试代码
├── .clangd        # Clangd 配置文件
├── .gitignore     # Git 忽略文件配置
├── Readme.md      # 项目说明文档
└── xmake.lua      # xmake 构建配置文件

其中src目录下包含以下子目录：

src/
├── block/           # 数据块的编码与解码
├── iterator/        # 统一的迭代器接口
├── lsm/             # LSM 引擎的核心逻辑
├── memtable/        # 内存表（MemTable）管理
├── redis_wrapper/   # Redis 协议兼容层
├── skiplist/        # 跳表实现
├── sst/             # SSTable 的读写与管理
├── utils/           # 工具函数与通用组件
├── wal/             # Write Ahead Log管理
```
