# AutoSprint

A Minecraft Bedrock Edition mod that provides auto-sprint functionality similar to Java Edition.

按一下 Ctrl 开启自动疾跑模式，按住 W 自动疾跑，再按 Ctrl 关闭。

---

## 功能 | Features

- 按 **Ctrl** 切换自动疾跑模式
- 开启后，按住 **W** 自动按住疾跑键（Ctrl）
- 再按 **Ctrl** 关闭自动疾跑

---

## 安装 | Installation

### 1. 下载 DLL

从 [Releases](https://github.com/yourusername/AutoSprint/releases) 下载最新的 `.dll` 文件。

### 2. 安装

**如果使用 LeviLauncher 或 mc-w10-version-launcher:**

1. 打开游戏文件夹
2. 找到 `mods` 文件夹
3. 将 `.dll` 文件放入 mods 文件夹

**如果使用其他注入器:**

将 DLL 注入到 Minecraft Windows Edition

---

## 使用 | Usage

1. 进入游戏
2. 按 **Ctrl** 开启自动疾跑模式（会有提示）
3. 按住 **W** 移动时自动疾跑
4. 再次按 **Ctrl** 关闭自动疾跑模式

---

## 构建 | Build

```bash
# 安装 xmake
# 克隆项目
git clone https://github.com/yourusername/AutoSprint.git
cd AutoSprint

# 构建
xmake build
```

---

## 技术细节 | Technical Details

- 使用 `GetAsyncKeyState` 检测按键
- 使用 `keybd_event` 模拟按键
- 需要 User32.lib

---

## Star 历史

[![Star History Chart](https://api.star-history.com/svg?repos=yourusername/AutoSprint&type=Date)](https://star-history.com/#yourusername/AutoSprint&Date)

---

## 许可证 | License

MIT License
