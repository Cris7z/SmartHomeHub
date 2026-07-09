# 分支模型

本项目采用适合硬件原型和比赛演示的小型协作流程。目标是让 `main` 始终保持可编译、可演示，同时给高频硬件联调、Web/AI 功能和文档修改留出独立分支。

## 长期分支

| 分支 | 用途 |
|---|---|
| `main` | 最新集成版本。合入前至少完成编译，重要演示节点用 tag 固定。 |

当前阶段不强制维护长期 `develop`、`release/*` 或个人长期分支。等硬件和演示流程稳定后，再按需要引入更重的发布流程。

## 短期分支命名

硬件联调：

```bash
git switch main
git pull --ff-only origin main
git switch -c bringup/i2s-mic-threshold
```

功能开发：

```bash
git switch main
git pull --ff-only origin main
git switch -c feature/web-ai-command
```

Bug 修复：

```bash
git switch main
git pull --ff-only origin main
git switch -c bugfix/alarm-reset
```

文档修改：

```bash
git switch main
git pull --ff-only origin main
git switch -c docs/public-readme
```

## 合并规则

推荐路径：

```text
bringup/* -> main
feature/* -> main
bugfix/* -> main
docs/* -> main
```

合并前至少确认：

- `pio run` 通过。
- 涉及 relay 的改动通过 `python -m unittest discover -s tools\doubao_relay -t . -p "test_*.py"`。
- 涉及实物行为的改动说明已测试的硬件、串口现象和剩余风险。
- 没有提交 `.pio/`、`.vscode` 个人配置、`.env`、`src/net/secrets.h`、日志或本地绝对路径。

## 版本标签

重要演示节点用 tag 固定，方便后续继续开发后仍能回到稳定版本。

```bash
git tag v1.0-demo
git push origin v1.0-demo
```

推荐标签命名：

- `v0.1-screen-ok`
- `v0.2-sensor-ok`
- `v0.3-alarm-ok`
- `v1.0-demo`
- `v1.1-competition-demo`

## 协作规则

- `main` 保持可编译，不提交明显无法构建的中间状态。
- 短分支按任务或模块命名，避免长期个人分支。
- 修改公共引脚、硬件接线、`platformio.ini`、语音 relay 协议或密钥配置方式前，应说明影响范围。
- 不在共享分支上使用 `git push --force`。
- 不把 token、Wi-Fi 密码、API Key、本地串口号或本地绝对路径写进公共提交。
