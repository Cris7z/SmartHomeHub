# SmartHomeHub

当前版本：V1.1

SmartHomeHub 是一个基于 ESP32-S3 的智能家居中控演示项目。项目使用 PlatformIO + Arduino 框架开发，集成温湿度采集、人体存在检测、灯光联动、无人安防、声音报警、红外收发、时间天气、远程预警、Web 控制台、BLE 控制、小智 AI 演示链路和 TFT 屏幕状态展示。

> 项目当前定位为硬件原型与比赛演示 Demo，可作为 ESP32-S3 智能家居、传感器联动和本地状态面板项目的参考起点。

## 功能特性

- AHT20 温湿度采集与屏幕显示
- LD2410C 人体存在检测
- WS2812B 灯带状态反馈
- 无人自动安防与异常声音报警
- I2S 麦克风去直流、平滑后的相对声音强度检测
- 有源蜂鸣器报警提示
- 红外发射演示空调控制动作
- 红外接收模块检测遥控器脉冲
- IP 城市定位、NTP 时间同步和 Open-Meteo 天气 / 日出日落显示
- PushPlus 远程报警推送
- 浏览器 Web Dashboard 状态查看和控制
- BLE 状态广播和命令控制
- Web / BLE 快捷键宏场景：Home、Away、Night
- AI Guard 风险评分和最近事件日志
- 小智 AI 演示框架：麦克风自动触发、状态机、本地回复、I2S 扬声器提示音和独立屏幕页
- 5 个实体按键控制安防、灯光、空调、屏幕翻页和报警清除
- 串口输出完整调试状态

## 演示逻辑

```text
有人 -> 灯带亮，安防关闭
无人超过 10 秒 -> 自动进入安防
安防中检测到较大声音 -> 蜂鸣器报警，灯带红色闪烁，安静 5 秒后自动恢复
温度 >= 28C 或手动空调键开启 -> 发送红外降温演示脉冲
红外接收模块检测到遥控器脉冲 -> 屏幕显示 IR: RX
连接 Wi-Fi -> IP 定位城市、同步时间、拉取当地天气和日出日落、启动 Web Dashboard
安防报警 -> 本地蜂鸣器/灯带报警，并在配置 PushPlus 后发送远程预警
快捷键宏 -> Home 回到居家自动模式，Away 开启离家安防，Night 开启夜间安防并显示天气时间页
小智 AI -> 麦克风声音触发或 Web/BLE 命令触发 -> 豆包实时语音识别和回复 -> MAX98357A 扬声器播放 TTS -> XIAOZHI 页显示状态
```

## 项目结构

```text
SmartHomeHub/
├─ src/
│  ├─ main.cpp              # 初始化和主循环
│  ├─ app/                  # 应用状态、自动控制、显示、串口诊断
│  ├─ board/                # 引脚配置、硬件对象和底层输出
│  ├─ io/                   # 输入读取、传感器采样
│  └─ net/                  # Wi-Fi、PushPlus、天气、Web Dashboard、BLE
├─ docs/
│  ├─ competition.md        # 比赛展示、接线和验收说明
│  └─ branching.md          # 分支模型和版本管理说明
├─ CONTRIBUTING.md          # 协作、提交和版本管理规范
├─ platformio.ini           # PlatformIO 构建配置
├─ README.md                # 项目说明
└─ .gitignore               # 忽略本地构建产物和 IDE 缓存
```

主程序入口为 `src/main.cpp`。仓库不维护 Arduino IDE 的 `.ino` 副本，避免多个入口不同步。

源码按嵌入式项目常见边界拆分：

