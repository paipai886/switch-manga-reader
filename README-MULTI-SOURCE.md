# MangaDex Reader 多源化改造说明

本目录是 gomprime/mangadex-reader（MIT）的**多源 fork（v1.1.0）**。
在原版「只读 MangaDex」的基础上，新增了一层可插拔的**数据源抽象**，
当前注册两个源：**MangaDex（可用）** 与 **Webtoon（结构就绪，默认关闭）**。

---

## 1. 这版改了什么

### 新增文件（均在 source/ 下）

| 文件 | 作用 |
| --- | --- |
| `api/manga_source.hpp` | 源接口：`MangaSource`（Search / Latest / Popular / Detail / Chapters / Pages / CoverUrl）+ `SourceOptions` + `SourceInfo` |
| `api/source_registry.hpp/.cpp` | 源注册表：`All()` / `Get(type)` / `ByKey(key)`，未知 key 回退 MangaDex |
| `api/mangadex_source.hpp/.cpp` | MangaDex 适配器：包一层原 `MangaDexClient`，行为不变 |
| `api/webtoon_source.hpp/.cpp` | Webtoon 适配器：解析真实页面 HTML（见第 4 节） |
| `util/html_util.hpp/.cpp` | 轻量 HTML 提取工具（FindBetween / StripTags / HtmlDecode / UrlEncode） |

### 修改文件

| 文件 | 改动 |
| --- | --- |
| `api/models.hpp` | `Manga` 加 `url`/`coverUrl`/`source` 字段；`Chapter` 加 `url`；`ImageQuality` 下沉到此处 |
| `api/mangadex_client.hpp` | 移除重复定义的 `ImageQuality`（移至 models.hpp） |
| `storage/library_store.hpp/.cpp` | `Settings` 加 `source`；`LibraryEntry` 加 `source`（库条目记录来源）；`Contains/Remove` 按 (id, source) 匹配 |
| `ui/browse_tab.cpp` | 浏览页经 `SourceRegistry::ByKey(settings.source)` 调 Latest/Popular |
| `ui/search_tab.cpp` | 搜索页同样经源对象调 Search |
| `ui/manga_detail_activity.cpp` | 详情/封面/章节列表全部经 `SourceRegistry::Get(manga.source)`；收藏/继续按源读写库 |
| `ui/reader_activity.cpp` | 章节列表/页面解析经 `manga.source` 定源（库条目跨源切换也不会用错源） |
| `ui/library_tab.cpp` | 库条目恢复 `source` 并预计算封面 URL |
| `ui/manga_card.cpp` | 封面走通用 `coverUrl`（不再假设 MangaDex） |
| `ui/settings_tab.hpp/.cpp` | 设置页新增「数据源」切换行，不可用源自动跳过并显示原因 |
| `Makefile` | `APP_VERSION` 1.0.4 → 1.1.0 |
| `romfs/i18n/en-US|pt-BR/settings.json` | 新增 `source_header` 键 |
| `romfs/i18n/en-US|pt-BR/source.json` | 新增：`name_mangadex` / `name_webtoon` / `unavailable_cdn` |

### 数据流（改造后）

```
浏览/搜索/详情/阅读器  —— 只认 MangaSource 接口 ——>  SourceRegistry
                                                      ├─ MangaDexSource ──> api.mangadex.org（REST + JSON）
                                                      └─ WebtoonSource ──> www.webtoons.com（HTML 页面解析）
```

设置里切换「数据源」→ 写入 `settings.json` 的 `source` 字段；
老配置文件没有该字段 → 按 `"mangadex"` 处理，完全向后兼容。

---

## 2. 数据源现状（2026-09-05 实测）

