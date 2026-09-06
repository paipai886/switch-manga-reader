#pragma once

#include <string>
#include <vector>

#include "api/models.hpp"

namespace api
{

// Source-agnostic browsing options. Only the fields a source understands are
// used; every field has a sensible default, so adapters never crash on
// options they ignore.
struct SourceOptions
{
    // MangaDex content-rating whitelist ("safe", "suggestive", ...).
    std::vector<std::string> contentRatings;
    // MangaDex translatedLanguage filter; empty = no filter. Webtoon ignores
    // this and always serves its English catalog.
    std::string language;
};

// Public metadata shown by the settings source picker.
struct SourceInfo
{
    SourceType type;
    // Stable settings key ("mangadex", "webtoon") persisted to settings.json.
    const char* key;
    // i18n key for the display name ("source/name_mangadex", ...).
    const char* nameKey;
    // false = known to be unusable from typical CN networks (Cloudflare JS
    // challenges, geo-blocks, blocked CDNs). The picker shows it greyed out
    // with reasonKey instead of letting the user select a source whose
    // requests will all fail.
    bool available;
    // i18n key for why it's unavailable (empty when available).
    const char* reasonKey;
};

// One online manga/comic backend behind a common facade. All methods are
// blocking network calls - only ever call them from worker threads, exactly
// like MangaDexClient before them. Adapters translate the common models to
// the backend's own wire format (JSON REST, HTML pages, protobuf, ...).
class MangaSource
{
  public:
    virtual ~MangaSource() = default;

    virtual SourceInfo info() const = 0;

    // Browse feeds. A source without a meaningful feed returns
    // {ok=true, items={}, total=0} instead of an error (Webtoon has no
    // distinct "latest" page).
    virtual SearchResult Search(const std::string& query, int offset, int limit, const SourceOptions& options) = 0;
    virtual SearchResult Latest(int offset, int limit, const SourceOptions& options) = 0;
    virtual SearchResult Popular(int offset, int limit, const SourceOptions& options) = 0;

    // Fills out (title/description/status/tags/coverUrl) for a list item.
    virtual bool Detail(const Manga& ref, Manga& out) = 0;

    // Chapter feed for a manga. offset/limit are honored where the backend
    // supports pagination; adapters without pagination return everything.
    virtual ChapterFeed Chapters(const Manga& manga, const SourceOptions& options, int offset, int limit) = 0;

    // Resolves a chapter to its page image URLs.
    virtual AtHomeServer Pages(const Manga& manga, const Chapter& chapter, ImageQuality quality) = 0;

    // Ready-to-fetch cover URL ("256"/"512" thumbnail, empty = full size).
    // Adapters that can't resize (Webtoon) ignore size and return the CDN URL.
    virtual std::string CoverUrl(const Manga& manga, const std::string& size) = 0;
};

} // namespace api
