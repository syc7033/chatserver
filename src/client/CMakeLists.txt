# 定义一个SRC_LIST变量 包含了该目录下的所有源文件
aux_source_directory(. SRC_LIST)

# 指定生成的可执行文件
add_executable(ChatClient ${SRC_LIST})

# 指定可执行文件需要链接的库文件
target_link_libraries(ChatClient pthread)
