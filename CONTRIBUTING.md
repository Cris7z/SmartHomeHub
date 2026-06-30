# 贡献与版本管理规范

本文档用于说明 SmartHomeHub 后续修改、修 bug、提交版本和找回历史版本时的基本要求。

## 基本流程

开始修改前，先拉取最新代码：

```bash
git pull
```

较大的功能修改或不确定的实验，建议新建分支：

```bash
git switch -c feature/short-description
```

如果只是小范围文档修改，可以在维护者确认后直接提交到 `main`。代码修改建议使用分支或 pull request。

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
- 不要提交 token、密码、Wi-Fi 密码或私人路径。
- 不要为了自己的串口号修改公共 `platformio.ini`。
- 如果某个分支暂时编译不过，需要明确标注为实验分支，不要合并到主线。

## Pull Request 检查清单

请求合并前确认：

- `pio run` 通过。
- 改动目的已经说明。
- 如果涉及硬件行为，说明已测试的模块和现象。
- 没有提交生成文件。
- commit 信息清晰，改动范围集中。
