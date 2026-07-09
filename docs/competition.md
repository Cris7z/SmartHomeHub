# 嵌赛项目说明

项目名称：乐鑫·智居 SmartHomeHub

版本：持续开发中，稳定演示节点见 Git tags

主控：ESP32-S3

代码入口：`src/main.cpp`

参数配置：`src/board/config.h`

构建方式：PlatformIO，配置文件为 `platformio.ini`

## 一、项目定位

这是一个智能家居中控演示系统，围绕环境感知、人在检测、灯光联动、无人安防、声音报警、红外空调模拟控制、小智 AI 语音演示链路和屏幕状态展示做完整闭环。

核心演示逻辑：

```text
有人 -> 灯带亮，安防关闭
无人超过 10 秒 -> 自动进入安防
安防状态下检测到大声音 -> 蜂鸣器报警，灯带红色闪烁，安静 5 秒后自动恢复
温度 >= 28C 或按下空调键 -> 发送红外降温演示脉冲
遥控器红外信号进入接收头 -> 屏幕显示 IR: RX
小智 AI -> 麦克风声音触发或 Web/BLE 命令触发 -> 豆包实时语音识别和回复 -> MAX98357A 扬声器播放 TTS -> XIAOZHI 页显示状态
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
| PlatformIO 环境 | `esp32-s3-devkitc-1-n16r8` |
| 框架 | Arduino |
| 串口监视器 | `115200` |
| 上传速度 | `921600` |
| 串口日志 | CH340 / UART0；USB CDC 默认关闭 |

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
| I2S 麦克风 | WS / LRCLK | GPIO39 |
| I2S 麦克风 | SD / DOUT | GPIO40 |
| MAX98357A I2S 功放 | DIN | GPIO47 |
| MAX98357A I2S 功放 | BCLK | GPIO42 |
| MAX98357A I2S 功放 | LRC / LRCLK | GPIO38 |
| 红外发射 | DAT | GPIO14 |
| 红外接收 | DAT / OUT / S | GPIO17 |
| 按键 1 | OUT | GPIO1 |
| 按键 2 | OUT | GPIO2 |
| 按键 3 | OUT | GPIO41 |
| 按键 4 | OUT | GPIO13 |
| 按键 5 | OUT | GPIO21 |

供电建议：

- AHT20、红外接收、按键模块优先接 3.3V。
- ILI9341 优先接 3.3V；模块明确支持 5V 时 VCC 可接 5V，但信号线仍接 ESP32-S3 GPIO。
- LD2410C 接 5V。
- MAX98357A 功放通常接 5V，GND 与 ESP32-S3 共地；DIN/BCLK/LRC 为 3.3V 逻辑信号。
- WS2812B 使用外部 5V 电源，并与 ESP32-S3 共地。
- 灯带 DIN 建议串 330-470 欧电阻，5V 和 GND 间建议并 1000uF 电解电容。
- GPIO19/20 保留给 ESP32-S3 原生 USB，GPIO3 属于启动绑带脚，避免外接模块占用。

## 四、按键交互

代码默认按下为高电平：

```cpp
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;
```

| 按键 | 功能 |
|---|---|
| KEY1 / GPIO1 | 切换强制安防 |
| KEY2 / GPIO2 | 切换手动灯光 |
| KEY3 / GPIO41 | 切换手动空调 |
| KEY4 / GPIO13 | 切换屏幕页面 |
| KEY5 / GPIO21 | 清除报警并退出强制安防 |

如果按键实际反了，把 `BUTTON_ACTIVE_LEVEL` 改成 `LOW`。

## 五、程序结构

主循环保留在 `src/main.cpp`：

```cpp
readEnvironment();
readInputs();
updateAutomation();
drawUi();
printSerialStatus();
```

文件职责：

| 文件 | 作用 |
|---|---|
| `src/main.cpp` | 初始化和主循环调度 |
| `src/board/config.h` | 引脚、触发电平、阈值和时间参数 |
| `src/board/hardware.cpp` | TFT、AHT20、灯带对象，蜂鸣器、红外发射、I2S 麦克风和 I2S 扬声器初始化 |
| `src/board/speaker_tone.*` | MAX98357A 扬声器提示音 PCM 帧生成 |
| `src/io/sensors.cpp` | AHT20 温湿度读取和 I2S 麦克风采样 |
| `src/io/mic_processing.*` | I2S 麦克风有效槽位选择、去直流、RMS 和百分比换算 |
| `src/io/inputs.cpp` | 人体、红外接收和按键读取 |
| `src/app/hub_state.cpp` | 系统状态结构 |
| `src/app/automation.cpp` | 安防、报警、空调和灯光联动 |
| `src/app/xiaozhi_core.*` / `src/app/xiaozhi_ai.*` | 小智 AI 演示状态机、本地回复和提示音触发 |
| `src/app/display.cpp` | ILI9341 屏幕状态界面 |
| `src/app/diagnostics.cpp` | 串口状态输出 |

核心函数：

| 函数 | 作用 |
|---|---|
| `setup()` | 初始化串口、GPIO、I2C、AHT20、I2S 麦克风、I2S 扬声器、TFT、灯带和小智状态机 |
| `readEnvironment()` | 每 2 秒读取 AHT20；传感器不存在时使用模拟数据 |
| `readInputs()` | 读取人体、红外、麦克风和五个按键 |
| `readMicrophone()` | 通过 I2S 采样计算去直流和平滑后的相对声音强度 |
| `handleButtonPress()` | 处理五个按键动作 |
| `updateAutomation()` | 计算安防、报警、空调、蜂鸣器和灯带状态 |
| `updateXiaozhiAi()` | 根据麦克风触发或 Web/BLE 命令运行小智本地演示链路 |
| `drawUi()` | 局部刷新 ILI9341 五页状态界面 |
| `printSerialStatus()` | 每 2 秒输出完整调试状态 |

## 六、屏幕和串口

屏幕显示：

```text
HOME      室内温湿度、ROOM、SEC、AC、IR
WEATHER   室外温度、天气、城市、日出、日落、Wi-Fi
SYSTEM    噪声百分比、BLE、声音状态、触发线、灯光模式、IR TEST
AI GUARD  风险分数、AI 状态、最近 4 条事件日志
XIAOZHI   小智状态、扬声器状态、本地回复、云端配置和麦克风自动触发状态
```

串口输出示例：

```text
T=26.5C H=55% presence=1 security=0 sound=0 mic=12345 micPct=19% ir=0 alarm=0 ac=0 lamp=0
```

## 七、测试顺序

1. 只接 ESP32-S3，执行 `pio run`，确认编译通过。
2. 上传程序，串口出现 `Lexin Smart Home Hub booting...`。
3. 接屏幕，确认状态界面正常显示。
4. 接 AHT20，串口显示 `AHT20: OK`。
5. 接 LD2410C，人在前面时 `ROOM` 变为 `OCCUPIED`。
6. 接 WS2812B，人来灯亮，人走后自动熄灭。
7. 等无人超过 10 秒，`SEC` 变为 `ARMED`。
8. 接 I2S 麦克风，切到 SYSTEM 页观察 `NOISE` 百分比；安防状态下制造较大声音，接近或超过 `100%` 后触发 `ALARM: NOISE!`，灯带红色闪烁；停止制造噪声后约 5 秒自动恢复。
9. 接 MAX98357A I2S 功放和扬声器，确认 DIN/BCLK/LRC 分别为 GPIO47/GPIO42/GPIO38；串口显示 `I2S speaker: OK`，触发小智时应有短提示音。没有扬声器时可以先看 XIAOZHI 页和串口日志。
9.1 配置豆包实时语音：复制 `tools/doubao_relay/.env.example` 为 `tools/doubao_relay/.env`，只在本地填写 API 凭据；执行 `tools\doubao_relay\setup.ps1`；再执行 `tools\doubao_relay\run.ps1 --probe`，必须出现 `AUTH OK`；启动 `tools\doubao_relay\run.ps1`，把脚本打印的 `ws://<relay-host-ip>:8765/voice` 写入忽略文件 `src/net/secrets.h` 的 `SMART_HOME_DOUBAO_RELAY_URL`；最后重新上传固件。
9.2 实时语音验收接线保持：`INMP441: BCLK GPIO15, WS GPIO39, SD GPIO40`，`MAX98357A: DIN GPIO47, BCLK GPIO42, LRC GPIO38`。对麦克风说一句中文，要求串口依次出现 `[VOICE] relay connected`、`[DOUBAO] session started`、`[DOUBAO] asr=`、`[DOUBAO] reply=`、`[VOICE] playback complete`，并且声音从 MAX98357A 扬声器输出，浏览器和开发主机扬声器不发声。
10. 接蜂鸣器，确认报警时蜂鸣器能响。
11. 接红外发射，调低温度阈值或捂热 AHT20，确认串口出现空调控制日志。
12. 接红外接收，遥控器按键后确认 `IR: RX`。
13. 逐个测试 5 个按键。