| 源 | 页面/接口 | 正文图片 | 结论 |
| --- | --- | --- | --- |
| **MangaDex** | ✅ api.mangadex.org 直连 | ✅ uploads.mangadex.org 直连 | **端到端可用，默认源** |
| **Webtoon** | ✅ 搜索/排行/详情/章节/阅读器页全部可解析 | ❌ `webtoon-phinf.pstatic.net` 在本网络不可达 | 代码就绪，**默认关闭**，网络环境允许后一键启用 |

其他候选（ComicK / MangaPlus / CopyManga / Komiic / mangabz / MangaSee123 / dm5 / baozimh）
均因 Cloudflare 人机挑战、地区封锁、DNS 不可达或动态渲染等因素实测不可用，详见对话记录。

### Webtoon 为什么默认关闭

能浏览、能搜、能列章节，但**正文图片 CDN（webtoon-phinf.pstatic.net）
从大陆网络无法直连**——即便选上也看不了图，所以注册表把它标为
`available=false`，设置页会显示「(image CDN unreachable from CN networks)」并跳过它。

**启用方法**（任选其一，改完重新编译）：
1. 让 Switch 走代理 / 改 hosts 使 pstatic.net 可达；
2. 编辑 `source/api/webtoon_source.cpp` 的 `info()`，把 `available` 改为 `true`。

---

## 3. 如何编译出 NRO

需要 devkitPro 工具链（`DEVKITPRO` 环境变量）+ borealis 子模块（官方 zip 不含）：

```bash
cd mangadex-reader-master
# 二进制资产来自官方 zip；本仓库只推文本源码
curl -L -o upstream.zip https://codeload.github.com/gomprime/mangadex-reader/zip/refs/heads/master
unzip -o upstream.zip
cp -r mangadex-reader-master/external ./
cp mangadex-reader-master/romfs/cacert.pem romfs/cacert.pem 2>/dev/null || true
git clone https://github.com/gomprime/mangadex-reader-borealis.git external/borealis
export DEVKITPRO=/opt/devkitpro
make            # 产物：mangadex-reader.nro
```

> 本机（Windows，无 devkitPro）无法执行 `make`，代码已用 MSVC `/Zs`
> 对新文件做语法级校验通过，并已用真实页面验证解析逻辑；真正的
> 构建/装机验证需要在装有 devkitPro 的环境完成（见 GitHub Actions workflow）。

---

## 4. 新增一个源怎么做（约定）

1. 建 `api/xxx_source.hpp/.cpp`，继承 `api::MangaSource`，实现 7 个方法；
2. 在 `SourceInfo` 里填 `key`（设置持久化键）、`nameKey`（i18n 键）、
   `available`/`reasonKey`（不可用时给用户看的原因）；
3. 在 `source_registry.cpp` 的 `Registry` 里加一个实例，即注册完成；
4. `Manga`/`Chapter` 的 `url` 字段是给「需要 URL 才能取详情/图片」的源
   用的（Webtoon 就是），API 型源（MangaDex）留空即可；
5. 新增 i18n 键放到 `romfs/i18n/{en-US,pt-BR}/source.json`。

每页/每图请求请在适配器内调用 `net::globalRateLimiter().acquire()`
（或自建限流器），并遵守各站 ToS 与频率限制。

---

## 5. 已验证的内容

- **解析逻辑**：`verify_webtoon_parser.py` 用真实抓取的 Webtoon 页面
  （搜索/排行/详情/阅读器）逐项校验——搜索 15 条卡片含 Tower of God、
  排行 30 条、章节 9 条（编号升序、id 唯一、标题无残留标签）、单话 140 张图
  URL，全部通过。
- **编译层面**：5 个新增/改动较大的源文件经 MSVC（`cl /Zs /std:c++17`）
  语法检查，0 error / 0 warning；`MangaDexSource` 对 `MangaDexClient`
  的调用与真实头文件签名逐一对齐。
- **未验证**：borealis 依赖的 UI 文件无法在本机编译（缺 devkitPro +
  borealis 头），仅做了人工逐段复核；NRO 成品未产出。