| 路径 | 职责 |
|---|---|
| `src/main.cpp` | 启动初始化和 superloop 调度 |
| `src/board/config.h` | 引脚、阈值、时间参数 |
| `src/board/hardware.*` | TFT、AHT20、灯带、I2S 麦克风和 I2S 扬声器等硬件对象和低层输出 |
| `src/board/speaker_tone.*` | MAX98357A 扬声器提示音 PCM 帧生成 |
| `src/io/sensors.*` | AHT20 和 I2S 麦克风采样 |
| `src/io/inputs.*` | 人体、红外接收、按键读取 |
| `src/app/hub_state.*` | 全局状态结构 |
| `src/app/automation.*` | 安防、报警、空调和灯光联动状态机 |
| `src/app/controls.*` | 按键、Web、BLE 共用的控制命令入口 |
| `src/app/shortcut_macro.*` | Home / Away / Night 快捷键宏场景预设 |
| `src/app/ai_guard.*` | 基于状态和声音强度计算 AI Guard 风险分数 |
| `src/app/xiaozhi_core.*` | 小智 AI 状态名、触发策略和本地回复格式化 |
| `src/app/xiaozhi_ai.*` | 小智 AI 演示状态机和扬声器提示音触发 |
| `src/app/event_log.*` | 最近事件日志 |
| `src/app/display.*` | ILI9341 五页屏幕界面 |
| `src/app/diagnostics.*` | 串口状态输出 |
| `src/net/pushplus.*` | Wi-Fi STA 连接和 PushPlus 报警推送 |
| `src/net/time_weather.*` | IP 城市定位、NTP 时间同步和 Open-Meteo 天气拉取 |
| `src/net/web_dashboard.*` | HTTP Web Dashboard 和 JSON API |
| `src/net/ble_service.*` | BLE 状态特征和命令特征 |

## 硬件清单

| 模块 | 用途 |
|---|---|
| ESP32-S3 开发板 | 主控 |
| AHT20 | 温湿度采集 |
| ILI9341 SPI TFT 屏幕 | 本地状态显示 |
| LD2410C | 人体存在检测 |
| WS2812B 灯带 | 灯光和报警状态反馈 |
| I2S 麦克风 | 声音强度检测 |
| MAX98357A I2S 功放 + 扬声器 | 小智 AI 语音输出 |
| 有源蜂鸣器 | 报警提示 |
| 红外发射模块 | 空调控制演示 |
| 红外接收模块 | 遥控器脉冲检测 |
| 5 个按键模块 | 本地控制和屏幕翻页 |
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
| 按键 3 | OUT | GPIO41 |
| 按键 4 | OUT | GPIO13 |
| 按键 5 | OUT | GPIO21 |
| 红外发射 | DAT | GPIO14 |
| I2S 麦克风 | SCK / BCLK | GPIO15 |
| I2S 麦克风 | WS / LRCLK | GPIO39 |
| I2S 麦克风 | SD / DOUT | GPIO40 |
| MAX98357A I2S 功放 | DIN | GPIO47 |
| MAX98357A I2S 功放 | BCLK | GPIO42 |
| MAX98357A I2S 功放 | LRC / LRCLK | GPIO38 |
| LD2410C | OUT | GPIO16 |
| 红外接收 | DAT / OUT | GPIO17 |
| WS2812B | DIN | GPIO18 |

供电建议：

- AHT20、红外接收、按键模块优先接 3.3V。
- ILI9341 优先接 3.3V；如果屏幕模块明确支持 5V，VCC 可接 5V，但信号线仍接 ESP32-S3 GPIO。
- LD2410C 通常接 5V。
- MAX98357A 功放通常接 5V，GND 与 ESP32-S3 共地；DIN/BCLK/LRC 为 3.3V 逻辑信号。
- WS2812B 使用外部 5V 电源，外部电源 GND 与 ESP32-S3 GND 必须相连。
- WS2812B 的 DIN 建议串联 330-470 欧电阻，灯带 5V 与 GND 之间建议并联 `1000uF` 电解电容。
- GPIO19/20 保留给 ESP32-S3 原生 USB，GPIO3 属于启动绑带脚，避免外接模块占用。

## 环境要求

推荐使用 VS Code + PlatformIO，Windows、macOS、Linux 均可。

需要准备：

- Git
- VS Code
- PlatformIO IDE 扩展，或 PlatformIO CLI
- ESP32-S3 USB 串口驱动
- 可联网的 2.4 GHz Wi-Fi，用于时间、天气、Web Dashboard 和远程预警
- 可选：PushPlus token，用于手机远程报警推送

