#include "api/mangadex_source.hpp"

namespace api
{

namespace
{
    void tagAsMangaDex(Manga& manga)
    {
        manga.source = SourceType::MangaDex;
        if (manga.coverUrl.empty() && !manga.coverFileName.empty())
            manga.coverUrl = MangaDexClient::CoverUrl(manga.id, manga.coverFileName);
    }
}

SourceInfo MangaDexSource::info() const
{
    return { SourceType::MangaDex, "mangadex", "source/name_mangadex", true, "" };
}

SearchResult MangaDexSource::Search(const std::string& query, int offset, int limit, const SourceOptions& options)
{
    SearchResult result = MangaDexClient::SearchManga(query, offset, limit, options.contentRatings, options.language);
    for (auto& manga : result.items)
        tagAsMangaDex(manga);
    return result;
}

SearchResult MangaDexSource::Latest(int offset, int limit, const SourceOptions& options)
{
    SearchResult result = MangaDexClient::GetLatestUpdates(offset, limit, options.contentRatings, options.language);
    for (auto& manga : result.items)
        tagAsMangaDex(manga);
    return result;
}

SearchResult MangaDexSource::Popular(int offset, int limit, const SourceOptions& options)
{
    SearchResult result = MangaDexClient::GetPopular(offset, limit, options.contentRatings, options.language);
    for (auto& manga : result.items)
        tagAsMangaDex(manga);
    return result;
}

bool MangaDexSource::Detail(const Manga& ref, Manga& out)
{
    bool ok = MangaDexClient::GetMangaDetail(ref.id, out);
    if (ok)
        tagAsMangaDex(out);
    return ok;
}

ChapterFeed MangaDexSource::Chapters(const Manga& manga, const SourceOptions& options, int offset, int limit)
{
    return MangaDexClient::GetChapterFeed(manga.id, options.language, offset, limit);
}

AtHomeServer MangaDexSource::Pages(const Manga& manga, const Chapter& chapter, ImageQuality quality)
{
    (void)manga;
    return MangaDexClient::GetAtHomeServer(chapter.id, quality);
}

std::string MangaDexSource::CoverUrl(const Manga& manga, const std::string& size)
{
    return MangaDexClient::CoverUrl(manga.id, manga.coverFileName, size);
}

} // namespace api
