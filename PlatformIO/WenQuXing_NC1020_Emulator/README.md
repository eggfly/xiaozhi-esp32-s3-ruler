# Test

It's a PlatformIO project.

* https://github.com/quadpixels/banxian_cc800_java
* https://github.com/fwindpeak/wqxsim-nc1020
* https://github.com/hackwaly/NC1020
* https://github.com/fancyblock/GVBASIC
* https://github.com/Wang-Yue/NC1020
* https://github.com/wangyu-/NC2000
* 牛: http://yxts8888.ysepan.com/
* https://www.emsky.net/bbs/forum.php?mod=viewthread&tid=33474
* https://computer.retromuseum.org:86/player.html?machine=ggvnc1020
* https://www.bilibili.com/video/BV1kt421w7Pb/
* https://github.com/wangyu-/NC1020android -> This branch is 2 commits ahead of hackwaly/NC1020:master

## performance

```
slice=20,cost=50ms
nc1020_loop,slice=20,cost=100ms

slice=20,cost=49ms
nc1020_loop,slice=20,cost=100ms

slice=20,cost=49ms
nc1020_loop,slice=20,cost=99ms

slice=20,cost=49ms
nc1020_loop,slice=20,cost=100ms

slice=20,cost=49ms
nc1020_loop,slice=20,cost=100ms

slice=20,cost=49ms
nc1020_loop,slice=20,cost=99ms

slice=20,cost=49ms
```

## Memory mapping

根据nc1020.cpp中的代码,主要的内存访问逻辑在Peek和PeekW函数中。我来整理一个详细的内存映射表格:


| 地址范围(十六进制) | 大小 | 类型 | 描述 | 访问方式 |
|------------------|------|------|------|---------|
| 0x0000 - 0x003F | 64字节 | IO空间 | I/O寄存器映射 | io_read/io_write数组 |
| 0x0040 - 0x007F | 64字节 | RAM | 特殊RAM区域 | ram_40指针 |
| 0x0080 - 0x00FF | 128字节 | RAM | 栈空间 | stack指针 |
| 0x0100 - 0x1FFF | 7936字节 | RAM | RAM页面0 | ram_page0指针 |
| 0x2000 - 0x3FFF | 8192字节 | RAM | RAM页面1 | ram_page1指针 |
| 0x4000 - 0x5FFF | 8192字节 | RAM | RAM页面2 | ram_page2指针 |
| 0x6000 - 0x7FFF | 8192字节 | RAM | RAM页面3 | ram_page3指针 |
| 0x8000 - 0x9FFF | 8192字节 | NOR Flash | NOR Bank 0-31 | nor_banks数组 |
| 0xA000 - 0xBFFF | 8192字节 | NOR Flash | NOR Bank 0-31 | nor_banks数组 |
| 0xC000 - 0xDFFF | 8192字节 | NOR Flash | NOR Bank 0-31 | nor_banks数组 |
| 0xE000 - 0xFFFF | 8192字节 | ROM | 系统ROM | rom_volume0/1/2数组 |

### 内存访问说明:

1. **IO空间 (0x0000-0x003F)**
   - 通过io_read和io_write函数指针数组访问
   - 每个地址都有对应的读写处理函数

2. **RAM空间 (0x0040-0x7FFF)**
   - 分为多个页面,通过memmap数组映射
   - 特殊区域如栈空间有专门指针

3. **NOR Flash (0x8000-0xDFFF)**
   - 通过nor_banks数组访问
   - 每个bank大小为0x8000
   - 最多支持32个bank

4. **ROM空间 (0xE000-0xFFFF)**
   - 通过rom_volume0/1/2数组访问
   - 支持多个ROM卷
   - 通过ram_io[0x0D]选择当前卷

### 内存映射切换:

1. **Bank切换**
   - 通过ram_io[0x00]选择当前bank
   - 影响0x8000-0xDFFF区域

2. **Volume切换**
   - 通过ram_io[0x0D]选择当前ROM卷
   - 影响0xE000-0xFFFF区域

3. **页面映射**
   - 通过memmap数组动态映射内存页面
   - 可以灵活配置RAM和ROM的映射关系


这个内存布局是基于代码中的以下关键部分:

1. Peek函数中的地址判断逻辑
2. memmap数组的使用
3. rom_volume和nor_banks数组的分配
4. IO空间的特殊处理