## 八、验收指标

1. 系统能通过屏幕实时显示室内温湿度和设备状态。
2. 系统能通过 LD2410C 判断有人/无人。
3. 系统能在人来时自动亮灯，人走后自动进入安防。
4. 系统能在无人安防状态下检测异常声音并触发蜂鸣器报警。
5. 系统能通过 RGB 灯带反馈有人、报警等状态，报警时红色闪烁。
6. 系统能根据温度状态模拟红外空调控制。
7. 系统能接收红外遥控器脉冲并在屏幕显示状态。
8. 系统支持 5 个实体按键进行安防、灯光、空调、屏幕翻页和报警清除。
9. 系统能通过串口输出全部传感器和执行器状态，方便调试。
10. 系统能通过麦克风阈值、Web Dashboard 或 BLE 命令触发小智 AI 实时语音链路，并在 XIAOZHI 页显示识别、回复和播放状态。

## 九、待优化点

1. 红外发射目前不是具体空调协议，后续需要接入真实空调码库或学习码。
2. I2S 麦克风输出的是相对声音强度，不是标准 dB；阈值 `MIC_RMS_TRIGGER`、自适应余量和平滑权重需要在比赛现场按噪声环境调参。
3. 屏幕已改为局部刷新；如现场仍有闪烁，优先检查供电、SPI 接线和屏幕背光稳定性。
4. 小智 AI 已接入局域网 relay 和豆包实时语音；正式演示前必须完成 `--probe` 鉴权、relay 常驻和一次实物语音验收。
