## 项目简介
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;lua是一门精妙简洁，而功能强大的语言，学习和掌握它的核心机制有着重要的意义。这是一个仿制lua解释器的项目(参照的版本是Lua5.3)，我希望通过逐步实现lua解释器的各个部分，更加深刻地掌握lua的基本结构和运作原理。本项目将分为多个部分完成，并为每一个部分附上一篇博文为该部分的设计和实现进行解析。开发这个项目的目的，并不是做一个能用于生产环境的lua解释器，而是尝试追寻前辈的步伐，尽最大可能理解其设计lua语言的思路，理解其中的关键细节。这是一个探索原理的旅程，因此效率并不是本项目要考虑的关键因素。这里我遵循的是"FIRST  make  it  run, THEN make it run fast"的原则，先让它跑起来。  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;本项目采取和官方版本的近似的命名规范，并尽最大努力遵循lua官方的设计主线。由于开发过程是我自己重新手写的，因此不会在所有细节和官方版本保持一致。同时这是一份参考资料，主旨是提供一条主线，不建议只跟随本旅程来研究lua。最终希望回到研究lua官方源码的道路上。  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;本系列将会为一些观点，理论附注上引证来源，并在Reference上展示，最后本人水平有限，如果有错误的地方，希望大家联系我加以指正。同时欢迎大家加入我创建的qq群185017593一起讨论技术。  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;系列内容，将在我的博客上展示，欢迎大家关注我的[blog](http://manistein.club/)。

## 系列内容([地址](https://manistein.github.io/blog/tags/let-us-build-a-lua-interpreter/))
* Part1：[虚拟机的基础--Lua基本数据结构、栈和基于栈的C函数调用的设计与实现](https://manistein.github.io/blog/post/program/build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part1/)
* Part2：[Garbage Collection基础架构](https://manistein.github.io/blog/post/program/build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part2/)
* Part3：[String设计与实现](https://manistein.github.io/blog/post/program/build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part3/)
* Part4：[Table设计与实现](https://manistein.github.io/blog/post/program/build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part4/)
* Part5：[脚本运行基础架构的设计与实现](https://manistein.github.io/blog/post/program/let-us-build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part5/)
* Part6：[词法分析器设计与实现](https://manistein.github.io/blog/post/program/let-us-build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part6/)
* Part7：[构建完整的语法分析器（上）](https://manistein.github.io/blog/post/program/let-us-build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part7/)
* Part8：[构建完整的语法分析器（下）](https://manistein.github.io/blog/post/program/let-us-build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part8/)
* Part9：[metatable](https://manistein.github.io/blog/post/program/let-us-build-a-lua-interpreter/%E6%9E%84%E5%BB%BAlua%E8%A7%A3%E9%87%8A%E5%99%A8part9/)
* Part10：userdata
* Part11：upvalue
* Part12：weektable
* Part13：require机制
* Part14：热更新原理
* Part15：再谈GC
* Part16：尾声

## 编译与运行

#### 获取仓库
```
git clone git@github.com:Manistein/dummylua-tutorial.git
```

#### 在Linux和Mac上编译与运行
```
cd dummylua-tutorial/
cd linux/
cd part01/
make

cd bin/
./dummylua
```

#### 在Windows上编译与运行
* 进入dummylua-tutorial/windows/project/
* 使用VS2013或以上版本打开工程
* 自行编译和运行

## 测试平台
* Linux（Ubuntu）
* Windows
* Mac
