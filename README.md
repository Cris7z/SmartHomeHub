# 乐鑫·智居 SmartHomeHub

ESP32-S3 智能家居演示项目，当前主线已经迁移到 PlatformIO，核心源码在 `src/main.cpp`。旧的 `SmartHomeHub.ino` 保留为 Arduino IDE 兼容副本，日常修改以 `src/main.cpp` 为准。

当前闭环：

```text
温湿度采集 -> 屏幕显示
人体存在检测 -> 灯光联动
无人超过 10 秒 -> 自动进入安防
安防中检测到较大声音 -> 蜂鸣器报警 + 灯带红色 + 屏幕报警
温度 >= 28C 或手动空调键 -> 红外发射演示脉冲
红外接收模块检测到遥控器脉冲 -> 屏幕显示 IR: RX
```

## 软件与构建

推荐使用 VS Code + PlatformIO。

```powershell
pio run
pio run -t upload
pio device monitor -b 115200
```

项目配置见 `platformio.ini`：

- 开发板环境：`esp32-s3-devkitc-1`
- 框架：Arduino
- 串口波特率：`115200`
- USB CDC：已通过 `build_flags` 开启
- PlatformIO 构建缓存和依赖缓存放在 `D:\A-Soft\DevTools\PlatformIO\ProjectCache\SmartHomeHub`

依赖库：

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `Adafruit AHTX0`
- `Adafruit NeoPixel`

## 硬件接线

所有 GND 必须共地。ESP32-S3 的 GPIO 不能直接接 5V 输出信号。

| 模块 | 模块引脚 | ESP32-S3 |
|---|---|---|
| AHT20 温湿度 | SDA | GPIO4 |
| AHT20 温湿度 | SCL | GPIO5 |
| ILI9341 屏幕 | SCK / CLK | GPIO12 |
| ILI9341 屏幕 | MOSI / SDI | GPIO11 |
| ILI9341 屏幕 | CS | GPIO10 |
| ILI9341 屏幕 | DC / RS | GPIO9 |
| ILI9341 屏幕 | RST | GPIO8 |
| ILI9341 屏幕 | BL / LED | GPIO7 |
| 有源蜂鸣器 | I/O | GPIO6 |
| 按键 1 | OUT | GPIO1 |
| 按键 2 | OUT | GPIO2 |
| 按键 3 | OUT | GPIO3 |
| 按键 4 | OUT | GPIO13 |
| 按键 5 | OUT | GPIO21 |
| 红外发射 | DAT | GPIO14 |
| I2S 麦克风 | SCK / BCLK | GPIO15 |
| I2S 麦克风 | WS / LRCLK | GPIO19 |
| I2S 麦克风 | SD / DOUT | GPIO20 |
| LD2410C 人体存在雷达 | OUT | GPIO16 |
| 红外接收 | DAT / OUT | GPIO17 |
| WS2812B 灯带 | DIN | GPIO18 |

供电建议：

- AHT20、红外接收、按键模块优先接 3.3V。
- ILI9341 优先接 3.3V；只有模块明确支持 5V 时，VCC 才接 5V，信号线仍接 ESP32-S3 GPIO。
- LD2410C 按常见模块接 5V。
- WS2812B 使用外部 5V 电源，外部电源 GND 与 ESP32-S3 GND 必须相连。
- WS2812B 的 DIN 建议串联 330-470 欧电阻，灯带 5V 与 GND 之间建议并联 `1000uF` 电解电容。

## 按键功能

代码默认按下为高电平：`BUTTON_ACTIVE_LEVEL = HIGH`。

| 按键 | 功能 | 串口日志 |
|---|---|---|
| KEY1 / GPIO1 | 切换强制安防模式 | `[KEY1] Force security` |
| KEY2 / GPIO2 | 切换手动灯光 | `[KEY2] Manual lamp` |
| KEY3 / GPIO3 | 切换手动空调 | `[KEY3] Manual AC` |
| KEY4 / GPIO13 | 蜂鸣器短测试 300ms | `[KEY4] Buzzer test` |
| KEY5 / GPIO21 | 清除报警并退出强制安防 | `[KEY5] Alarm/security cleared` |

如果按键实际逻辑相反，把 `BUTTON_ACTIVE_LEVEL` 改成 `LOW`。

## 屏幕与串口状态

ILI9341 屏幕显示：

```text
Lexin Smart Home
温度 / 湿度
ROOM: OCCUPIED / EMPTY
SEC:  ARMED / OFF
AC:   COOLING / STANDBY
IR:   RX / IDLE
ALARM: NOISE!
MODE: FORCED SECURITY
MANUAL: LAMP / AC
```

串口每 2 秒输出一次：

```text
T=26.5C H=55% presence=1 security=0 sound=0 mic=12345 ir=0 alarm=0 ac=0 lamp=0
```

字段含义：

| 字段 | 含义 |
|---|---|
| `T` / `H` | 温度、湿度 |
| `presence` | LD2410C 是否检测到有人 |
| `security` | 是否处于安防状态 |
| `sound` / `mic` | I2S 麦克风触发状态和 RMS 声音强度 |
| `ir` | 红外接收是否检测到脉冲 |
| `alarm` | 是否报警 |
| `ac` | 是否处于降温控制状态 |
| `lamp` | 手动灯光开关状态 |

## 测试顺序

1. 只接 ESP32-S3，编译并上传，串口出现 `Lexin Smart Home Hub booting...`。
2. 接 ILI9341，屏幕显示 `Smart Home Hub`，随后进入状态界面。
3. 接 AHT20，串口显示 `AHT20: OK`，屏幕温湿度变成真实数据。
4. 接 LD2410C，人在前方时 `ROOM` 变为 `OCCUPIED`，灯带亮暖色。
5. 离开超过 10 秒后，`SEC` 变为 `ARMED`。
6. 接 I2S 麦克风，在安防状态下制造较大声音，屏幕出现 `ALARM: NOISE!`，蜂鸣器响，灯带变红。
7. 捂热 AHT20 或临时调低 `TEMP_COOLING_THRESHOLD_C`，串口出现 `[AC] Demo IR cooling command sent`。
8. 用遥控器对准红外接收头，屏幕 `IR` 变为 `RX`。
9. 逐个测试 5 个按键，确认串口日志和屏幕底部状态变化。

## 当前限制

- 红外发射目前只是 38 kHz 演示脉冲，不是真实空调协议。
- I2S 麦克风阈值 `MIC_RMS_TRIGGER = 65000` 需要按现场噪声重新校准。
- AHT20 未接入时，代码会使用模拟温湿度，方便先调屏幕和联动。
- 屏幕当前每 500ms 全屏重绘，演示足够，但后续可以做局部刷新降低闪烁。
