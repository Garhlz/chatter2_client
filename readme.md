- Windows 11
- Qt 6.5.3 (MingW 11.2.0 64-bit)
- CMake ≥3.16
- MinGW 11.2.0
这是我的cppqt构建的聊天应用的客户端. 目前我实现了聊天大厅功能, 还没有实现私聊,群聊,文件上传等.
我遇到了一个问题, 就是qt无法成功渲染我的qss文件:
Loaded ":/styles/styles.qss" successfully

Loaded ":/styles/messagebubble.qss" successfully

Loaded ":/styles/chatwindow.qss" successfully

Could not parse application stylesheet

Applied QSS styles successfully

Main: Connecting to server at "localhost" : 9999
我查阅了qt6的文档,发现qt6似乎不支持嵌套的样式表选择器语法,能否帮我重构qss文件以及涉及的cpp文件,使之符合这种简单的语法:QPushButton#okButton
顺便,帮我美化一下样式和布局设计好吗