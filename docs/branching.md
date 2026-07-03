# 分支模型

本项目采用适合嵌入式/硬件联调的小型 Git Flow。目标是让 `main` 始终保持可演示，日常开发在 `develop` 或短分支中完成，硬件联调和比赛冻结版本有明确位置。

## 长期分支

| 分支 | 用途 | 合并方向 |
|---|---|---|
| `main` | 稳定演示分支，只放已经验证过的版本 | 只从 `release/*` 或紧急 `hotfix/*` 合并 |
| `develop` | 日常集成分支，功能和 bugfix 先合入这里 | 从 `feature/*`、`bugfix/*`、`bringup/*` 合并 |
| `bringup/hardware-validation` | 硬件接线、传感器阈值、现场联调分支 | 验证稳定后合入 `develop` |
| `release/demo-v0.1` | 当前演示基线冻结分支，用于随时回到稳定 Demo | 只接受演示前必要修复 |

## 短期分支命名

功能开发：

```bash
git switch develop
git pull
git switch -c feature/tft-ui-refresh
```

Bug 修复：

```bash
git switch develop
git pull
git switch -c bugfix/alarm-reset
```

硬件联调：

```bash
git switch bringup/hardware-validation
git pull
git switch -c bringup/i2s-mic-threshold
```

紧急修复稳定演示版：

```bash
git switch main
git pull
git switch -c hotfix/demo-upload-failure
```

## 推荐合并路径

常规功能：

```text
feature/* -> develop -> release/* -> main
```

Bug 修复：

```text
bugfix/* -> develop
```

硬件联调：

```text
bringup/* -> develop
```

演示前冻结：

```text
develop -> release/demo-vX.Y -> main
```

线上/比赛紧急修复：

```text
hotfix/* -> main -> develop
```

## 版本标签

重要节点用 tag 固定：

```bash
git tag v0.1-demo-baseline
git push origin v0.1-demo-baseline
```

推荐标签：

- `v0.1-demo-baseline`：当前可编译、可演示基线。
- `v0.2-hardware-ok`：主要硬件模块验证通过。
- `v1.0-competition-demo`：比赛最终演示版本。

## 协作规则

- `main` 不做随手开发，保持稳定。
- `develop` 是默认集成分支，日常功能从这里切出去。
- `feature/*`、`bugfix/*`、`bringup/*` 分支尽量短生命周期，用完合并或删除。
- 修改公共引脚、硬件接线或 `platformio.ini` 前，应先说明影响范围。
- 合并到 `develop` 或 `main` 前至少运行一次 `pio run`。
- 不使用 `git push --force` 改写共享分支历史。
