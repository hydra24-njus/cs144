Checkpoint 0 Writeup
====================

My name: Zhai Jingbo

My SUNet ID: 522024330117

I collaborated with: nothing

I would like to credit/thank these classmates for their help: nobody

This lab took me about 1 hours to do. I did not attend the lab session.

My secret code from section 2.1 was: doesn't exist

I was surprised by or edified to learn that: It's easy to complete.

### 环境配置

我使用 wsl2 Ubuntu 24.04 LTS 作为实验环境，物理机配置为
- Intel core i5 13500
- UHD 770
- 2*16G 4400MT/S DDR5 Memory


### telnet

如图所示

![telnet](/writeups/graphs/telnet.png)
![smtp](/writeups/graphs/smtp.png)
![smtp_result](/writeups/graphs/smtp2.png)
![netcat](/writeups/graphs/netcat.png)

### webget

我使用 `socket.hh` 中的 class `TCPSocket` 和 `adrress.hh` 中的class `Address` 实现 `webget` 功能。在建立 TCP 连接后，向其写入请求命令，然后逐行读取接收到的信息并输出。
很简单的一项，只需要完形填空即可。

![](/writeups/graphs/webget.png)
![](/writeups/graphs/webget-test.png)

### Byte_Stream

同样是简单的实现。

我使用 `string` 数据结构作为 `buffer`，并在 `Byte_Stream` 类中补充了部分变量记录相关信息。

在 `push` 和 `pop` 的实现中，我做了精确的边界条件检查，以尽可能处理所有可能的执行路径。

相比于 sponge 版本的 cs144，本实验大幅重构了代码框架，但对于实验目标（如本文需要实现的几个方法），手册和头文件的注释中都缺少明确、精准的行为描述，为通过所有测试造成了一定困难。

![](/writeups/graphs/check0.png)