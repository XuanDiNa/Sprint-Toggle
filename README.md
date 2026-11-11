# Toggle Sprint

Client-side toggle and force sprint for Minecraft Bedrock and LeviLauncher.

**Windows x64 · Minecraft 1.26.52.03 · 0.1.0-test7-leftalt — Prerelease**

[Downloads](https://github.com/XuanDiNa/Sprint-Toggle/releases/tag/v0.1.0-test7-leftalt) · [Build from source](BUILDING.md) · [ModMenu](https://github.com/XuanDiNa/ModMenuBedrock)

## Features

- **Toggle Sprint** (default): press left Ctrl once to latch sprint, then press it again to stop. Holding the key does not repeat the toggle.
- **Force Sprint**: request sprint while moving forward, without a toggle key.
- **Off**: use normal Minecraft controls.
- Custom shortcuts with Ctrl, Shift and Left Alt combinations.
- JSON settings and optional ModMenu configuration with hot reload.

Inventory, chat, pause/settings screens and temporary focus loss suspend input
while retaining the latch. Return to gameplay and move forward to resume.
Keys pressed while a screen owns input do not toggle sprint. Death, player/world
changes, Off and changed settings clear the latch; restarts do not retain it.

Vanilla restrictions such as hunger, sneaking, collisions and item use still
apply. No server plugin or LeviLamina dependency is required.

## Installation

Fully exit Minecraft and back up the existing plugin folder outside the loading
directory. Enable one copy of `ToggleSprint.dll` in LeviLauncher.

- **Updating:** merge the `ToggleSprint` folder from `-update.zip` into your existing folder. It includes the DLL, manifests and setting descriptions **without config.json**, preserving your settings.
- **First installation:** use `-full.zip`, which includes default settings. Do not overwrite customized settings with this default when updating.

DLL replacement requires a restart; settings changes do not. Verify the included
checksums and follow [TESTING.md](TESTING.md) for the short gameplay check.

This is the native `ToggleSprint.dll` implementation. The historical `SprintMod/`
folder and older releases are retained; do not load `SprintMod.dll` alongside it.

## Configuration

```json
{
  "mode": 1,
  "toggleKey": "LCTRL"
}
```

`mode` is `0` Off, `1` Toggle Sprint or `2` Force Sprint, with no separate
`enabled` switch. `toggleKey` affects Toggle mode only; `NONE` is unbound.

With **ModMenuBedrock 0.6.0-test5-leftalt / settings v5**, open
**Mods → Toggle Sprint → Configure**. Apply saves and reloads within about
500 ms; Cancel discards the draft. Labels follow ModMenu's game language.
ModMenu is optional; you can edit the JSON directly.

Shortcuts include `F6`, `CTRL+K`, `SHIFT+K`, `LALT+K` and
`CTRL+SHIFT+LALT+K`. Hold the modifiers before pressing the primary key.
The Ctrl/Shift/Left Alt mask must match exactly; Right Alt does not replace
Left Alt. Late modifiers and preheld keys after a screen, focus or settings
change cannot create a new toggle. Release the key and press it again.

During capture, **Escape selects Unbound**; **Backspace is a bindable key**.
Apply is required to save. Standalone left/right modifier keys remain available.
Escape and mouse buttons cannot be bound. Existing single-key settings and
supported legacy spellings remain readable without rewriting the file;
the schema describes canonical tokens. Invalid settings retain the last valid state.

Minecraft and other plugins keep their own bindings. Avoid overlapping shortcuts
when testing; ModMenu does not resolve conflicts. The default left Ctrl may also
activate alongside another plugin's Ctrl combination.

## Testing status

This follow-up keeps the exact test7 DLL from the matched seven-plugin bundle;
it updates standalone descriptions, documentation, verification and packaging.
The native hooks and exact-version checks are unchanged. Other game versions
remain inactive.

Offline checks cover configuration, physical-binding/policy composition, native
input forwarding, image guards, ModMenu transactions and package integrity.
They do not establish gameplay acceptance. The latest combination-key and
UI-retention behavior still needs the short check in TESTING.md.
`READY` in the log confirms startup only.

---

## 繁體中文

適用於 Minecraft 基岩版與 LeviLauncher 的客戶端切換疾跑插件。

**Windows x64 · Minecraft 1.26.52.03 · 0.1.0-test7-leftalt 測試版**

[下載](https://github.com/XuanDiNa/Sprint-Toggle/releases/tag/v0.1.0-test7-leftalt) · [原始碼建置](BUILDING.md) · [ModMenu](https://github.com/XuanDiNa/ModMenuBedrock)

### 功能

- **切換疾跑**（預設）：按一次左 Ctrl 鎖定，再按一次停止；長按不重複切換。
- **強制疾跑**：向前移動時持續請求疾跑，不需要切換鍵。
- **關閉**：使用原版控制。
- 支援 Ctrl、Shift、左 Alt 組合鍵。
- JSON 設定與可選用的 ModMenu 設定頁，支援即時重讀。

背包、聊天、暫停／設定介面及暫時失焦只暫停輸入，保留疾跑鎖定；返回遊戲後
向前移動即可恢復。介面內按鍵不切換疾跑。死亡、玩家／世界變更、關閉模式與
修改設定會清除鎖定；重啟遊戲不保留鎖定。

仍遵守飢餓、潛行、碰撞、使用物品等原版限制，不需要伺服器插件或 LeviLamina。

### 安裝

完全退出 Minecraft，先將原有插件備份到載入目錄外，在 LeviLauncher 中只啟用
一份 `ToggleSprint.dll`。

- **更新**：合併 `-update.zip` 內的 `ToggleSprint` 資料夾。內含 DLL、manifest 與設定描述檔，**不含 config.json**，保留原有設定。
- **首次安裝**：使用 `-full.zip`，內含預設設定；更新時不要用預設檔覆蓋自訂設定。

替換 DLL 需要重啟遊戲，修改設定不需要。核對套件內的雜湊，測試步驟見
[TESTING.md](TESTING.md)。

目前版本使用原生 `ToggleSprint.dll`；倉庫內保留歷史 `SprintMod/` 與舊版發佈，
請勿同時載入 `SprintMod.dll`。

### 設定

上方 JSON 的 `mode` 為 `0` 關閉、`1` 切換、`2` 強制，沒有額外的 `enabled`。
`toggleKey` 預設 `LCTRL`，只用於切換模式；`NONE` 為無綁定。

使用 **ModMenuBedrock 0.6.0-test5-leftalt／設定 v5**，進入
**Mods → Toggle Sprint → Configure**。Apply 儲存後約 500 ms 生效，Cancel
捨棄草稿，文字依 ModMenu 的遊戲語言顯示。沒有 ModMenu 也可直接修改 JSON。

支援 `F6`、`CTRL+K`、`SHIFT+K`、`LALT+K`、`CTRL+SHIFT+LALT+K` 等。
先按住修飾鍵，再按主鍵；Ctrl／Shift／左 Alt 必須精確符合，右 Alt 不代替左 Alt。
晚加修飾鍵、跨介面／失焦／設定變更一直按住主鍵，都不應產生新的切換；需放開再按。

綁鍵時 **Escape 設為無綁定**，**Backspace 可正常綁定**，按 Apply 才儲存。
仍可單獨綁定左右修飾鍵；Esc 與滑鼠按鍵不可綁定。原有單鍵設定與支援的舊寫法
可直接讀取，DLL 不改寫設定檔；schema 描述標準寫法。無效設定保留上次有效值。

遊戲和其他插件各自保留按鍵，不會由 ModMenu 統一解決衝突。測試時請避開重複綁定，
尤其預設左 Ctrl 可能與其他插件的 Ctrl 組合鍵同時觸發。

### 測試狀態

本次沿用七插件配套測試包內完全相同的 test7 DLL，補齊獨立描述、文件、驗證與打包。
原生掛鉤與精確版本檢查維持原樣，其他遊戲版本安全停用。

離線檢查涵蓋設定、實體按鍵與策略組合、原生輸入轉交、遊戲映像檢查、ModMenu
儲存交易及套件完整性，不代表遊戲內已驗收。新版組合鍵與介面保留鎖定仍待
TESTING.md 中的簡短人工測試；日誌 `READY` 只代表啟動成功。