确认 PlatformIO CLI 是否可用：

```bash
pio --version
```

如果命令不存在，可先安装 VS Code 的 PlatformIO IDE 扩展。首次编译时，PlatformIO 会根据 `platformio.ini` 自动安装 ESP32 平台包和依赖库。

Windows 下建议将仓库放在纯英文路径中，避免 ESP32 工具链在生成链接产物时遇到非 ASCII 路径编码问题。

## 联网配置

联网功能默认不会把账号密码提交到仓库。需要使用 Wi-Fi、天气、Web Dashboard、PushPlus 或后续小智云端接口时，复制示例文件：

```bash
copy src\net\secrets_example.h src\net\secrets.h
```

然后编辑 `src/net/secrets.h`：

```cpp
#define SMART_HOME_WIFI_SSID "your_wifi"
#define SMART_HOME_WIFI_PASS "your_password"
#define SMART_HOME_PUSHPLUS_TOKEN "your_pushplus_token"
#define SMART_HOME_XIAOZHI_ENDPOINT ""
#define SMART_HOME_XIAOZHI_TOKEN ""
#define SMART_HOME_WEATHER_LAT "24.4798"
#define SMART_HOME_WEATHER_LON "118.0894"
```

天气会优先通过公网 IP 自动定位到城市级经纬度，再请求 Open-Meteo 的当地天气、日出和日落时间。`SMART_HOME_WEATHER_LAT` / `SMART_HOME_WEATHER_LON` 只作为 IP 定位失败时的备用坐标。

如果不创建 `secrets.h`，固件仍可运行本地传感器、屏幕、按键、灯带、红外、蜂鸣器和小智 AI 本地演示功能；Wi-Fi、天气、网页控制和 PushPlus 会保持未连接或未配置状态。

## 豆包实时语音配置

小智实时语音链路使用 ESP32-S3 采集 INMP441 音频，通过局域网 WebSocket 发给电脑上的 `tools/doubao_relay`，再由电脑桥接到豆包实时语音 API。电脑只做协议转发，不播放声音；最终 TTS 音频通过 MAX98357A 从开发板扬声器输出。

配置顺序：

1. 删除截图里暴露过的 API Key。
2. 重新创建一个豆包语音 API Key，只写入 `tools/doubao_relay/.env`。
3. 执行一次 `tools\doubao_relay\setup.ps1`。
4. 执行 `D:\A-Soft\DevTools\SmartHomeHubVoiceRelay\.venv\Scripts\python.exe -m tools.doubao_relay.server --probe`，必须看到 `AUTH OK`。
5. 把 `run.ps1` 打印出来的局域网地址写入被 git 忽略的 `src/net/secrets.h`：

```cpp
#define SMART_HOME_DOUBAO_RELAY_URL "ws://电脑局域网IP:8765/voice"
```

6. 启动 `tools\doubao_relay\run.ps1`，再编译上传固件。

`.env` 的空模板在 `tools/doubao_relay/.env.example`，复制后只在本机填写。

队友 clone 后还需要按 [CONTRIBUTING.md](CONTRIBUTING.md) 的“队友克隆规则”创建自己的 `src/net/secrets.h` 和 `tools/doubao_relay/.env`。这些本地密钥、Wi-Fi 和 relay IP 不提交到 Git。

实物接线保持不变：

```text
INMP441: BCLK GPIO15, WS GPIO39, SD GPIO40
MAX98357A: DIN GPIO47, BCLK GPIO42, LRC GPIO38
```

验收串口标记：

```text
[VOICE] relay connected
[DOUBAO] session started
[DOUBAO] asr=
[DOUBAO] reply=
[VOICE] playback complete
```

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
| 默认串口 | `COM8` |
| 上传波特率 | `921600` |
| USB CDC | 已开启 |

如果你的开发板不是 `COM8`，可以先执行 `pio device list` 查看实际端口，再修改 `platformio.ini` 中的 `monitor_port` / `upload_port`，或使用命令行临时指定端口。

