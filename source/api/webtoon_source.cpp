#include "api/webtoon_source.hpp"

#include <algorithm>

#include "net/http_client.hpp"
#include "util/html_util.hpp"

namespace api
{

namespace
{
    constexpr const char* kWebtoonBase = "https://www.webtoons.com";
    // The site accepts plain requests, but serves its full (non-challenge)
    // page only to browser-like clients; the Referer keeps hotlink checks on
    // chapter images happy too.
    constexpr const char* kUserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36";
    constexpr const char* kReferer = "https://www.webtoons.com/";

    std::vector<std::string> webtoonHeaders()
    {
        return {
            "User-Agent: " + std::string(kUserAgent),
            "Referer: " + std::string(kReferer),
            "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
        };
    }

    // Fetches a page with the Webtoon headers, returning "" on any failure.
    std::string fetchPage(const std::string& url)
    {
        net::globalRateLimiter().acquire();
        net::HttpResponse response = net::HttpClient::Get(url, webtoonHeaders());
        if (!response.ok)
            return "";
        return response.body;
    }

    // Both the search page and the ranking page render the same card markup
    // (only the link class differs), so one parser serves both feeds.
    //   <a href="https://www.webtoons.com/en/{genre}/{slug}/list?title_no=N"
    //      class="link _card_item" ... data-title-no="N">
    //      <div class="image_wrap"><img src="{cover}" ...></div>
    //      <div class="info_text"><strong class="title">{title}</strong>...
    //   </a>
    void parseCardItems(const std::string& html, std::vector<Manga>& out)
    {
        const std::string kCardMarker = "class=\"link _card_item\"";
        const std::string kRankingMarker = "class=\"link _ranking_title_a\"";
        const std::string kAnchorStart = "<a href=\"";
        const std::string kTitleOpen = "<strong class=\"title\">";
        const std::string kImgSrc = "src=\"";

        size_t pos = 0;
        while (pos < html.size())
        {
            size_t card = html.find(kCardMarker, pos);
            size_t ranking = html.find(kRankingMarker, pos);
            size_t marker = (card == std::string::npos) ? ranking : (ranking == std::string::npos) ? card : std::min(card, ranking);
            if (marker == std::string::npos)
                break;

            // href lives before the class attribute, inside the same <a> tag.
            size_t hrefTagStart = html.rfind(kAnchorStart, marker);
            if (hrefTagStart != std::string::npos && html.find('>', hrefTagStart) > marker)
            {
                size_t hrefStart = hrefTagStart + kAnchorStart.size();
                size_t hrefEnd = html.find('"', hrefStart);
                if (hrefEnd != std::string::npos)
                {
                    Manga manga;
                    manga.source = SourceType::Webtoon;
                    manga.url = html.substr(hrefStart, hrefEnd - hrefStart);

                    // data-title-no comes after the class attribute.
                    std::string id = util::FindBetween(html, "data-title-no=\"", "\"", marker);
                    if (!id.empty())
                        manga.id = id;

                    // Block ends at the anchor's closing tag.
                    size_t blockEnd = html.find("</a>", marker);
                    if (blockEnd == std::string::npos)
                        blockEnd = html.size();

                    std::string title = util::FindBetween(html, kTitleOpen, "</strong>", marker);
                    if (!title.empty())
                        manga.title = util::HtmlDecode(util::StripTags(title));

                    size_t imgPos = html.find(kImgSrc, marker);
                    if (imgPos != std::string::npos && imgPos < blockEnd)
                    {
                        size_t urlStart = imgPos + kImgSrc.size();
                        size_t urlEnd = html.find('"', urlStart);
                        if (urlEnd != std::string::npos)
                            manga.coverUrl = html.substr(urlStart, urlEnd - urlStart);
                    }

                    out.push_back(std::move(manga));
                }
            }

            pos = marker + 1;
        }
    }

