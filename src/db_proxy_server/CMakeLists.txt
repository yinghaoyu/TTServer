cmake_minimum_required(VERSION 2.6)
PROJECT(db_proxy_server)

# 收集指定目录中所有源文件
AUX_SOURCE_DIRECTORY(./ SRC_LIST)
AUX_SOURCE_DIRECTORY(./business SRC_LIST)

# 设置bin文件路径
SET(EXECUTABLE_OUTPUT_PATH  ../bin)

# 设置库路径
if(APPLE)
    SET(PB_LIB_DIR ../base/pb/lib/mac)
elseif(UNIX)
    SET(PB_LIB_DIR ../base/pb/lib/linux)
endif()

# 设置mysql包含目录及库路径
SET(MYSQL_INCLUDE_DIR /usr/include/mysql)
SET(MYSQL_LIB /usr/lib/mysql)

# 添加自定义宏
ADD_DEFINITIONS( -g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
-DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
-DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++11 -DENCRYPT)

# 指定头文件目录
INCLUDE_DIRECTORIES(./ ./businiess ../base ../base/jsoncpp ../base/pb/protocol
../base/pb/ ${MYSQL_INCLUDE_DIR})

#指定链接文件目录
LINK_DIRECTORIES(./ ../bin ${PB_LIB_DIR} ${MYSQL_LIB})

#ADD_XXXX必须在TARGET_LINK_LIBRARIES前面，否则会报错
#ADD_LIBRARY(${PROJECTNAME} SHARED/STATIC ${SRC_LIST})
ADD_EXECUTABLE(db_proxy_server ${SRC_LIST})

#TARGET_LINK_LIBRARIES(db_proxy_server pthread base protobuf-lite dl mysqlclient_r hiredis curl slog crypto)
TARGET_LINK_LIBRARIES(db_proxy_server pthread base protobuf-lite dl mysqlclient hiredis curl crypto)
