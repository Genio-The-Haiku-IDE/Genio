# lsp-cpp
An easy [language-server-protocol](https://github.com/microsoft/language-server-protocol) client

* 一个简单的lsp客户端
* c++上目前似乎并没有一个简单好用lsp的客户端
* 所以就想给我的代码编辑框写一个lsp client

只有头文件(header-only) 直接引用就好了

cmake:

`
include_directories(${lsp_include_dirs})
` 

> 很多代码参考clangd 有的直接Ctrl+C了 (●'◡'●)