    std::string extractChapterNumber(const std::string& id)
    {
        // Chapter id is "title_no:episode_no".
        size_t colon = id.find(':');
        if (colon == std::string::npos)
            return id;
        return id.substr(colon + 1);
    }
}

WebtoonSource::WebtoonSource()
{
}

SourceInfo WebtoonSource::info() const
{
    // Unavailable by default: the image CDN (webtoon-phinf.pstatic.net /
    // swebtoon-phinf.pstatic.net) cannot be reached from typical
    // mainland-China networks (verified 2026-09-05), so the reader would
    // fail even though search/browse work.
    return { SourceType::Webtoon, "webtoon", "source/name_webtoon", false, "source/unavailable_cdn" };
}

SearchResult WebtoonSource::Search(const std::string& query, int offset, int limit, const SourceOptions& options)
{
    (void)offset;
    (void)limit;
    (void)options;

    SearchResult result;
    std::string url = std::string(kWebtoonBase) + "/en/search?keyword=" + util::UrlEncode(query);
    std::string html = fetchPage(url);
    if (html.empty())
        return result;

    parseCardItems(html, result.items);
    result.ok = true;
    result.total = static_cast<int>(result.items.size());
    return result;
}

SearchResult WebtoonSource::Latest(int offset, int limit, const SourceOptions& options)
{
    // Webtoon dropped the "/originals/{day}" daily page (404 since 2026); the
    // ranking page is the closest stable "what's active" feed, so Latest is
    // served from it too rather than failing.
    return Popular(offset, limit, options);
}

SearchResult WebtoonSource::Popular(int offset, int limit, const SourceOptions& options)
{
    (void)limit;
    (void)options;

    SearchResult result;
    std::string url = std::string(kWebtoonBase) + "/en/ranking/score";
    std::string html = fetchPage(url);
    if (html.empty())
        return result;

    parseCardItems(html, result.items);

    // The ranking page lists ~30 titles on one page; honor offset by slicing.
    int first = std::max(0, offset);
    if (first < static_cast<int>(result.items.size()))
        result.items.erase(result.items.begin(), result.items.begin() + first);

    result.ok = true;
    result.total = static_cast<int>(result.items.size());
    return result;
}

bool WebtoonSource::Detail(const Manga& ref, Manga& out)
{
    if (ref.url.empty())
        return false;

    std::string html = fetchPage(ref.url);
    if (html.empty())
        return false;

    out = ref;
    out.source = SourceType::Webtoon;

    // h1.subj is the clean title (no nested tags); fall back to the first h1
    // only if the marker is gone. StripTags handles any leftover markup.
    std::string title = util::FindBetween(html, "<h1 class=\"subj\">", "</h1>");
    if (title.empty())
        title = util::FindBetween(html, "<h1", "</h1>");
    if (!title.empty())
        out.title = util::HtmlDecode(util::StripTags(title));

    std::string cover = util::FindBetween(html, "<meta property=\"og:image\" content=\"", "\"");
    if (cover.empty())
        cover = util::FindBetween(html, "property=\"og:image\" content=\"", "\"");
    if (!cover.empty())
        out.coverUrl = cover;

    std::string description = util::FindBetween(html, "<meta property=\"og:description\" content=\"", "\"");
    if (description.empty())
        description = util::FindBetween(html, "property=\"og:description\" content=\"", "\"");
    if (!description.empty())
        out.description = util::HtmlDecode(util::StripTags(description));

    // Status heuristic from the page body: Webtoon shows "Updating" for
    // ongoing and "Completed"/"完結" for finished series. Default ongoing.
    out.status = "ongoing";
    if (html.find("연재완료") != std::string::npos || html.find("Completed") != std::string::npos
        || html.find("完結") != std::string::npos || html.find("完结") != std::string::npos)
        out.status = "completed";

    // Tags: the .genre list items inside the aside detail box.
    size_t genrePos = 0;
    while (true)
    {
        size_t genreTag = html.find("class=\"genre\">", genrePos);
        if (genreTag == std::string::npos)
            break;
        size_t end = html.find("</", genreTag);
        if (end != std::string::npos)
        {
            std::string tag = util::HtmlDecode(html.substr(genreTag + std::string("class=\"genre\">").size(), end - genreTag - std::string("class=\"genre\">").size()));
            if (!tag.empty())
                out.tags.push_back(tag);
        }
        genrePos = genreTag + 1;
    }

    return true;
}

ChapterFeed WebtoonSource::Chapters(const Manga& manga, const SourceOptions& options, int offset, int limit)
{
    (void)options;

    ChapterFeed feed;
    if (manga.url.empty())
        return feed;

    std::string html = fetchPage(manga.url);
    if (html.empty())
        return feed;

    // <li class="_episodeItem detail_list_item" id="episode_653"
    //     data-episode-no="653">
    //   <a href=".../viewer?title_no=95&episode_no=653" class="detail_list_link">
    //     <span class="subj"><span>[Season 3] Ep. 235 (Season 3 Finale)</span></span>
    //     <span class="date">Feb 23, 2025</span>
    //   </a>
    const std::string kItemMarker = "class=\"_episodeItem detail_list_item\"";
    const std::string kHref = "href=\"";
    const std::string kSubj = "<span class=\"subj\">";
    const std::string kDate = "<span class=\"date\">";

    size_t pos = 0;
    while (pos < html.size())
    {
        size_t item = html.find(kItemMarker, pos);
        if (item == std::string::npos)
            break;

        size_t blockEnd = html.find("</li>", item);
        if (blockEnd == std::string::npos)
            blockEnd = html.size();

        size_t hrefPos = html.find(kHref, item);
        if (hrefPos == std::string::npos || hrefPos > blockEnd)
        {
            pos = item + 1;
            continue;
        }
        size_t urlStart = hrefPos + kHref.size();
        size_t urlEnd = html.find('"', urlStart);
        if (urlEnd == std::string::npos || urlEnd > blockEnd)
        {
            pos = item + 1;
            continue;
        }
        std::string viewerUrl = html.substr(urlStart, urlEnd - urlStart);
        if (viewerUrl.find("/viewer?") == std::string::npos)
        {
            pos = item + 1;
            continue;
        }

        Chapter chapter;
        chapter.url = viewerUrl;

        // title_no and episode_no are the trailing query params of the
        // viewer URL. Note: never extract "to end of string" via an empty
        // delimiter - find("", pos) returns pos and yields an empty value.
        std::string titleNo;
        size_t titlePos = viewerUrl.find("title_no=");
        if (titlePos != std::string::npos)
        {
            titlePos += std::string("title_no=").size();
            size_t amp = viewerUrl.find('&', titlePos);
            titleNo = viewerUrl.substr(titlePos, (amp == std::string::npos) ? std::string::npos : amp - titlePos);
        }

        std::string episodeNo;
        size_t episodePos = viewerUrl.find("episode_no=");
        if (episodePos != std::string::npos)
        {
            episodePos += std::string("episode_no=").size();
            size_t amp = viewerUrl.find('&', episodePos);
            episodeNo = viewerUrl.substr(episodePos, (amp == std::string::npos) ? std::string::npos : amp - episodePos);
        }

        if (titleNo.empty() || episodeNo.empty())
        {
            pos = item + 1;
            continue;
        }
        chapter.id = titleNo + ":" + episodeNo;
        chapter.chapterNumber = episodeNo;

        std::string title = util::FindBetween(html, kSubj, "</span>", item);
        if (!title.empty())
            chapter.title = util::HtmlDecode(util::StripTags(title));
        if (chapter.title.empty())
            chapter.title = "Ep. " + episodeNo;

        std::string date = util::FindBetween(html, kDate, "</span>", item);
        if (!date.empty())
            chapter.publishAt = date;

        feed.items.push_back(std::move(chapter));
        pos = blockEnd + 5;
    }

    // Newest first on the site; the reader expects oldest-first ordering to
    // page forward naturally, so reverse.
    std::reverse(feed.items.begin(), feed.items.end());

    // offset/limit apply to the reversed list.
    if (offset > 0 && offset < static_cast<int>(feed.items.size()))
        feed.items.erase(feed.items.begin(), feed.items.begin() + offset);
    if (limit > 0 && limit < static_cast<int>(feed.items.size()))
        feed.items.resize(limit);

    feed.ok = true;
    feed.total = static_cast<int>(feed.items.size());
    return feed;
}

AtHomeServer WebtoonSource::Pages(const Manga& manga, const Chapter& chapter, ImageQuality quality)
{
    (void)manga;
    (void)quality;

    AtHomeServer result;
    if (chapter.url.empty())
        return result;

    std::string html = fetchPage(chapter.url);
    if (html.empty())
        return result;

    // <div id="_imageList"> ... <img ... data-url="{page}"> ... </div>
    std::string images = util::FindBetween(html, "id=\"_imageList\"", "</div>");
    if (images.empty())
        return result;

    result.pageUrls = util::FindAllBetween(images, "data-url=\"", "\"");
    result.ok = !result.pageUrls.empty();
    return result;
}

std::string WebtoonSource::CoverUrl(const Manga& manga, const std::string& size)
{
    (void)size;
    return manga.coverUrl;
}

} // namespace api
