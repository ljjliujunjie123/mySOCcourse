## MSP430 段式液晶实验

### 实验目的

- 了解MSP430F6638的LCD驱动模块
- 了解液晶显示的原理

### 实验内容

#### 硬件

- 段式液晶的驱动信号由两个部分组成

  - 第一部分是公共端偏压信号（COM），主板上的段式液晶模块是1/3偏压（bias）的，也就是加在液晶模块每一位上的电压分为VCC—2/3VCC—1/3VCC—GND四个等级，这个偏压信号可以通过软件设置让MSP430f6638内部的LCD驱动模块自动生成，其引脚COM0—COM3就是这些电压的输出引脚，直接与段式液晶屏的COM0—COM3相连即可。

  - 第二部分信号是驱动信号（S0-S11），与液晶屏上的S0-S11连接

    <img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324105839955.png" alt="image-20210324105839955" style="zoom:50%;" />

- 液晶显示的模式分为，2MUX,3MUX,4MUX，6638实验主板使用的是4MUX。

  <img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324110052107.png" alt="image-20210324110052107" style="zoom:45%;" />

#### 代码思路（库文件代码为主）

- 配置好系统时钟，即选为系统时钟配置合适的晶振
- 配置好与段式液晶相连的IO口，即确定相应IO口的工作模式
- 操作IO口对段式液晶进行初始化
- 编写两个工具函数，分别是showSomething，clearLCD
  - 前者调用LCDSEG_SetDigit函数显示需要展示的内容
  - 后者调用LCDSEG_SetDigit函数清楚LCD的内容
- 以数组的形式定义好需要显示的内容信息
- 进入循环体
  - 调用showSomething显示第一段信息
  - 利用__delay_cycles函数延时一段时间
  - 调用clearLCD清屏
  - 调用showSomething显示第二段信息
  - 利用__delay_cycles函数延时一段时间
  - 调用clearLCD清屏

### 实验结果（演示内容为同组另一位同学拍摄，与本人代码略微不同)

<img src="C:\Users\199568~1\AppData\Local\Temp\WeChat Files\aa5efbe023cbd0963d36b072707690e.jpg" alt="aa5efbe023cbd0963d36b072707690e" style="zoom:50%;" />

<img src="C:\Users\199568~1\AppData\Local\Temp\WeChat Files\bfe4e28d98dd36bb535dabbf54e76d5.jpg" alt="bfe4e28d98dd36bb535dabbf54e76d5" style="zoom:50%;" />

<img src="C:\Users\199568~1\AppData\Local\Temp\WeChat Files\fac9ace350e8d42c6eee81be3a2713b.jpg" alt="fac9ace350e8d42c6eee81be3a2713b" style="zoom:50%;" />

- 实验效果很理想，实验了预期目标
- 有几点思考：
  - 在显示相似内容时，能否预先定义一些基本数组，用以存放相似部分的16进制数，作为输入参数以4MUX方式点亮对应的管脚。
  - 显示时为了让肉眼能看到，需要delay一段时间，如果自己构造这个delay函数，能不能在里面加一些其他功能，比如响应按键的按下，然后调起其他模块的功能。
  - LCD点亮前需要初始化LCD以及设置时钟信息等等，这些操作与其他模块该如何兼容，特别是设置时钟频率这一点，会不会影响其他模块的设计。

### 源代码

- 见附件

### 演示视频

- 见附件