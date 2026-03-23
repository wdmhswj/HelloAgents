# HelloAgents

## 现代 C++ 项目布局
```text
project_root/
├── CMakeLists.txt          # 顶层构建脚本
├── include/                # 公共头文件（对外接口）
│   └── project_name/       # 命名空间目录，防止头文件冲突
│       ├── core/
│       │   └── widget.h
│       └── utils.h
├── src/                    # 源文件及私有头文件
│   ├── core/
│   │   ├── widget.cpp
│   │   └── widget_impl.h   # 私有实现头文件
│   ├── utils.cpp
│   └── main.cpp            # 可执行项目的入口
├── tests/                  # 单元测试
│   ├── CMakeLists.txt
│   └── test_widget.cpp
├── examples/               # 示例代码
│   └── demo.cpp
└── libs/                   # 第三方依赖（或使用包管理器）
```