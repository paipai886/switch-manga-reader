#include "util/html_util.hpp"

#include <curl/curl.h>

namespace util
{

std::vector<std::string> FindAllBetween(const std::string& haystack, const std::string& from, const std::string& to)
{
    std::vector<std::string> out;
    size_t pos = 0;

    while (pos < haystack.size())
    {
        size_t begin = haystack.find(from, pos);
        if (begin == std::string::npos)
            break;
        begin += from.size();

        size_t end = haystack.find(to, begin);
        if (end == std::string::npos)
            break;

        out.push_back(haystack.substr(begin, end - begin));
        pos = end + to.size();
    }

    return out;
}

std::string FindBetween(const std::string& haystack, const std::string& from, const std::string& to, size_t startPos)
{
    size_t begin = haystack.find(from, startPos);
    if (begin == std::string::npos)
        return "";
    begin += from.size();

    size_t end = haystack.find(to, begin);
    if (end == std::string::npos)
        return "";

    return haystack.substr(begin, end - begin);
}

size_t FindNext(const std::string& haystack, const std::string& marker, size_t startPos)
{
    size_t pos = haystack.find(marker, startPos);
    if (pos == std::string::npos)
        return std::string::npos;
    return pos + marker.size();
}

std::string HtmlDecode(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    for (size_t i = 0; i < text.size();)
    {
        if (text[i] != '&')
        {
            out += text[i++];
            continue;
        }

        static const std::pair<const char*, const char*> kEntities[] = {
            { "&amp;", "&" },
            { "&lt;", "<" },
            { "&gt;", ">" },
            { "&quot;", "\"" },
            { "&#39;", "'" },
            { "&#x27;", "'" },
            { "&nbsp;", " " },
        };

        bool matched = false;
        for (const auto& [entity, replacement] : kEntities)
        {
            if (text.compare(i, strlen(entity), entity) == 0)
            {
                out += replacement;
                i += strlen(entity);
                matched = true;
                break;
            }
        }

        if (!matched)
            out += text[i++];
    }

    return out;
}

std::string StripTags(const std::string& text)
{
    std::string out;
    out.reserve(text.size());

    bool inTag = false;
    for (char c : text)
    {
        if (c == '<')
            inTag = true;
        else if (c == '>')
            inTag = false;
        else if (!inTag)
            out += c;
    }

    return out;
}

std::string UrlEncode(const std::string& value)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return value;

    char* escaped = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.length()));
    std::string result = escaped ? escaped : value;

    if (escaped)
        curl_free(escaped);
    curl_easy_cleanup(curl);

    return result;
}

} // namespace util
