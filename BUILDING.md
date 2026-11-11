# Building / 建置

## English

Requirements: Windows x64, Visual Studio 2022 C++ tools, xmake 3.x and Python 3.
The JSON header is vendored; xmake obtains MinHook 1.3.4 on first configuration.
See THIRD_PARTY_NOTICES.txt for dependency licenses.

```powershell
git clone https://github.com/XuanDiNa/Sprint-Toggle.git
cd Sprint-Toggle
py -m pip install -r tools/requirements.txt
xmake f -p windows -a x64 -m release -y
xmake build -y
```

The DLL is written to `build/windows/x64/release/ToggleSprint.dll`.
Run the standalone Debug/Release tests and artifact checks with:

```powershell
./tools/verify_release.ps1
```

The default run has seven C++ suites and Python schema/artifact checks. Profile
fixtures run without Minecraft installed. To additionally verify both exact
game images, pass `-OriginalImage` and `-LauncherImage` with your local paths.
These images are read only and are not distributed here.

For the optional eighth suite, clone ModMenuBedrock separately and pass its
checkout path using `-ModMenuSource`. This release's settings integration was
verified with ModMenuBedrock source commit
`861e98a52f8bbc480c2dc01281401cb94153b8d9`.
This option only adds tests; the plugin does not link against ModMenu.

The public source includes the exact runtime and its generated native profile.
The profile metadata is in `profile/native-evidence.json`; private analysis
databases, game binaries and decompilation exports are not included. Do not
change version guards to target another executable.

`-FrozenDll` additionally smoke-tests a previously frozen DLL using an isolated
temporary copy. `-Package -FrozenDll <path>` only packages the exact test7 DLL
whose SHA-256 is recorded in the packaging script. A local rebuild can have a
different hash; it must not silently replace the release DLL. The download
assets retain the original matched-bundle binary. Offline tests do not establish
gameplay acceptance.

## 繁體中文

需要 Windows x64、Visual Studio 2022 C++ 工具、xmake 3.x 與 Python 3。
JSON 標頭已隨原始碼提供，首次設定時由 xmake 取得 MinHook 1.3.4，授權見
THIRD_PARTY_NOTICES.txt。依上方指令建置，DLL 位於
`build/windows/x64/release/ToggleSprint.dll`。

執行 `./tools/verify_release.ps1` 可完成 Debug／Release 的七個 C++ 套件及
Python schema／套件檢查。不需要安裝遊戲即可跑 Profile 測試資料；如需額外
核對實際映像，使用 `-OriginalImage` 與 `-LauncherImage` 傳入本機路徑。
檢查只讀取遊戲檔案，倉庫不提供這些檔案。

第八個 ModMenu 儲存整合套件為選用：另外複製 ModMenuBedrock，以
`-ModMenuSource` 指定其原始碼目錄。本次使用上方記錄的 commit 驗證。
此選項只增加測試，插件不連結 ModMenu。

公開原始碼包含原有執行程式與生成的原生版本資料，結構化資料位於
`profile/native-evidence.json`；不包含私人分析資料庫、遊戲檔案或反編譯匯出。
請勿修改版本檢查來套用其他遊戲映像。

`-FrozenDll` 會透過隔離的臨時副本測試已凍結 DLL；
`-Package -FrozenDll <路徑>` 只接受打包腳本中固定雜湊的 test7 DLL。
本機重編譯可能得到不同雜湊，不可直接取代發佈檔；下載套件保留原有配套測試包的
相同 DLL。離線通過不等於遊戲內已驗收。
