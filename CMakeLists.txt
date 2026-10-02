cmake_minimum_required(VERSION 3.21)
project(random VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_add_executable(random
    main.cpp
    MainWindow.h
    MainWindow.cpp        # 生命周期与信号装配
    MainWindowUi.cpp      # 界面与主题
    MainWindowList.cpp    # 名单与人员
    MainWindowFilter.cpp  # 筛选
    MainWindowDraw.cpp    # 抽取与历史
    MainWindowIo.cpp      # 序列化与文件
    MarkdownTable.h       # Markdown 表格工具
    MarkdownTable.cpp
    random.cpp            # 随机数获取
)
target_link_libraries(random PRIVATE Qt6::Widgets bcrypt)
set_target_properties(random PROPERTIES WIN32_EXECUTABLE TRUE)
