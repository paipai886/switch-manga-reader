#pragma once

#include <string>
#include <vector>

namespace api
{

// Identifies which online source a Manga/Chapter came from. Every consumer
// that later touches the title (detail, chapters, pages, library) must route
// through the matching MangaSource (see api/manga_source.hpp) instead of
// assuming MangaDex.
enum class SourceType
{
    MangaDex = 0,
    Webtoon = 1,
};

// Image resolution tier requested from a source. MangaDex maps this to its
// data/data-saver at-home variants; sources without quality tiers (Webtoon)
// ignore it.
enum class ImageQuality
{
    Data, // original quality
    DataSaver, // compressed
};

struct Manga
{
    // Source-local identifier. MangaDex: the UUID. Webtoon: "title_no".
    std::string id;
    // Source-local page/ref URL when the source needs more than the id to
    // fetch detail (Webtoon stores its /list?title_no= page here; MangaDex
    // leaves it empty and works off the id alone).
    std::string url;
    std::string title;
    std::string description;
    std::string status; // "ongoing", "completed", "hiatus", "cancelled"
    std::string contentRating; // "safe", "suggestive", "erotica", "pornographic"
    // MangaDex-only: cover relationship attribute, kept for library.json
    // compatibility. Empty for sources that don't use it.
    std::string coverFileName;
    // Generic ready-to-use cover URL (source-agnostic). MangaDex fills this
    // via CoverUrl() and Webtoon uses the CDN thumbnail directly.
    std::string coverUrl;
    SourceType source = SourceType::MangaDex;
    std::vector<std::string> tags;
};

struct SearchResult
{
    bool ok = false; // false means the request itself failed (network/TLS/HTTP error)
    long httpStatus = 0;
    std::string errorDetail; // libcurl error string, set when httpStatus stays 0
    std::vector<Manga> items;
    int total = 0;
};

struct Chapter
{
    std::string id;
    // Source-local reader URL when the source needs more than the id to
    // fetch pages (Webtoon stores its full /viewer?title_no=&episode_no=
    // URL here; MangaDex leaves it empty and works off the id alone).
    std::string url;
    std::string chapterNumber; // string on purpose: MangaDex allows "3.5" etc, and null -> ""
    std::string volume;
    std::string title;
    std::string translatedLanguage;
    int pageCount = 0;
    std::string publishAt;
    // Non-empty when this chapter isn't hosted on MangaDex at all - it's a
    // pointer to an official/licensed reader elsewhere (e.g. MangaPlus).
    // /at-home/server has no page data to serve for these.
    std::string externalUrl;
};

struct ChapterFeed
{
    bool ok = false; // false means the request itself failed (network/TLS/HTTP error)
    long httpStatus = 0;
    std::string errorDetail; // libcurl error string, set when httpStatus stays 0
    std::vector<Chapter> items;
    int total = 0;
};

struct AtHomeServer
{
    bool ok = false;
    std::vector<std::string> pageUrls;
};

} // namespace api
