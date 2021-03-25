## MSP430 GPIO 原理以及接口技术应用

### 实验目的

- 了解I/O口的原理
- 学习C语言中结构体和循环等语法的使用
- 了解部分引脚的功能

### 实验内容

#### 硬件

- MSP430F6638实验主板右下角有5个按键，和5个LED指示灯。

- 按键编号为S1-S5，其与6638芯片的P4.0-P4.4相连。

<img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324102325365.png" alt="image-20210324102325365" style="zoom:50%;" />

- LED编号为LED1-LED5，与6638的P4.5-P.7，P5.7，P8.0相连。

<img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324102007607.png" alt="image-20210324102007607" style="zoom:50%;" />

- 上述涉及的6638的端口有3个，分别是P4,P5,P8，每一个端口有8个引脚，如P4.0-P4.7。

- 软件编程时面向的是端口，而非其中的引脚。

#### 代码思路

- 利用结构体将需要使用的I/O口涉及的寄存器封装起来，便于调用
- 初始化时钟、各个I/O口的输入输出方向
- 初始化LCD
- 设置两个flag变量，名为syncflag，pressState，分别控制LED的亮灭互斥和按键控制常亮。
- 进入循环体
  - 利用P4端口的高低电平监测是否有按键按下，有则更新pressState
  - 初始化所有LED为灭状态
  - 根据两个flag进行分支选择
  - 如果没有按键按下
    - 进入LED互斥亮灭
    - 即根据syncflag的真假，翻转LED1和LED2或者LED3和LED4
    - 然后利用LCDSEG_SetDigit函数在LCD上显示3，2，1倒计时，时间间隔用__delay_cycles
  - 如果有按键按下
    - 进入LED常亮
    - 直接将LED1-LED4引脚输出为高电平，点亮即可
  - 分支执行结束，翻转syncflag

### 实验结果（所用图片是同组另一位同学的，效果略有差异）

- 闪烁状态

<img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324212411881.png" alt="image-20210324212411881" style="zoom:50%;" />

<img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324212620898.png" alt="image-20210324212620898" style="zoom:50%;" />

- 常亮状态（由按键切换）

<img src="C:\Users\19956875375\AppData\Roaming\Typora\typora-user-images\image-20210324212446361.png" alt="image-20210324212446361" style="zoom:50%;" />

- 实验结果很理想，实现了预期效果。有几点思考

  - 按键消抖问题，本实验用的是软件消抖，所以需要一定的delay时间

  - 按键响应问题。因为倒计时需要用delay函数耗时，所以对按键来说响应不及时，这个问题在第一次实验中也遇到了，解决策略是套双层循环，小循环每次只delay几十ms，大循环循环几百次，间接达到目的，可以有效解决按键响应不及时问题。

    附件演示效果中即使用了该策略，但我本人的代码没有使用。

### 源代码

- 见附件

### 演示视频

- 见附件