依赖库：

- `Adafruit GFX Library`
- `Adafruit ILI9341`
- `Adafruit AHTX0`
- `Adafruit NeoPixel`
- Arduino ESP32 框架内置 `WiFi`、`HTTPClient`、`WebServer` 和 `ESP32 BLE Arduino`

## 按键功能

代码默认按键按下为高电平：

```cpp
constexpr int BUTTON_ACTIVE_LEVEL = HIGH;
```

| 按键 | 功能 | 串口日志 |
|---|---|---|
| KEY1 / GPIO1 | 切换强制安防模式 | `[KEY1] Force security` |
| KEY2 / GPIO2 | 切换手动灯光覆盖 | `[KEY2] Manual lamp override` |
| KEY3 / GPIO41 | 切换手动空调并开启 5 秒红外诊断发射 | `[KEY3] Manual AC` / `[IR] Diagnostic burst` |
| KEY4 / GPIO13 | 切换屏幕页面 | `[KEY4] Display page` |
| KEY5 / GPIO21 | 清除报警并退出强制安防 | `[KEY5] Alarm/security cleared` |

如果按键模块输出逻辑相反，将 `BUTTON_ACTIVE_LEVEL` 改为 `LOW`。

## 屏幕与串口状态

ILI9341 屏幕有 5 个页面，KEY4、Web Dashboard 或 BLE 命令 `page` 可以切换：

```text
HOME      室内温湿度、ROOM、SEC、AC、IR
WEATHER   室外温度、天气、城市、日出、日落、Wi-Fi
SYSTEM    噪声百分比、BLE、声音状态、触发线、灯光模式、IR TEST
AI GUARD  风险分数、AI 状态、最近 4 条事件日志
XIAOZHI   小智状态、扬声器状态、本地回复、云端配置和麦克风自动触发状态
```

顶部显示时间和心跳点；底部显示页面指示点，报警时底部改为 `ALARM: NOISE!`。

串口每 2 秒输出一次状态：

```text
T=26.5C H=55% presence=1 security=0 sound=0 mic=12345 micPct=19% base=10000 thr=65000 risk=25/LOW ir=0 alarm=0 ac=0 lamp=0 lampOverride=0 page=0 xz=IDLE spk=1 play=0 ble=0 sta=1 ip=192.168.1.10 push=1 time=12:30 weather=Clear out=28.5C loc=Xiamen rise=05:24 set=19:00
```

字段含义：

| 字段 | 含义 |
|---|---|
| `T` / `H` | 温度、湿度 |
| `presence` | LD2410C 是否检测到有人 |
| `security` | 是否处于安防状态 |
| `sound` / `mic` / `micPct` | I2S 麦克风触发状态、平滑 RMS 和相对触发阈值百分比 |
| `base` / `thr` | 自适应麦克风基线和当前触发阈值 |
| `risk` | AI Guard 风险分数和等级 |
| `ir` | 红外接收是否检测到脉冲 |
| `alarm` | 是否报警 |
| `ac` | 是否处于降温控制状态 |
| `lamp` / `lampOverride` | 灯光实际手动状态和是否覆盖自动联动 |
| `page` | 当前屏幕页码 |
| `xz` | 小智 AI 当前状态：`IDLE`、`LISTEN`、`THINK`、`SPEAK` |
| `spk` / `play` | I2S 扬声器驱动是否初始化、当前是否正在输出提示音 |
| `ble` | 是否有 BLE 客户端连接 |
| `sta` / `ip` | Wi-Fi STA 是否连接和本机 IP |
| `push` | PushPlus token 是否已配置 |
| `time` / `weather` / `out` | NTP 时间、天气文本和室外温度 |
| `loc` / `rise` / `set` | IP 定位城市、当地日出和日落时间 |

## Web Dashboard 和 BLE 控制

配置 Wi-Fi 后，固件会在联网成功时启动网页控制台。串口会打印：

```text
[WEB] dashboard ready: http://192.168.x.x/
```

