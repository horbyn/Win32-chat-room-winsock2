## This is a P2P SERVER chatting room

English below | [中文版](https://horbyn.github.io/2022/03/14/winsock/)

<br>

## INTRODUCTION

This is a very simple LAN-P2P demo, learning in mind. The effects I used are primer, so it may be not suit those who attempts to comprehend in deep on IO model or other corresponding aspects. The whole program, based on TCP protocol to communicate, uses the simple enough IO model--select model--to complete asynchrony, without multithreading at all. The preparations are `Win32 Framework`, `Winsock Framework` and `select model` in my opinions if you want to complete a similar effects. The main oppose of the post here is to exposit the designing.

<br>

## LICENSE

The source codes the post involved use [MIT](./LICENSE) license.

<br>

## DEMONSTRATION

### LOCALHOST

The localhost bounds to "127.0.0.1:8888" by default

![localhost.png](https://s2.loli.net/2022/03/13/PdIkNa7f4VCjMqs.png)

<br>

### LAN

Make sure two computers share the same wifi. And you have to modify the address to be bounded from "127.0.0.1" to the ip address using `ipconfig` commend to show.

![lan1.4.png](https://s2.loli.net/2022/03/13/1TEodKAaue4UnS6.png)

![lan1.7.png](https://s2.loli.net/2022/03/13/HzlqcPh8tgGkITm.png)

<br>

## DEV ENVIRONMENT

- Visual Studio Community 2019 v16.4.1
- Microsoft SDKs v10.0.18362
- Microsoft (R) C/C++ Optimizing Compiler Version 19.24.28314 for x86

<br>

## NOTE

About **ERR::xxxxx** codes returned by Server or Client, details on [Win32 Winsock error code](https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2) that could search by "Ctrl + F". like:

![how-to-use.png](https://s2.loli.net/2022/03/14/eRUVudqr4MmOsGl.png)

<br>

## TODO

The program is not perfect firstly, which is also my direction I optimize in the future:

- DO NOT SUPPORT CHINESE. In fact, I am carefully handle that the translations between `char` and `wchar_t`, but gibberish still happened. While the text in "Edit Control" catched on the background are also gibberish when I debug. I am not sure if there is something I overlooked.
- PANEL NOT FRIENDLY. The panel based on "Static Control" and my encapsulating functions are working together to display messages. It looks comfortable if I use directly `TextOut()` and scroll. But to do so involves another aspect that I need more time to deal with.
- SOME BUG. The phenomenon that messages cannot receiving and sending normally when I boot multiple clients with Chinese inputing and user switching to send/recv messages happened. This problem, however, is too hard to re-appear and I could not repair now.
- C STYLE. Win32 is originally a set of C APIs, so it is understandable to write in C. After all, the readability of C code is poor, which is filled with global variables and state flags. Considering maintain and expansibility, I hope I could make reconstruction in cpp.

<br>
