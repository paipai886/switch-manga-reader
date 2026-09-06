#pragma once

#include "api/manga_source.hpp"

namespace api
{

// Webtoon (www.webtoons.com, Naver's official webtoon platform) adapter.
//
// The site serves regular HTML with no Cloudflare JS challenge, so a plain
// libcurl client can read it - but it is registered as unavailable by
// default: the image CDN (webtoon-phinf.pstatic.net / swebtoon-phinf.
// pstatic.net) is unreachable from typical mainland-China networks, so the
// reader would fail even though search/browse work. Flip SourceInfo.available
// to true (or run the Switch behind a proxy) to use it.
//
// All parsing targets markup captured from the real pages in 2026-09:
//   search  -> <a ... class="link _card_item" data-title-no="N"> blocks
//   ranking -> <a ... class="link _ranking_title_a" data-title-no="N">
//   detail  -> h1.subj / meta[og:image] / meta[og:description]
//   chapters-> <li class="_episodeItem detail_list_item" data-episode-no>
//   viewer  -> <div id="_imageList"> <img data-url="...">
class WebtoonSource : public MangaSource
{
  public:
    WebtoonSource();

    SourceInfo info() const override;

    SearchResult Search(const std::string& query, int offset, int limit, const SourceOptions& options) override;
    SearchResult Latest(int offset, int limit, const SourceOptions& options) override;
    SearchResult Popular(int offset, int limit, const SourceOptions& options) override;

    bool Detail(const Manga& ref, Manga& out) override;
    ChapterFeed Chapters(const Manga& manga, const SourceOptions& options, int offset, int limit) override;
    AtHomeServer Pages(const Manga& manga, const Chapter& chapter, ImageQuality quality) override;
    std::string CoverUrl(const Manga& manga, const std::string& size) override;
};

} // namespace api
