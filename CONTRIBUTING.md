# 贡献与版本管理规范

本文档用于说明 SmartHomeHub 后续修改、修 bug、提交版本和找回历史版本时的基本要求。

分支模型见 [docs/branching.md](docs/branching.md)。当前仓库采用轻量协作流程：`main` 保持可编译、可演示，日常修改从最新 `main` 切短分支，稳定演示节点用 tag 固定。

## 基本流程

开始修改前，先切到 `main` 并拉取最新代码：

```bash
git switch main
git pull --ff-only origin main
```

较大的功能修改或不确定的实验，建议新建分支：

```bash
git switch -c feature/short-description
```

如果只是小范围文档修改，可以在维护者确认后直接提交到 `main`。代码、硬件接线、协议或脚本修改建议使用短分支或 pull request。

## 提交前检查

代码修改提交前，先运行编译：

```bash
pio run
```

检查准备提交的内容：

```bash
git status
git diff
```

不要提交本地生成文件，包括：

- `.pio/`
- `.cache/`
- `compile_commands.json`
- `.vscode/c_cpp_properties.json`
- `.vscode/launch.json`
- 个人串口配置
- 本地日志或临时文件

## 提交要求

每个 commit 尽量只表达一个明确改动。提交信息要能看出这次改了什么：

```bash
git add src/main.cpp README.md
git commit -m "Fix alarm reset behavior"
git push
```

推荐的提交信息：

- `Update README wiring table`
- `Fix I2S microphone threshold handling`
- `Add infrared receive status display`
- `Tune security mode timeout`

避免使用过于模糊的信息：

- `update`
- `fix`
- `changes`
- `final`

## Bug 修复流程

修 bug 前先记录清楚问题：

1. 现象是什么？
2. 如何复现？
3. 当前接了哪些硬件模块？
4. 串口监视器输出了什么？

修复后：

1. 运行 `pio run`。
2. 条件允许时测试对应硬件行为。
3. 用清晰的提交信息提交修复。
4. 推送分支或发起 pull request。

## 历史版本找回

只要修改已经 commit 并 push，就可以通过 Git 找回历史版本。

查看提交历史：

```bash
git log --oneline
```

恢复某个文件到旧版本：

```bash
git checkout <commit-id> -- src/main.cpp
```

从旧版本新建一个分支：

```bash
git switch -c old-demo-version <commit-id>
```

撤销某次错误提交，且不破坏共享历史：

```bash
git revert <commit-id>
```

## 稳定版本标签

比赛演示、阶段验收或硬件联调成功后，建议打 tag 保存稳定版本：

```bash
git tag v1.0-demo
git push origin v1.0-demo
```

推荐标签命名：

- `v0.1-screen-ok`
- `v0.2-sensor-ok`
- `v0.3-alarm-ok`
- `v1.0-demo`

tag 可以帮助项目在后续继续修改后，仍然快速回到某个稳定演示版本。

## 安全规则

- 不要在共享分支上使用 `git push --force`。
- 不要随意重写已经 push 的历史，除非所有协作者都确认。
- 不要提交 token、密码、Wi-Fi 密码、局域网地址或本地绝对路径。
- 不要为了自己的串口号修改公共 `platformio.ini`。
- 如果某个分支暂时编译不过，需要明确标注为实验分支，不要合并到主线。

## 协作者本地配置

协作者 clone 仓库后可以直接编译代码，但不能复用其他开发环境中的私有配置。以下文件必须每个人在本地单独创建，不能提交到 Git：

- `src/net/secrets.h`
- `tools/doubao_relay/.env`

本地配置步骤：

1. 复制 `src/net/secrets_example.h` 为 `src/net/secrets.h`。
2. 填自己的 Wi-Fi、天气备用坐标，以及 relay 主机地址：

```cpp
#define SMART_HOME_DOUBAO_RELAY_URL "ws://<relay-host-ip>:8765/voice"
```

3. 复制 `tools/doubao_relay/.env.example` 为 `tools/doubao_relay/.env`。
4. 在 `.env` 里填写自己的豆包语音 API Key。
5. 首次使用先运行 `tools\doubao_relay\setup.ps1`。
6. 执行 `tools\doubao_relay\run.ps1 --probe`，确认云端鉴权通过。
7. 每次演示前运行 `tools\doubao_relay\run.ps1`，让开发主机作为 ESP32-S3 和豆包云端之间的语音 relay。

协作注意事项：

- 不要把 API Key、Wi-Fi 密码或 relay 私有 IP 写进公共文档、代码、截图或提交记录。
- `platformio.ini` 保持公共配置，不要为了个人串口号写死 `upload_port`。
- 如果修改了硬件引脚、阈值、语音参数或 relay 协议，提交信息里要写清楚影响范围。
- 提交前至少运行 `pio run`；涉及 relay 的改动还要运行 `python -m unittest discover -s tools\doubao_relay -t . -p "test_*.py"`。

## Pull Request 检查清单

请求合并前确认：

- `pio run` 通过。
- 改动目的已经说明。
- 如果涉及硬件行为，说明已测试的模块和现象。
- 没有提交生成文件。
- commit 信息清晰，改动范围集中。
