#pragma once

#include <string>
#include <vector>

#include "api/manga_source.hpp"

namespace api
{

// Owns the singleton MangaSource instances and resolves source keys/types.
// The active source is decided by the UI from storage::Settings::source and
// looked up here via ByKey().
class SourceRegistry
{
  public:
    // All registered sources, in picker order (available first).
    static const std::vector<MangaSource*>& All();

    static MangaSource* Get(SourceType type);
    // Resolves a settings key; unknown/empty keys fall back to MangaDex so
    // old settings files never break.
    static MangaSource* ByKey(const std::string& key);
    static std::string KeyOf(SourceType type);
};

} // namespace api