浏览器打开该地址后可以查看室内外环境、城市天气、日出日落、房间状态、安防状态、AI Guard、网络状态和最近事件，并通过按钮执行：

| Web 按钮 / BLE 命令 | 功能 |
|---|---|
| `security` | 切换强制安防 |
| `lamp` | 切换手动灯光覆盖 |
| `ac` | 切换手动空调并启动红外诊断发射 |
| `page` / `mode` | 切换屏幕页面 |
| `clear` | 清除报警并退出强制安防 |
| `home` / `macro_home` | 居家宏：退出强制安防，恢复灯光自动联动，回到 HOME 页 |
| `away` / `macro_away` | 离家宏：开启强制安防，灯光手动关闭，切到 AI GUARD 页 |
| `night` / `macro_night` | 夜间宏：开启强制安防，灯光手动关闭，切到 WEATHER 页 |
| `xiaozhi` / `ai` / `voice` | 手动触发小智 AI 演示链路，跳到 XIAOZHI 页并播放提示音 |

Web JSON 状态接口：

```text
http://板子IP/api/state
```

BLE 设备名为 `SmartHomeHub`。BLE 服务使用 Nordic UART 风格 UUID：

| 项目 | UUID |
|---|---|
| Service | `6e400001-b5a3-f393-e0a9-e50e24dcca9e` |
| Command Write | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` |
| State Read/Notify | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` |

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
9. 接 I2S 麦克风，切到 SYSTEM 页观察 `NOISE` 百分比；在安防状态下制造较大声音，确认接近或超过 `100%` 后触发 `ALARM: NOISE!`，灯带红色闪烁；停止制造噪声后约 5 秒自动恢复。
10. 接 MAX98357A I2S 功放和扬声器，确认 DIN/BCLK/LRC 分别为 GPIO47/GPIO42/GPIO38；串口应显示 `I2S speaker: OK`，触发小智时会输出短提示音。
11. 接蜂鸣器，确认报警时蜂鸣器响。
12. 接红外发射，按 KEY3，确认串口连续出现 `[IR] Diagnostic burst`，并用手机摄像头观察红外发射头。
13. 接红外接收，使用遥控器按键后确认 `IR: RX`。
14. 逐个测试 5 个按键：KEY1 安防、KEY2 灯光、KEY3 空调/红外、KEY4 翻页、KEY5 清除。
15. 配置 `src/net/secrets.h` 后重启，确认串口 `[WiFi] status=CONNECTED`、`[Location] 城市 经纬度 时区` 和 `[Weather] 城市 温度 ... rise=... set=...`。
16. 切到 WEATHER 页，确认顶部时间正常，页面显示室外温度、天气、城市、日出、日落和 Wi-Fi 状态。
17. 浏览器打开 `http://板子IP/`，确认 Web Dashboard 能显示状态并能执行按钮控制。
18. 打开 `http://板子IP/api/state`，确认返回 JSON 状态，包含 `location`、`timezone`、`sunrise`、`sunset`。
19. 用手机 BLE 工具搜索 `SmartHomeHub`，读取 State 特征，向 Command 特征写入 `security`、`lamp`、`ac`、`page`、`clear` 验证控制。
20. 在 Web Dashboard 点击 `Home`、`Away`、`Night`，或向 BLE Command 特征写入 `home`、`away`、`night`，确认页面、安防和灯光状态按宏场景切换。
21. 在 Web Dashboard 点击 `XiaoZhi`，或向 BLE Command 特征写入 `xiaozhi`，确认屏幕跳到 XIAOZHI 页，串口出现 `[XIAOZHI] wake` / `[XIAOZHI] reply`；未接扬声器时只看串口和页面状态即可。
22. 配置 PushPlus token 后，在无人安防状态下制造较大声音，确认本地报警且串口出现 `[PushPlus] HTTP code`，手机收到远程预警。

## 可调参数

常用参数集中在 `src/board/config.h`：

