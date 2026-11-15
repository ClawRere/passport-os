# AI 维护入口

这是 Passport OS 的当前维护副本，整理于 2026-11-08。默认只读本文件与 `docs/CURRENT.md`，再按任务定位源码；需要构建/烧录时才读 `docs/MAINTENANCE.md`。

- 用户认可当前版本，后续以小范围修正为主。不要主动扩功能、重构全项目或重新讨论历史方案。
- 当前目录是后续修改位置；旧目录 `F:/WORK/AI硬件/passport-os` 保留为历史档案，不要两边同步修改。
- 不要默认遍历或读取 `firmware/`、`managed_components/`、`build*/`、生成字库、大型测试 fixture 或旧目录的 docs、_diag、release。使用 rg 限定相关模块。
- `main/`、`components/`、`bootloader_components/` 是完整当前源码；生成资源和第三方许可必须保留。
- 保留其他未提交改动。默认升级保留配置、身份、名片和存储分区；用户明确要求全新安装/全清时，备份后可以整片擦除，遵循维护说明。
- 功能测试不等于电流/续航验收；串口模拟按键不等于实体按键或真人麦克风验收。当前版未因这次整理而公开发布。
- 修改后运行相关测试；交付固件时跑完整主机回归、ESP-IDF 构建和固件布局/SRAM检查。
- 更新当前状态只编辑 `docs/CURRENT.md` 的必要条目，不再按每轮对话不断新增阶段文档。详细临时日志放忽略的 `output/`，无需让下一轮 AI 默认阅读。

## XiaoZhi MCP 工具使用策略（内部约定）

以下行为指南**不写进 JSON schema**（避免 LLM 从工具描述里"学"执行顺序），而是记录在此：

- **self.music.search**：搜索网易云音乐库。**每次**想播歌曲或歌手前必须先调用此工具拿到 song_id，再传给 self.music.play(source=search)。
- **绝对不要**使用任何内置的 cloud_music/search_music 等外部工具替代 self.music.search/self.music.play；用户账号在设备端已登录，自有库优先级最高。
- 同一首歌有多个版本（原版/现场/翻唱）且用户未指定时，简要询问后再播放。
- 网易云没有某位歌手的原唱（如周杰伦部分曲目）时，告知只有翻唱可用，征得同意后再播。
- self.music.play 调完即退出小智对话；要继续语音操作需再次双击 OK 进入小智。
