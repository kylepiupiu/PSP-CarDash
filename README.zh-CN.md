# PSP CarDash

[English](README.md) | [简体中文](README.zh-CN.md) | [日本語](README.ja.md)

<p align="center"><img src="assets/icon.jpg" width="180" alt="PSP CarDash 图标"></p>

把 Sony PSP 改造成一个实时车载仪表显示终端，并通过 ESP32 作为遥测数据桥。

> **当前稳定基线：v0.3 ESP32 Link**
>
> 参考车型：**Mercedes-Benz W210 E430 / M113 V8 4.3L**
>
> 真实车辆 OBD/K-Line 接入尚未启用。当前稳定版使用 ESP32 模拟遥测数据，通过 Wi-Fi → UDP → PSP 实时传输。

## 项目简介

老 PSP 仍然拥有不错的屏幕、实体按键和基本网络能力。PSP CarDash 的目标，是让它重新成为一个专用的车辆遥测显示终端。PSP 端保持轻量，ESP32-S3 负责连接现代传感器、车辆接口与 PSP。

~~~text
车辆 / OBD（未来）
        │
     ESP32-S3
        │
   2.4 GHz Wi-Fi
        │
     UDP 4210
        │
       PSP
~~~

## 当前稳定功能

v0.3 已在真实 PSP 硬件上完成测试，支持：

- Drive / Engine / Trip / Diagnostics 四个页面
- ESP32-S3 SoftAP 热点连接
- 10 Hz UDP 实时遥测
- ESP32 不可用时自动保留本地 Demo 模式
- Demo / ESP32 数据源运行时切换
- PSP Wi-Fi Profile 选择
- ESP32 链路中断时安全回退
- W210 E430 / M113 参考车辆配置

## 硬件需求

### 必需硬件

- 支持 Homebrew 的 Sony PSP
- ESP32-S3 开发板
- ESP32 烧录用 USB 数据线

### 当前参考硬件

- PSP-3000 系列
- ESP32-S3 DevKitC-1
- 为兼容 PSP 老 Wi-Fi 协议而使用的 2.4 GHz 开放热点

PSP 的 Wi-Fi 协议较老。为提高兼容性，参考 ESP32 固件默认建立一个**开放式 2.4 GHz 热点**：

~~~text
SSID: PSP-CAR-DASH
AP IP: 192.168.4.1
UDP: 4210
~~~

## 仓库结构

~~~text
PSP-CarDash/
├── psp/            PSP Homebrew 源码与 Makefile
├── esp32/          ESP32-S3 遥测桥
├── tools/          电脑端 UDP 模拟器
├── docs/           协议、硬件说明与路线图
├── assets/         项目图像资源
└── experimental/   尚未进入稳定版的实验功能
~~~

## PSP 按键

| 按键 | 功能 |
|---|---|
| 左 / 右 | 切换仪表页面 |
| START | 开关本地 Demo 动画 |
| SELECT | 冻结 / 恢复数据 |
| L / R 肩键 | 未连接时选择 PSP Wi-Fi Profile |
| △ | 连接 / 断开 ESP32 Link |
| ○ | 退出程序 |
| HOME | PSP 系统退出 |

## 快速开始

### 1. 编译 PSP 程序

安装 [PSPDEV](https://pspdev.github.io/)，并确保 `psp-gcc` 可用。

~~~bash
export PSPDEV=/opt/pspdev
export PATH="$PSPDEV/bin:$PATH"

cd psp
make clean
make
~~~

将 `psp/EBOOT.PBP` 复制到：

~~~text
/PSP/GAME/PSPCARDASH/EBOOT.PBP
~~~

### 2. 烧录 ESP32

使用 Arduino IDE 或 Arduino CLI 打开并烧录：

~~~text
esp32/PSP_CAR_DASH_ESP32_LINK.ino
~~~

ESP32 启动后建立：

~~~text
SSID: PSP-CAR-DASH
Security: OPEN
UDP: 4210
Rate: 10 Hz
~~~

### 3. 在 PSP 上建立 Wi-Fi Profile

进入 PSP XMB：

~~~text
Settings
 -> Network Settings
 -> Infrastructure Mode
 -> New Connection
 -> Scan
 -> PSP-CAR-DASH
 -> Security: None
 -> Save
~~~

启动 PSP CarDash，用 L/R 肩键选择对应的 Wi-Fi Profile，然后按 **△** 连接。

连接正常时，Diagnostics 页面应看到：

~~~text
AP STATE   4 / 4
PSP IP     192.168.4.x
UDP PORT   4210
PACKETS    持续增加
~~~

## 遥测协议

v0.3 使用简单的 ASCII 文本包：

~~~text
PCD1,seq,speed,rpm,coolant,throttle,voltage10,intake,load,fuel10,oiltemp,oilpress10
~~~

示例：

~~~text
PCD1,145,86,3250,92,27,142,31,38,68,96,38
~~~

详细字段定义见 [docs/PROTOCOL.md](docs/PROTOCOL.md)。

## 电脑端测试

没有 ESP32 时，也可以用电脑模拟 UDP 数据源：

~~~bash
python3 tools/pcd_sender.py
~~~

## 项目状态

### 稳定版 — v0.3 ESP32 Link

- PSP 程序运行：已测试
- 页面切换：已测试
- ESP32 SoftAP：已测试
- UDP 实时遥测：已测试
- Demo 回退：已测试

### 实验版 — 高保真 Render UI

正在开发平滑、非像素风的 480×272 高保真车载仪表界面。在完成同等真机验证前，不会替代稳定版源码。

详见 [experimental/render-ui/README.md](experimental/render-ui/README.md)。

## 路线图

- [x] PSP 稳定运行骨架
- [x] ESP32-S3 Wi-Fi 数据桥
- [x] UDP 遥测协议
- [x] 本地 Demo / 回退模式
- [ ] 高保真渲染仪表 UI
- [ ] 确认 W210 诊断接口和物理层
- [ ] W210 标准 OBD-II / K-Line 遥测
- [ ] 真实 RPM / 车速 / 水温 / 油门 / 电压
- [ ] DTC 故障码读取
- [ ] 行程记录
- [ ] 更多车型配置

## 车辆接入安全

当前稳定代码**不会发送任何车辆控制指令**。

未来车辆接入首先坚持只读遥测。在连接实验硬件到车辆诊断接口或通信总线前，必须确认针脚、协议、电平以及必要的保护/隔离设计。

## 参与贡献

欢迎提交 Issue、真实硬件测试结果、车型协议适配和 UI 改进。

详见 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 开源许可

代码使用 MIT License，详见 [LICENSE](LICENSE)。

## 商标声明

本项目是独立、非官方的开源兴趣项目。Sony、PlayStation Portable / PSP、Mercedes-Benz、W210 及相关名称和标识归各自权利人所有。本项目与相关厂商不存在隶属、赞助或官方认可关系。
