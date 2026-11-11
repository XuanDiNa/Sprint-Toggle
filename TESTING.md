# Toggle Sprint testing

**0.1.0-test7-leftalt · Windows x64 · Minecraft 1.26.52.03**

## English

Exit Minecraft, back up the existing ToggleSprint folder outside the loading
directory, then merge `-update.zip`. Keep `config.json` and logs.
Use the full package only for first installation. Keep the other six plugins
unchanged. Settings v5 needs **ModMenu 0.6.0-test5-leftalt**.

The DLL is identical to the matched seven-plugin test package. If you already
tested these exact bindings with that DLL, no duplicate campaign is required.
Offline tests do not exercise OS key delivery or Minecraft gameplay.

1. Check the latest log starts with `START 0.1.0-test7-leftalt` and reaches `READY`. In a clear survival world with enough food, confirm your original settings were retained.
2. Configure Toggle with an unused shortcut such as `CTRL+F6`. Test Cancel, then Apply. Hold Ctrl before F6: one press starts the latch, a second fresh press stops it; holding F6 must not repeat. F6 alone or Ctrl+Shift+F6 must not trigger. F6 first, then Ctrl must not trigger.
3. Test `LALT+F6` and `CTRL+SHIFT+LALT+F6` similarly. Right Alt must not replace Left Alt. Avoid shortcuts used by other plugins or Minecraft; the menu does not resolve conflicts.
4. Latch sprint, open inventory/chat and pause/settings for at least 3 seconds, then return and move forward: sprint should resume. Hold the shortcut across closing a screen and briefly switch focus: no phantom toggle. Release and press again to stop. Applying changed settings clears the latch; a preheld key must not relatch.
5. During capture, Esc selects Unbound without leaving the page. Apply and confirm `NONE` persists; Backspace should instead save `BACKSPACE`. Restore your preferred binding. Reopen Configure or restart to confirm persistence.
6. Recheck Force without a shortcut and Off with ordinary controls. Move/jump forward for 15 seconds and watch for corrections, stuck sprint, crashes or delay. Check single-player and your usual server separately if both are used.

`READY` and `INPUT` counts confirm route activity only. Preserve the log;
report the failed step, binding, mode and single-player/server context.
To roll back, exit Minecraft and restore the old ToggleSprint folder and settings.

## 繁體中文

完全退出遊戲，先將原有 ToggleSprint 備份到載入目錄外，再合併 `-update.zip`。
保留 `config.json` 與日誌，首次安裝才使用完整包。另外六個插件維持原樣。
設定 v5 需使用 **ModMenu 0.6.0-test5-leftalt**。

DLL 與七插件配套測試包完全相同；若已用該 DLL 測過以下相同操作，不必重複整輪。
離線測試沒有驗證作業系統按鍵傳遞或 Minecraft 遊戲內行為。

1. 最新日誌應有 `START 0.1.0-test7-leftalt` 與 `READY`。在路徑空曠、飽食度足夠的生存世界，確認原有設定保留。
2. Configure 選切換模式，綁未使用的組合，例如 `CTRL+F6`，先試 Cancel，再 Apply。先按 Ctrl 再按 F6，一次鎖定、第二次解除，長按不重複。單按 F6 或 Ctrl+Shift+F6 不觸發；先按 F6 再補 Ctrl 也不觸發。
3. 同樣測 `LALT+F6`、`CTRL+SHIFT+LALT+F6`。右 Alt 不代替左 Alt。避開其他插件與遊戲已用的快捷鍵，ModMenu 不處理衝突。
4. 鎖定後開背包／聊天、暫停／設定至少 3 秒，返回向前應恢復。跨介面按住快捷鍵及暫時失焦不得誤切換；放開後在遊戲內再按才解除。套用有變動的設定會清除鎖定，一直按住不得重新鎖定。
5. 綁鍵時 Esc 應設無綁定而不離開頁面，Apply 後保存 `NONE`；Backspace 應保存為 `BACKSPACE`。再恢復偏好設定，重開 Configure 或重啟確認保留。
6. 強制模式不用快捷鍵即可前進疾跑，關閉模式使用原版控制。向前連跳 15 秒，留意拉回、卡住疾跑、閃退或延遲；有用單人與伺服器時分別驗證。

`READY` 與 `INPUT` 計數只證明路徑執行。保留日誌，回報失敗步驟、綁定、模式及
單人／伺服器即可。回復時退出遊戲，還原原有 ToggleSprint 資料夾與設定。
