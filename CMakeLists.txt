cmake_minimum_required(VERSION 3.16)
project(chatter_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Concurrent)

set(PROJECT_SOURCES
    src/main.cpp
    src/GlobalEventBus.cpp
    src/GlobalEventBus.h
    src/ui/LoginWindow.cpp
    src/ui/LoginWindow.h
    src/ui/RegisterWindow.cpp
    src/ui/RegisterWindow.h
    src/ui/ChatWindow.cpp
    src/ui/ChatWindow.h
    src/ui/MessageBubble.cpp
    src/ui/MessageBubble.h
    src/ui/PublicChatTab.cpp
    src/ui/PublicChatTab.h
    src/ui/PrivateChatTab.cpp
    src/ui/PrivateChatTab.h
    src/ui/PrivateChatSession.cpp
    src/ui/PrivateChatSession.h
    src/ui/GroupChatTab.cpp
    src/ui/GroupChatTab.h
    src/ui/GroupChatSession.cpp
    src/ui/GroupChatSession.h
    src/network/ChatClient.cpp
    src/network/ChatClient.h
    src/network/MessageProcessor.cpp
    src/network/MessageProcessor.h
    src/utils/MessageHandler.cpp
    src/utils/MessageHandler.h
    src/utils/JsonConverter.cpp
    src/utils/JsonConverter.h
    src/utils/UserInfo.cpp
    src/utils/UserInfo.h
    src/utils/ConfigManager.cpp
    src/utils/ConfigManager.h
    src/utils/GroupTask.cpp
    src/utils/GroupTask.h
    src/FileTransferManager.cpp
    src/FileTransferManager.h
    src/dialogs/UserSelectionDialog.cpp
    src/dialogs/UserSelectionDialog.h
    src/utils/User.h
    src/utils/UserManager.cpp
    src/utils/UserManager.h 
    src/WindowManager.cpp
    src/WindowManager.h
    resources/resources.qrc
)

add_executable(chatter_client WIN32 ${PROJECT_SOURCES})

target_link_libraries(chatter_client PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

target_include_directories(chatter_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 确保 GUI 应用程序设置
set_target_properties(chatter_client PROPERTIES
    WIN32_EXECUTABLE false
)
# 关闭, 因为需要debug输出
message(STATUS "Sources: ${PROJECT_SOURCES}")

