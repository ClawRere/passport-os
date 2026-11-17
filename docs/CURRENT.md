## 当前待办
2026-11-17 六处修复并烧录（commit b028dfe）：①首页长按OK→小智对话；②语音备忘改用 capture init；③红外扩展6信号 UP/DOWN切换 OK发送；④清单/备忘从抽屉移除（源码留 stub 供 xiaozhi/device_control/netease/power 引用）；⑤CI 全绿（run 43912132499 PASS）；⑥app 分区仅更新 0x10000，NVS WiFi 凭证、姓名照片 Muse 配置保留。固件 full.bin 3,314,560 B，SHA256 3c654567；读回前 3,314,560 字节与烧录文件完全一致；设备恢复 COM4 ESP32-C3 v1.1。

2026-11-17 Launcher 还原 + shutdown 工具（commit d493fd3）：恢复 Apple Watch 风格卡片轮播布局（s_slots/s_icons 移除 REMINDERS/LISTS）；新增 self.system.shutdown MCP 工具。CI commit 36b05e5 PASS。固件 v3 SHA256 0d5b99c1；COM4 ESP32-C3 v1.1 启动正常，9 apps 注册成功，Boot Complete。本次全片烧录至 0x00000，NVS 被覆盖导致 WiFi 凭证需重新配置（下次更新请用 app-only 写 0x10000）。


2026-11-15 移除冗余应用并增强首页交互：删除 game(你在狗叫什么)、sound_jump(声音跳一跳)、voice(随身AI)、muse(Muse) 四个应用的构建与图标，清理 app_registry.cc / launcher.cc / device_control_mcp.cc / main.cc 中的孤儿引用；保留 tetris/游戏目录（俄罗斯方块仍使用）。新增 BTN_EVT_TRIPLE 按键事件与 OkGesture 三击检测（kDoubleUs=180ms, kTripleGapUs=280ms）；长按 OK 在首页打开小智对话、三击 OK 将 BTN_EVT_TRIPLE 分发给前台应用（网易云等已实现 toggle 逻辑的应用可直接响应暂停/继续）。WiFi 凭证通过 NVS 持久化，每次固件烧录不重置。2026-11-16 Launcher UI 改版：卡片轮播替换为 Apple Watch 风格 3×4 圆角网格（每格62×52px），选中态带配色边框高亮；首次进入每个应用有淡入动效；上下方向键环绕导航，OK打开。修复 lvgl 兼容问题（`lv_obj_set_scale` 不可用，改用背景透明度动画；多 flag 清除改为分两次调用）。IR应用与语音备忘录新增：ir component（ESP32-C3 GPIO11 NEC TX）、voicememo app（SPIFFS PCM 录音/回放）；三击OK分发给前台应用（网易云/电台 toggle 暂停），长按OK首页进小智对话；WiFi NVS持久化不重置。修复时间同步依赖（恢复 nvs_settings.c + time_sync.c 构建）及 IR 组件 C++ 链接问题（ir_nec.h extern C）。完整主机回归 + CI 构建全部通过（run 43884414402 @ 247eff7，PASS）。2026-11-16 23:09 COM4 全片烧录 FoloToy-AI-Passport-full.bin（3,370,080 B，写 9.4s，hash 校验通过；读回 app 分区 4 MB 确认一致性）；堆检查 heap_ok=1，启动完成，进入应用网格界面。


2026-11-14 基于远程 main 整理：保留全部既有源码改动，恢复维护副本缺失的 29 项公开说明、社区模板和 rc.56 历史验收记录；中英文首页沿用原项目介绍，更新 13 项功能，按用户要求将首页的源码版本段落替换为最新固件页面链接，并在功能表前突出小智作为设备语音操作入口，列出硬件设置、媒体、番茄钟、提醒、清单和应用打开的指令场景及切换边界。完整静态检查（含全部工作流 actionlint 和主机回归）、589 文件源码打包 dry-run、首页本地链接检查通过；复核前述 2026-11-14 发布配置构建产物的布局、SRAM 和公开默认值通过。这轮未改运行源码、未重新构建或烧录。索引保持远程基线，完整拟提交差异与审阅清单位于忽略的 `output/20261114-rc59-review/`；本轮审阅已获用户同意，允许创建提交并推送源码。

2026-11-14 上传准备：补齐小智 WebSocket PING 的主机测试替身，覆盖连接/断线、PING 失败仍释放、退出顺序及重复退出；CI 与校验器用 bash 调用脚本，避免 Windows 初次提交缺执行位；`sdkconfig.diag` 不再被忽略，源码包可包含诊断配置；README 标注 rc.59，语音音量工具说明改为静音不影响固定音量提醒。完整静态检查（含 actionlint 与主机回归）、源码打包 dry-run、发布配置构建、布局与 SRAM 检查通过（应用 4,069,216 B，首块堆 78,760 B）。本轮未重新烧录，未上传最新源码或发布固件；日志与修改前备份在忽略的 `output/20261114-upload-prep/`。

rc.59 实机重试用户已反馈未见问题；其余此前列出的 rc.58 手机页面/全新安装检查与长期使用验收仍按实际结果记录，通过后再决定社区发布。

取得电流计型号/接法/采集能力后，用同一源码仅切换三项省电策略制作 A/B 对照，测首页亮屏、联网熄屏、小智空闲熄屏、离线扫描。记录电压、平均/峰值电流、能量和时间，排除电脑 USB 对轻睡眠的影响。用户未提出新需求时，不主动扩展范围。

仅需要追溯证据时查旧目录 `docs/power-acceptance-20261108.md` 和旁边 `_diag/power-acceptance-20261108/`；不要日常加载所有历史记录。


2026-11-17 Launcher 微调（commits 12f3c2e + d1be0af）：在 d493fd3 Apple Watch 网格基础上做三处调整——屏幕背景改为纯白（0xFFFFFF），移除 UP/DOWN 切换时的图标弹跳动画，应用名标签改用 WRAP 模式完整显示中文长名称。未改动导航逻辑和布局结构。CI commits 12f3c2e/d1be0af PASS，固件 v6 SHA256 bfa93b73。COM4 ESP32-C3 v1.1 烧录完成、Boot Complete。NVS 再次被覆盖，WiFi 凭证需重新配网。
