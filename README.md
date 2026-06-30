# 乐鑫·智居 SmartHomeHub

ESP32-S3 智能家居中控演示项目。当前仓库以 PlatformIO 为主线，主程序入口是 `src/main.cpp`。

## 项目结构

```text
SmartHomeHub/
├─ src/
│  └─ main.cpp              # 主程序
├─ docs/
│  └─ competition.md        # 比赛展示、接线和验收说明
├─ platformio.ini           # PlatformIO 构建配置
├─ README.md                # 环境配置和快速上手
└─ .gitignore               # 忽略本地构建产物和 IDE 缓存
```

仓库里不再保留 `SmartHomeHub.ino` 副本，避免 Arduino IDE 版本和 PlatformIO 版本不同步。后续改代码只改 `src/main.cpp`。

## 环境配置

### 1. 必需软件

推荐使用 VS Code + PlatformIO，Windows、macOS、Linux 都可以。

需要准备：

- Git
- VS Code
- PlatformIO IDE 扩展，或 PlatformIO CLI
- ESP32-S3 数据线
- 对应系统的 ESP32-S3 USB 串口驱动

也可以直接使用 PlatformIO CLI：

```bash
pio --version
```

如果命令不存在，先安装 VS Code 的 PlatformIO IDE 扩展。PlatformIO 会自动安装 ESP32 平台包和项目依赖库。

### 2. 克隆并打开项目

这是私有仓库，队友需要先被加入 collaborator 或组织成员，才能克隆。

克隆仓库：

```bash
git clone https://github.com/Cris7z/SmartHomeHub.git
cd SmartHomeHub
```

然后用 VS Code 打开仓库根目录。PlatformIO 会根据 `platformio.ini` 自动拉取 ESP32 平台和依赖库。

这些都是本地生成文件，不要提交：

- `.pio/`
- `.cache/`
- `compile_commands.json`
- `.vscode/c_cpp_properties.json`
- `.vscode/launch.json`

### 3. 开发板配置

`platformio.ini` 当前配置：

- 开发板环境：`esp32-s3-devkitc-1`
- 框架：Arduino
- 串口监视器波特率：`115200`
- 上传波特率：`921600`
- USB CDC：已开启

依赖库：

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `Adafruit AHTX0`
- `Adafruit NeoPixel`

### 4. 常用命令

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

查看当前设备端口：

```bash
pio device list
```

如果上传失败，先确认 VS Code / PlatformIO 识别到正确串口。不同系统的端口名字不同：

| 系统 | 常见端口示例 |
|---|---|
| Windows | `COM10` |
| macOS | `/dev/cu.usbmodemXXXX` |
| Linux | `/dev/ttyUSB0` 或 `/dev/ttyACM0` |

可以临时指定端口，不需要改 `platformio.ini`：

```bash
pio run -t upload --upload-port COM10
pio device monitor --port COM10 -b 115200
```

把 `COM10` 换成自己电脑上实际显示的端口。

## 协作约定

- 项目主线只维护 `src/main.cpp`。
- 不提交本机绝对路径、个人串口号、IDE 缓存和构建产物。
- 如果只是在自己电脑上临时指定端口，优先用命令行参数或 VS Code PlatformIO 面板，不要直接改 `platformio.ini`。
- 如果确实需要改公共配置，先和队友确认开发板型号、串口参数和依赖版本。

## 当前功能

```text
温湿度采集 -> 屏幕显示
人体存在检测 -> 灯光联动
无人超过 10 秒 -> 自动进入安防
安防中检测到较大声音 -> 蜂鸣器报警 + 灯带红色 + 屏幕报警
温度 >= 28C 或手动空调键 -> 红外发射演示脉冲
红外接收模块检测到遥控器脉冲 -> 屏幕显示 IR: RX
```

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

## 调试顺序

1. 只接 ESP32-S3，执行 `pio run`，确认编译通过。
2. 执行 `pio run -t upload` 上传程序。
3. 执行 `pio device monitor -b 115200`，确认串口出现 `Lexin Smart Home Hub booting...`。
4. 接 ILI9341，屏幕显示 `Smart Home Hub`，随后进入状态界面。
5. 接 AHT20，串口显示 `AHT20: OK`，屏幕温湿度变成真实数据。
6. 接 LD2410C，人在前方时 `ROOM` 变为 `OCCUPIED`，灯带亮暖色。
7. 离开超过 10 秒后，`SEC` 变为 `ARMED`。
8. 接 I2S 麦克风，在安防状态下制造较大声音，屏幕出现 `ALARM: NOISE!`，蜂鸣器响，灯带变红。
9. 捂热 AHT20 或临时调低 `TEMP_COOLING_THRESHOLD_C`，串口出现 `[AC] Demo IR cooling command sent`。
10. 用遥控器对准红外接收头，屏幕 `IR` 变为 `RX`。
11. 逐个测试 5 个按键，确认串口日志和屏幕底部状态变化。

## 当前限制

- 红外发射目前只是 38 kHz 演示脉冲，不是真实空调协议。
- I2S 麦克风阈值 `MIC_RMS_TRIGGER = 65000` 需要按现场噪声重新校准。
- AHT20 未接入时，代码会使用模拟温湿度，方便先调屏幕和联动。
- 屏幕当前每 500ms 全屏重绘，演示足够，但后续可以做局部刷新降低闪烁。
