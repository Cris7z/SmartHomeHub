# SmartHomeHub

SmartHomeHub 是一个基于 ESP32-S3 的智能家居中控演示项目。项目使用 PlatformIO + Arduino 框架开发，集成温湿度采集、人体存在检测、灯光联动、无人安防、声音报警、红外收发和 TFT 屏幕状态展示。

> 项目当前定位为硬件原型与比赛演示 Demo，可作为 ESP32-S3 智能家居、传感器联动和本地状态面板项目的参考起点。

## 功能特性

- AHT20 温湿度采集与屏幕显示
- LD2410C 人体存在检测
- WS2812B 灯带状态反馈
- 无人自动安防与异常声音报警
- I2S 麦克风声音强度检测
- 有源蜂鸣器报警提示
- 红外发射演示空调控制动作
- 红外接收模块检测遥控器脉冲
- 5 个实体按键控制安防、灯光、空调、蜂鸣器测试和报警清除
- 串口输出完整调试状态

## 演示逻辑

```text
有人 -> 灯带亮，安防关闭
无人超过 10 秒 -> 自动进入安防
安防中检测到较大声音 -> 蜂鸣器报警，灯带变红，屏幕显示报警
温度 >= 28C 或手动空调键开启 -> 发送红外降温演示脉冲
红外接收模块检测到遥控器脉冲 -> 屏幕显示 IR: RX
```

## 项目结构

```text
SmartHomeHub/
├─ src/
│  └─ main.cpp              # 主程序
├─ docs/
│  └─ competition.md        # 比赛展示、接线和验收说明
├─ platformio.ini           # PlatformIO 构建配置
├─ README.md                # 项目说明
└─ .gitignore               # 忽略本地构建产物和 IDE 缓存
```

主程序入口为 `src/main.cpp`。仓库不维护 Arduino IDE 的 `.ino` 副本，避免多个入口不同步。

## 硬件清单

| 模块 | 用途 |
|---|---|
| ESP32-S3 开发板 | 主控 |
| AHT20 | 温湿度采集 |
| ILI9341 SPI TFT 屏幕 | 本地状态显示 |
| LD2410C | 人体存在检测 |
| WS2812B 灯带 | 灯光和报警状态反馈 |
| I2S 麦克风 | 声音强度检测 |
| 有源蜂鸣器 | 报警提示 |
| 红外发射模块 | 空调控制演示 |
| 红外接收模块 | 遥控器脉冲检测 |
| 5 个按键模块 | 本地控制 |
| 外部 5V 电源 | 给灯带等模块供电 |

## 引脚连接

所有 GND 必须共地。ESP32-S3 的 GPIO 不能直接接 5V 输出信号。

| 模块 | 模块引脚 | ESP32-S3 |
|---|---|---|
| AHT20 | SDA | GPIO4 |
| AHT20 | SCL | GPIO5 |
| ILI9341 | SCK / CLK | GPIO12 |
| ILI9341 | MOSI / SDI | GPIO11 |
| ILI9341 | CS | GPIO10 |
| ILI9341 | DC / RS | GPIO9 |
| ILI9341 | RST | GPIO8 |
| ILI9341 | BL / LED | GPIO7 |
| 蜂鸣器 | I/O | GPIO6 |
| 按键 1 | OUT | GPIO1 |
| 按键 2 | OUT | GPIO2 |
| 按键 3 | OUT | GPIO3 |
| 按键 4 | OUT | GPIO13 |
| 按键 5 | OUT | GPIO21 |
| 红外发射 | DAT | GPIO14 |
| I2S 麦克风 | SCK / BCLK | GPIO15 |
| I2S 麦克风 | WS / LRCLK | GPIO19 |
| I2S 麦克风 | SD / DOUT | GPIO20 |
| LD2410C | OUT | GPIO16 |
| 红外接收 | DAT / OUT | GPIO17 |
| WS2812B | DIN | GPIO18 |

供电建议：

- AHT20、红外接收、按键模块优先接 3.3V。
- ILI9341 优先接 3.3V；如果屏幕模块明确支持 5V，VCC 可接 5V，但信号线仍接 ESP32-S3 GPIO。
- LD2410C 通常接 5V。
- WS2812B 使用外部 5V 电源，外部电源 GND 与 ESP32-S3 GND 必须相连。
- WS2812B 的 DIN 建议串联 330-470 欧电阻，灯带 5V 与 GND 之间建议并联 `1000uF` 电解电容。

## 环境要求

推荐使用 VS Code + PlatformIO，Windows、macOS、Linux 均可。

需要准备：

- Git
- VS Code
- PlatformIO IDE 扩展，或 PlatformIO CLI
- ESP32-S3 USB 串口驱动

确认 PlatformIO CLI 是否可用：

```bash
pio --version
```

如果命令不存在，可先安装 VS Code 的 PlatformIO IDE 扩展。首次编译时，PlatformIO 会根据 `platformio.ini` 自动安装 ESP32 平台包和依赖库。

