Checkpoint 3 Writeup
====================

My name: 翟竞波

My SUNet ID: 522024330117

This checkpoint took me about 3 hours to do. I did not attend the lab session.

#2 和 #3 对应讲义的第2、3章；对于 ping 数据的收集和分析在 #4

#### 2. Implementation Challenges:

在我的实现中，TCP 连接始终无法终止。从框架代码中发现，如果使用 `_tcp.close()` 则会立即关闭连接，这显然是不合适的。正确的操作应该是使用 `_tcp.wait_until_closed()` 在等待另一端也传输完毕后再释放。

```C++
void get_URL( const string& host, const string& path )
{
  CS144TCPSocket _tcp;
  _tcp.connect( Address( host, "http" ) );
  string _input = "GET " + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n";
  _tcp.write( _input );
  while ( !_tcp.eof() ) {
    string buffer;
    _tcp.read( buffer );
    cout << buffer;
  }
  _tcp.wait_until_closed();
}
```

#### 3. Remaining Bugs:

我已通过所有测试点。

#### 4. Ping report:

在课程群群友的推荐下，我选用 [www.usa.gov](www.usa.gov) 作为目标，收集到的数据位于 [traceroute.log](check4/traceroute.log) 和 [data.txt](check4/data.txt).

我到该域名的网络链路如下：
```text
traceroute to 41.186.255.86 (41.186.255.86), 255 hops max, 60 byte packets
 1  _gateway.wu.lxd (10.9.77.1)  0.104 ms  0.017 ms  0.014 ms
 2  _gateway (114.212.80.1)  3.168 ms  3.378 ms  3.480 ms
 3  172.28.255.13 (172.28.255.13)  0.706 ms  0.917 ms  1.134 ms
 4  * * *
 5  211.65.206.9 (211.65.206.9)  2.038 ms 219.219.156.241 (219.219.156.241)  1.862 ms 211.65.206.9 (211.65.206.9)  1.955 ms
 6  * * *
 7  * * *
 8  219.219.156.46 (219.219.156.46)  2.319 ms 219.219.156.42 (219.219.156.42)  3.000 ms 219.219.156.46 (219.219.156.46)  2.749 ms
 9  101.4.116.98 (101.4.116.98)  2.267 ms  2.249 ms  2.380 ms
10  101.4.118.106 (101.4.118.106)  2.241 ms  10.125 ms  9.949 ms
11  101.4.117.25 (101.4.117.25)  5.623 ms  5.787 ms  6.103 ms
12  100.64.61.1 (100.64.61.1)  5.327 ms  5.182 ms  4.642 ms
13  101.4.112.61 (101.4.112.61)  11.855 ms  11.718 ms  11.731 ms
14  101.4.118.213 (101.4.118.213)  11.594 ms  11.462 ms  11.328 ms
15  101.4.112.65 (101.4.112.65)  28.760 ms  28.628 ms  28.938 ms
16  * * *
17  101.4.114.194 (101.4.114.194)  29.486 ms  30.168 ms  29.803 ms
18  101.4.117.102 (101.4.117.102)  29.737 ms 101.4.115.182 (101.4.115.182)  33.042 ms 101.4.114.170 (101.4.114.170)  29.639 ms
19  101.4.117.214 (101.4.117.214)  175.704 ms  175.642 ms  175.609 ms
20  ix-xe-9-1-5-0.tcore1.lvw-losangeles.as6453.net (66.110.59.181)  197.082 ms  197.047 ms  197.046 ms
21  * * *
22  * * *
23  * * *
24  * if-bundle-20-2.qcore1.sqn-sanjose.as6453.net (66.198.101.133)  288.814 ms *
25  * * *
26  * * *
27  * if-bundle-10-2.qcore2.ldn-london.as6453.net (64.86.143.83)  290.937 ms *
28  * * *
29  195.219.213.131 (195.219.213.131)  283.969 ms  282.045 ms  282.430 ms
30  195.219.17.175 (195.219.17.175)  284.040 ms  283.759 ms  284.310 ms
31  * * *
32  41.181.190.197 (41.181.190.197)  294.729 ms  286.868 ms  294.705 ms
33  41.181.244.234 (41.181.244.234)  293.018 ms  300.808 ms  297.377 ms
34  41.181.247.41 (41.181.247.41)  457.454 ms  451.189 ms  457.111 ms
35  41.186.255.195 (41.186.255.195)  442.388 ms  437.515 ms  443.147 ms
36  41.186.255.86 (41.186.255.86)  443.663 ms  435.185 ms  438.940 ms

```

我编写了一个 [python](check4/analyse.py) 脚本用于分析和计算数据，并生成图像。

1. 总体送达率：0.9998。收到了 37653 个回复，丢失了 7 个回复（缺少的icmp_seq序列号: 8974, 8975, 8976, 8977, 11983, 11984, 11985）
2. 最长连续成功：25675，区间为 (11986, 37660)
3. 最长连续成功：4，区间为 (8974, 8977)
4. - 给定N请求成功，N+1也成功的条件概率: 0.9999；
   - 给定N请求失败，N+1成功的条件概率: 0.2857；
   - 显然，丢失与丢失之间是强相关（连续的区间）
5. 最小 RTT: 212.0ms
6. 最大 RTT: 249.0ms
7. 如图所示
![](check4/time.png)
8. 频率直方图近似正态分布。
![](check4/histogram.png)
![](check4/cdf.png)
9. 数据过于接近，并且分布较为密集。但可以看出，主要集中分布在 `y=x` 的直线附近，说明 RTT 和时间（主要是上一个周期的网络状态）强相关
![](check4/correlation.png)
10. 我和 `www.usa.gov` 的连接稳定，但会偶发地出现 1s 左右的波动。当前网络质量和前一小段时间的网络质量有很强的联系：如果某一时刻发生丢包，接下来的一段时间内可能会持续丢包。这符合我的预期。


- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
