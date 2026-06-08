# ZigBee SmartHome System

本项目是一个基于 ZigBee 的智能家居监测与控制系统，主要由 Qt 上位机、CC2530 ZigBee 协调器、CC2530 终端节点和 STM32 控制节点组成。

## 项目组成

- `qt_upper_computer`：Qt 上位机工程，用于数据显示、曲线显示和设备控制
- `cc2530_coordinator`：CC2530 ZigBee 协调器工程，负责组网和数据转发
- `cc2530_terminal`：CC2530 终端节点工程，负责温湿度、烟雾等数据采集
- `stm32_node`：STM32 控制节点工程，负责光照采集和 PWM 灯光控制

## 实现功能

- DHT11 温湿度采集
- MQ-2 烟雾检测
- BH1750 光照采集
- 三路 PWM 灯光控制
- ZigBee 无线通信
- Qt 上位机数据显示与控制
- 自定义串口通信协议

## 项目状态

该项目已完成毕业设计实物演示。