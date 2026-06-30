# 嵌赛项目说明

项目名称：乐鑫·智居 SmartHomeHub

主控：ESP32-S3

代码入口：`src/main.cpp`

构建方式：PlatformIO，配置文件为 `platformio.ini`

## 一、项目定位

这是一个智能家居中控演示系统，围绕环境感知、人在检测、灯光联动、无人安防、声音报警、红外空调模拟控制和屏幕状态展示做完整闭环。

核心演示逻辑：

```text
有人 -> 灯带亮，安防关闭
无人超过 10 秒 -> 自动进入安防
安防状态下检测到大声音 -> 蜂鸣器报警，灯带变红，屏幕显示报警
温度 >= 28C 或按下空调键 -> 发送红外降温演示脉冲
遥控器红外信号进入接收头 -> 屏幕显示 IR: RX
```

## 二、环境配置

推荐使用 VS Code + PlatformIO。

常用命令：

```powershell
pio run
pio run -t upload
pio device monitor -b 115200
```

开发板配置：

| 项目 | 配置 |
|---|---|
| PlatformIO 环境 | `esp32-s3-devkitc-1` |
| 框架 | Arduino |
| 串口监视器 | `115200` |
| 上传速度 | `921600` |
| USB CDC | 已开启 |

依赖库由 PlatformIO 自动安装：

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `Adafruit AHTX0`
- `Adafruit NeoPixel`

## 三、硬件接线

总原则：

```text
ESP32 3V3  -> 3.3V 电源轨
ESP32 GND  -> GND 电源轨
外部 5V+   -> 5V 电源轨
外部 5V-   -> GND 电源轨
ESP32 GND 和外部 5V 电源 GND 必须共地
ESP32 GPIO 不能接 5V 输出信号
```

| 模块 | 模块引脚 | ESP32-S3 |
|---|---|---|
| AHT20 温湿度 | SDA | GPIO4 |
| AHT20 温湿度 | SCL | GPIO5 |
| ILI9341 屏幕 | SCK / CLK | GPIO12 |
| ILI9341 屏幕 | MOSI / SDI | GPIO11 |
| ILI9341 屏幕 | CS | GPIO10 |
| ILI9341 屏幕 | DC / RS | GPIO9 |
| ILI9341 屏幕 | RST / RESET | GPIO8 |
| ILI9341 屏幕 | BL / LED | GPIO7 |
| LD2410C 人体雷达 | OUT | GPIO16 |
| WS2812B 灯带 | DIN | GPIO18 |
| 有源蜂鸣器 | I/O | GPIO6 |
| I2S 麦克风 | SCK / BCLK | GPIO15 |
| I2S 麦克风 | WS / LRCLK | GPIO19 |
| I2S 麦克风 | SD / DOUT | GPIO20 |
| 红外发射 | DAT | GPIO14 |
| 红外接收 | DAT / OUT / S | GPIO17 |
| 按键 1 | OUT | GPIO1 |
| 按键 2 | OUT | GPIO2 |
| 按键 3 | OUT | GPIO3 |
| 按键 4 | OUT | GPIO13 |
| 按键 5 | OUT | GPIO21 |

供电建议：

- AHT20、红外接收、按键模块优先接 3.3V。
- ILI9341 优先接 3.3V；模块明确支持 5V 时 VCC 可接 5V，但信号线仍接 ESP32-S3 GPIO。
- LD2410C 接 5V。
- WS2812B 使用外部 5V 电源，并与 ESP32-S3 共地。
- 灯带 DIN 建议串 330-470 欧电阻，5V 和 GND 间建议并 1000uF 电解电容。

## 四、按键交互

代码默认按下为高电平：

```cpp
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;
```

| 按键 | 功能 |
|---|---|
| KEY1 / GPIO1 | 切换强制安防 |
| KEY2 / GPIO2 | 切换手动灯光 |
| KEY3 / GPIO3 | 切换手动空调 |
| KEY4 / GPIO13 | 蜂鸣器测试 300ms |
| KEY5 / GPIO21 | 清除报警并退出强制安防 |

如果按键实际反了，把 `BUTTON_ACTIVE_LEVEL` 改成 `LOW`。

## 五、程序结构

主循环：

```cpp
readEnvironment();
readInputs();
updateAutomation();
drawUi();
printSerialStatus();
```

| 函数 | 作用 |
|---|---|
| `setup()` | 初始化串口、GPIO、I2C、AHT20、I2S 麦克风、TFT、灯带 |
| `readEnvironment()` | 每 2 秒读取 AHT20；传感器不存在时使用模拟数据 |
| `readInputs()` | 读取人体、红外、麦克风和五个按键 |
| `readMicrophone()` | 通过 I2S 采样计算 RMS 声音强度 |
| `handleButtonPress()` | 处理五个按键动作 |
| `updateAutomation()` | 计算安防、报警、空调、蜂鸣器和灯带状态 |
| `drawUi()` | 每 500ms 刷新 ILI9341 状态界面 |
| `printSerialStatus()` | 每 2 秒输出完整调试状态 |

## 六、屏幕和串口

屏幕显示：

```text
Lexin Smart Home

温度      湿度
ROOM     OCCUPIED / EMPTY
SEC      ARMED / OFF
AC       COOLING / STANDBY
IR       RX / IDLE

底部状态条：
ALARM: NOISE!
MODE: FORCED SECURITY
MANUAL: LAMP / AC
```

串口输出示例：

```text
T=26.5C H=55% presence=1 security=0 sound=0 mic=12345 ir=0 alarm=0 ac=0 lamp=0
```

## 七、测试顺序

1. 只接 ESP32-S3，执行 `pio run`，确认编译通过。
2. 上传程序，串口出现 `Lexin Smart Home Hub booting...`。
3. 接屏幕，确认状态界面正常显示。
4. 接 AHT20，串口显示 `AHT20: OK`。
5. 接 LD2410C，人在前面时 `ROOM` 变为 `OCCUPIED`。
6. 接 WS2812B，人来灯亮，人走后自动熄灭。
7. 等无人超过 10 秒，`SEC` 变为 `ARMED`。
8. 接 I2S 麦克风，安防状态下制造较大声音，触发 `ALARM: NOISE!`。
9. 接蜂鸣器，确认报警和 KEY4 测试都能响。
10. 接红外发射，调低温度阈值或捂热 AHT20，确认串口出现空调控制日志。
11. 接红外接收，遥控器按键后确认 `IR: RX`。
12. 逐个测试 5 个按键。

## 八、验收指标

1. 系统能通过屏幕实时显示室内温湿度和设备状态。
2. 系统能通过 LD2410C 判断有人/无人。
3. 系统能在人来时自动亮灯，人走后自动进入安防。
4. 系统能在无人安防状态下检测异常声音并触发蜂鸣器报警。
5. 系统能通过 RGB 灯带反馈有人、报警等状态。
6. 系统能根据温度状态模拟红外空调控制。
7. 系统能接收红外遥控器脉冲并在屏幕显示状态。
8. 系统支持 5 个实体按键进行模式、灯光、空调、蜂鸣器测试和报警清除。
9. 系统能通过串口输出全部传感器和执行器状态，方便调试。

## 九、待优化点

1. 红外发射目前不是具体空调协议，后续需要接入真实空调码库或学习码。
2. I2S 麦克风阈值 `MIC_RMS_TRIGGER` 需要在比赛现场按噪声环境调参。
3. 屏幕刷新是全屏刷新，后续可改成局部刷新减少闪烁。
