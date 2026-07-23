#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace Engine {
    // Shared, reference-counted reference to a cached asset. Copying a
    // handle is cheap and shares the same underlying object — after
    // AssetCache::ReloadChanged() runs, every existing copy sees the
    // refreshed asset without being re-fetched from the cache.
    template<typename T>
    using AssetHandle = std::shared_ptr<T>;

    // Loads assets by file path exactly once and hands out shared handles
    // to the cached instance on every later call for the same path.
    template<typename T>
    class AssetCache {
    public:
        // Builds a brand-new asset on a cache miss; the asset never
        // supports hot-reload (ReloadChanged() skips it).
        template<typename LoaderFn>
        AssetHandle<T> Load(const std::filesystem::path &path, LoaderFn &&loader) {
            return Load(path, std::forward<LoaderFn>(loader),
                        [](T &, const std::filesystem::path &) {});
        }

        // reloader repopulates an already-cached asset in place, keeping
        // the same T object alive so every AssetHandle<T> already handed
        // out observes the refresh on its next use.
        template<typename LoaderFn, typename ReloaderFn>
        AssetHandle<T> Load(const std::filesystem::path &path, LoaderFn &&loader,
                             ReloaderFn &&reloader) {
            std::string key = path.string();

            if (const auto it = m_entries.find(key); it != m_entries.end())
                return it->second.asset;

            Entry entry;
            entry.asset = loader();
            entry.path = path;
            entry.lastWriteTime = std::filesystem::last_write_time(path);

            const AssetHandle<T> asset = entry.asset;
            entry.reload = [asset, reloader](const std::filesystem::path &assetPath) {
                reloader(*asset, assetPath);
            };

            const AssetHandle<T> handle = entry.asset;
            m_entries.emplace(std::move(key), std::move(entry));
            return handle;
        }

        // Re-stats every cached path; any whose write time moved since the
        // last load (or the last reload) gets its reloader re-run in place.
        void ReloadChanged() {
            for (auto &[key, entry] : m_entries) {
                const auto writeTime = std::filesystem::last_write_time(entry.path);

                if (writeTime != entry.lastWriteTime) {
                    entry.reload(entry.path);
                    entry.lastWriteTime = writeTime;
                }
            }
        }

    private:
        struct Entry {
            AssetHandle<T> asset;
            std::filesystem::path path;
            std::filesystem::file_time_type lastWriteTime;
            std::function<void(const std::filesystem::path &)> reload;
        };

        std::unordered_map<std::string, Entry> m_entries;
    };
}