| 参数 | 默认值 | 说明 |
|---|---|---|
| `BUTTON_ACTIVE_LEVEL` | `HIGH` | 按键触发电平 |
| `PRESENCE_ACTIVE_LEVEL` | `HIGH` | LD2410C OUT 有人触发电平 |
| `IR_RX_ACTIVE_LEVEL` | `LOW` | 红外接收触发电平 |
| `IR_RX_HOLD_MS` | `1000` | IR RX latched 屏幕保持时间 |
| `MIC_RMS_TRIGGER` | `65000` | I2S 麦克风报警阈值 |
| `MIC_RMS_ADAPT_MARGIN` | `35000` | 自适应麦克风阈值安全余量 |
| `MIC_RMS_ADAPT_MULTIPLIER_PERCENT` | `240` | 自适应阈值相对环境基线的倍率 |
| `MIC_SMOOTH_WEIGHT_PERCENT` | `20` | 麦克风平滑时新样本权重，越大响应越快但越容易抖 |
| `SPEAKER_TONE_HZ` | `880` | 小智提示音频率 |
| `SPEAKER_TONE_MS` | `220` | 小智提示音时长 |
| `XIAOZHI_AUTO_WAKE_COOLDOWN_MS` | `8000` | 麦克风自动触发小智的冷却时间 |
| `XIAOZHI_LISTEN_MS` / `XIAOZHI_THINK_MS` / `XIAOZHI_SPEAK_MS` | `800` / `600` / `900` | 小智演示状态机每段保持时间 |
| `TEMP_COOLING_THRESHOLD_C` | `28.0` | 空调模拟触发温度 |
| `EMPTY_TO_SECURITY_MS` | `10000` | 无人进入安防的等待时间 |
| `ALARM_HOLD_MS` | `5000` | 最后一次异常声音后报警保持时间，安静超过该时间自动恢复 |
| `AC_COMMAND_GAP_MS` | `15000` | 红外空调指令发送间隔 |
| `IR_TEST_WINDOW_MS` | `5000` | KEY3 红外诊断持续时间 |
| `IR_TEST_BURST_GAP_MS` | `120` | 红外诊断脉冲间隔 |
| `PHONE_ALERT_COOLDOWN_MS` | `60000` | PushPlus 远程预警最小间隔 |

## 当前限制

- 红外发射目前只是约 78ms 的 38 kHz 演示脉冲，不是真实空调协议；按 KEY3 会开启 5 秒诊断发射，方便用手机摄像头或红外接收头验证。
- I2S 麦克风显示的是相对声音强度，不是标准 dB；代码会自动选择有效 I2S 槽位、去直流偏置并做平滑，但比赛现场仍建议观察 `[MIC]` 串口日志中的 `rms` / `smooth` / `base` / `thr` / `pct` 后微调参数。
- AHT20 未接入时，代码会使用模拟温湿度，方便先调屏幕和联动。
- 屏幕使用局部刷新，减少 ILI9341 全屏重绘闪烁。
- 时间、天气、Web Dashboard 和 PushPlus 依赖 Wi-Fi；未配置 `src/net/secrets.h` 时这些联网功能会跳过或显示等待状态。
- IP 定位使用城市级公网 IP 信息，精度到城市，不适合做室内或街道级定位；天气备用经纬度在 `secrets_example.h` 中，可按实际城市修改。
- 天气数据来自 Open-Meteo HTTP 接口，日出日落使用接口返回的当地时区时间。
- LD2410C 当前使用 OUT 引脚判断有人/无人，不读取 UART 距离、移动目标或静止目标数据。
- 小智 AI 目前是可运行的本地演示链路：麦克风阈值或 Web/BLE 命令触发，生成家居状态回复，并通过 MAX98357A 输出提示音；还不是真实唤醒词、ASR、云端大模型语义对话或 TTS 流式播放。

## 贡献

欢迎提交 issue 或 pull request。提交代码前请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 和 [docs/branching.md](docs/branching.md)。

建议在提交前先运行：

```bash
pio run
```

请不要提交本地生成文件，例如 `.pio/`、`.cache/`、`compile_commands.json` 和个人 IDE 配置。

## 许可证

当前仓库尚未指定开源许可证。正式公开发布前建议补充 `LICENSE` 文件。
