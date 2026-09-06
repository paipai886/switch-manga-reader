#pragma once

#include <string>
#include <vector>

// Minimal, targeted HTML extraction helpers used by the HTML-page-based
// sources (Webtoon). Deliberately not a general parser: each helper matches
// one stable markup pattern observed on the real pages, so the surface area
// (and the chance of surprises) stays small. All helpers are pure string
// operations - no external dependencies, safe to call from worker threads.
namespace util
{

// Finds every substring between `from` and `to`, scanning left to right
// without nesting awareness. Used to split a page into repeating item blocks
// (e.g. all `<a ... class="link _card_item">...</a>` chunks) or to extract
// attribute values between quote marks.
std::vector<std::string> FindAllBetween(const std::string& haystack, const std::string& from, const std::string& to);

// Finds the first occurrence between `from` and `to` after `startPos`.
// Returns empty when not found.
std::string FindBetween(const std::string& haystack, const std::string& from, const std::string& to, size_t startPos = 0);

// Finds the first occurrence of `marker` in the block starting at startPos
// and returns the position just past it, or std::string::npos when absent.
// Used to walk a list of item blocks one at a time.
size_t FindNext(const std::string& haystack, const std::string& marker, size_t startPos);

// Decodes the handful of HTML entities that actually appear in titles and
// descriptions on the captured pages. Anything else is passed through.
std::string HtmlDecode(const std::string& text);

// Removes all <...> tag spans (Webtoon titles are wrapped in an extra inner
// <span>, and og:description carries <b>/<br> markup). Text between tags is
// kept; call HtmlDecode() afterwards.
std::string StripTags(const std::string& text);

// URL-encodes a query string component (space -> %20, etc.), using the same
// rules as the MangaDex client's internal urlEncode.
std::string UrlEncode(const std::string& value);

} // namespace util
