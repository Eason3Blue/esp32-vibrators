# ESP32-ManVibrators

#### 介绍
[点击跳转到本家](https://gitee.com/people_on_the_horizon/esp32-vibrators)

使用Arduino编写的ESP32的使用pwm控制电机转动的程序。
可以实现0-255手动调节以及自动随机调节(幅度和时间的最大值和最小值可以手动在线设置)。
提示:模式和参数会自动存储在NVS中并自动在下一次通电时应用。
要求使用BLE进行远程通信。

#### 命令介绍
### 	在手动模式下
		直接输入任意一个0-255的数，就可以实现控制。
### 在自动模式下
##		默认参数模式
			直接输入"random"(不包含引号)即可，默认参数如下：
				AMax = 220
				AMin = 160
				timeMax = 10
				timeMin = 3
##		手动输入参数
			命令构成为：
				random <AMax> <AMin> <timeMax> <timeMin>
			A的取值范围为： 1-255
			time的取值范围为： 1-60