## 快速开始

克隆仓库：

```bash
git clone https://github.com/Cris7z/SmartHomeHub.git
cd SmartHomeHub
```

编译：

```bash
pio run
```

上传：

```bash
pio run -t upload
```

打开串口监视器：

```bash
pio device monitor -b 115200
```

查看可用串口：

```bash
pio device list
```

如果需要手动指定端口：

```bash
pio run -t upload --upload-port COM10
pio device monitor --port COM10 -b 115200
```

常见端口示例：

| 系统 | 示例 |
|---|---|
| Windows | `COM10` |
| macOS | `/dev/cu.usbmodemXXXX` |
| Linux | `/dev/ttyUSB0` 或 `/dev/ttyACM0` |

## PlatformIO 配置

`platformio.ini` 当前配置：

| 项目 | 配置 |
|---|---|
| PlatformIO 环境 | `esp32-s3-devkitc-1` |
| 开发框架 | Arduino |
| 串口监视器波特率 | `115200` |
| 上传波特率 | `921600` |
| USB CDC | 已开启 |

依赖库：

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `Adafruit AHTX0`
- `Adafruit NeoPixel`

## 按键功能

代码默认按键按下为高电平：

```cpp
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;
```

| 按键 | 功能 | 串口日志 |
|---|---|---|
| KEY1 / GPIO1 | 切换强制安防模式 | `[KEY1] Force security` |
| KEY2 / GPIO2 | 切换手动灯光 | `[KEY2] Manual lamp` |
| KEY3 / GPIO3 | 切换手动空调 | `[KEY3] Manual AC` |
| KEY4 / GPIO13 | 蜂鸣器短测试 300ms | `[KEY4] Buzzer test` |
| KEY5 / GPIO21 | 清除报警并退出强制安防 | `[KEY5] Alarm/security cleared` |

如果按键模块输出逻辑相反，将 `BUTTON_ACTIVE_LEVEL` 改为 `LOW`。

## 屏幕与串口状态

ILI9341 屏幕显示内容：

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

串口每 2 秒输出一次状态：

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

## 调试建议

建议按以下顺序逐步接线和验证：

1. 只接 ESP32-S3，执行 `pio run`，确认编译通过。
2. 执行 `pio run -t upload` 上传程序。
3. 执行 `pio device monitor -b 115200`，确认串口出现 `Lexin Smart Home Hub booting...`。
4. 接 ILI9341，确认状态界面正常显示。
5. 接 AHT20，确认串口显示 `AHT20: OK`。
6. 接 LD2410C，人在前方时 `ROOM` 变为 `OCCUPIED`。
7. 接 WS2812B，确认人来灯亮，人走后自动熄灭。
8. 等无人超过 10 秒，确认 `SEC` 变为 `ARMED`。
9. 接 I2S 麦克风，在安防状态下制造较大声音，确认触发 `ALARM: NOISE!`。
10. 接蜂鸣器，确认报警和 KEY4 测试都能响。
11. 接红外发射，捂热 AHT20 或临时调低 `TEMP_COOLING_THRESHOLD_C`，确认串口出现空调控制日志。
12. 接红外接收，使用遥控器按键后确认 `IR: RX`。
13. 逐个测试 5 个按键。

## 可调参数

常用参数集中在 `src/main.cpp` 顶部：

| 参数 | 默认值 | 说明 |
|---|---|---|
| `BUTTON_ACTIVE_LEVEL` | `HIGH` | 按键触发电平 |
| `PRESENCE_ACTIVE_LEVEL` | `HIGH` | LD2410C OUT 有人触发电平 |
| `IR_RX_ACTIVE_LEVEL` | `LOW` | 红外接收触发电平 |
| `MIC_RMS_TRIGGER` | `65000` | I2S 麦克风报警阈值 |
| `TEMP_COOLING_THRESHOLD_C` | `28.0` | 空调模拟触发温度 |
| `EMPTY_TO_SECURITY_MS` | `10000` | 无人进入安防的等待时间 |
| `ALARM_HOLD_MS` | `5000` | 报警保持时间 |
| `AC_COMMAND_GAP_MS` | `15000` | 红外空调指令发送间隔 |

## 当前限制

- 红外发射目前只是 38 kHz 演示脉冲，不是真实空调协议。
- I2S 麦克风阈值需要按现场噪声环境重新校准。
- AHT20 未接入时，代码会使用模拟温湿度，方便先调屏幕和联动。
- 屏幕当前每 500ms 全屏重绘，演示足够，但后续可改成局部刷新以降低闪烁。

## 贡献

欢迎提交 issue 或 pull request。建议在提交前先运行：

```bash
pio run
```

请不要提交本地生成文件，例如 `.pio/`、`.cache/`、`compile_commands.json` 和个人 IDE 配置。

## 许可证

当前仓库尚未指定开源许可证。正式公开发布前建议补充 `LICENSE` 文件。
