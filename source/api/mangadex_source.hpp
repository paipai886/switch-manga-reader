#pragma once

#include "api/manga_source.hpp"
#include "api/mangadex_client.hpp"

namespace api
{

// MangaDex adapter. This is a thin pass-through over the existing
// MangaDexClient - every REST call, rate limiter and parse rule is
// unchanged, so the original behaviour is preserved exactly. It only adds
// the SourceType/coverUrl plumbing the multi-source layer needs.
class MangaDexSource : public MangaSource
{
  public:
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
