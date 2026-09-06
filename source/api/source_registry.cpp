#include "api/source_registry.hpp"

#include "api/mangadex_source.hpp"
#include "api/webtoon_source.hpp"

namespace api
{

namespace
{
    struct Registry
    {
        MangaDexSource mangadex;
        WebtoonSource webtoon;
        std::vector<MangaSource*> all { &mangadex, &webtoon };
    };

    Registry& instance()
    {
        static Registry registry;
        return registry;
    }
}

const std::vector<MangaSource*>& SourceRegistry::All()
{
    return instance().all;
}

MangaSource* SourceRegistry::Get(SourceType type)
{
    for (MangaSource* source : instance().all)
    {
        if (source->info().type == type)
            return source;
    }
    return instance().all.front();
}

MangaSource* SourceRegistry::ByKey(const std::string& key)
{
    for (MangaSource* source : instance().all)
    {
        if (key == source->info().key)
            return source;
    }
    // Unknown or empty key (old settings) -> the first registered source.
    return instance().all.front();
}

std::string SourceRegistry::KeyOf(SourceType type)
{
    return Get(type)->info().key;
}

} // namespace api
