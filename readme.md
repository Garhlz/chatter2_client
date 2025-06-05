# 项目结构
client/项目根目录，包含所有客户端相关文件。 
CMakeLists.txt: CMake 配置文件，定义编译规则，指定 Qt 依赖和源文件。 

resources/资源文件目录，存放界面样式和其他静态资源。 
resources.qrc: Qt 资源文件，注册样式表等资源以便程序访问。 
styles/styles.qss: QSS 样式表，定义界面组件的视觉效果，如颜色、边框、渐变背景等。

src/源代码目录，包含核心功能实现，分为以下子模块：
main.cpp程序入口，初始化 QApplication、ChatClient 以及登录/注册窗口，处理命令行参数并加载 QSS 样式。

network/网络通信模块，负责与服务器的 TCP 连接和消息处理。
ChatClient.cpp/h核心网络客户端类，管理 TCP 连接、心跳机制、消息发送与接收。
MessageProcessor.cpp/h消息处理类，解析服务器返回的 JSON 消息并触发对应信号。

ui/用户界面模块，包含所有窗口和控件逻辑。
LoginWindow.cpp/h登录窗口，处理用户登录请求，支持无边框窗口拖动。
RegisterWindow.cpp/h注册窗口，支持新用户注册，包含输入验证。
ChatWindow.cpp/h主聊天窗口，集成公共聊天、私聊和群聊标签页，显示在线人数和状态。
PublicChatTab.cpp/h公共聊天标签页，支持大厅消息发送与显示。
PrivateChatTab.cpp/h私聊标签页，管理私聊会话，显示在线/离线用户列表。
PrivateChatSession.cpp/h私聊会话组件，处理特定用户的私聊消息和文件传输。
GroupChatTab.cpp/h群聊标签页（功能未完全实现），支持群组选择和消息发送。
MessageBubble.cpp/h消息气泡组件，展示聊天消息，支持左右布局和动态宽度。

utils/工具模块，提供通用功能支持。
JsonConverter.cpp/hJSON 数据转换工具，处理 JSON 对象与字节数组的互转。
MessageHandler.cpp/h消息生成工具，构造不同类型的 JSON 消息（如登录、聊天等）。

# 技术要点介绍
1. Qt 框架与信号-槽机制:
项目基于 Qt 6.5.3，利用其跨平台能力和强大的 GUI 组件。信号-槽机制贯穿整个项目，例如 ChatClient 的 messageReceived 信号触发 ChatWindow 的消息显示逻辑。这种设计解耦了网络层与 UI 层，提高了代码可维护性

2. TCP 网络通信ChatClient:
使用 QTcpSocket 与服务器建立长连接，支持实时消息传输。消息以 JSON 格式编码，MessageProcessor 解析消息类型（如 CHAT、PRIVATE_CHAT、FILE）并分发处理。心跳机制（未完全实现）通过定时发送 HEARTBEAT 消息确保连接活跃

3. QSS 样式美化:
项目通过 styles.qss 定义统一的界面风格，包括渐变背景、圆角按钮、动态悬停效果等。QSS 的 CSS-like 语法便于快速调整视觉效果，增强用户体验

4. 模块化设计:
项目将功能分为网络（network）、界面（ui）和工具（utils）模块，各模块职责明确。例如，MessageHandler 只负责消息构造，JsonConverter 只处理 JSON 转换，降低了模块间的耦合度，便于功能扩展（如添加群聊管理）

5. CMake 构建系统:
使用 CMake 管理项目构建，CMakeLists.txt 配置 Qt 依赖、C++17 标准和自动生成 MOC/UIC 文件。支持 MinGW 编译器，确保 Windows 环境下高效开发。使用脚本进一步简化了构建流程