# random

基于 Qt 6、C++17、Qt Widgets 的 Windows 名单抽取工具。

## 功能
- 文件：加载名单、保存名单、另存名单
- 多名单标签页，支持滚动、关闭和 + 快捷加载
- 左侧人员列表、右侧人员详细信息
- 根据当前名单字段动态生成筛选条件
- 抽取 / 停止随机抽取动画
- 亮暗主题切换
- 自动加载上一次名单
- CSV / TXT 导入与保存
- CMake / Qt Creator 工程

## 名单格式
第一行为字段名，第一列必须为“姓名”。

示例：

    姓名,学号,班级,性别
    张三,20260001,计算机1班,男
    李四,20260002,计算机1班,女
    王五,20260003,计算机2班,男

当前版本使用简单逗号分隔解析；字段内容本身包含逗号时暂不支持 RFC 4180 引号规则。

## 构建

需要 Qt 6 和 CMake：

    cmake -S . -B build
    cmake --build build --config Release

也可以在 Qt Creator 中直接打开 CMakeLists.txt。

## License

仓库中的 LICENSE 为 GNU GPL v2。
