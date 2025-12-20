/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileCache.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/20 02:08:00 by msennane          #+#    #+#             */
/*   Updated: 2025/12/20 02:08:00 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <ctime>
#include <list>
#include <map>
#include <string>
#include <vector>

// Cache entry storing file data with metadata for validation
struct CacheEntry {
  std::vector<char> data; // File contents
  time_t mtime;           // Last modification time for staleness check
  size_t size;            // Size in bytes
};

// LRU file cache singleton for static files
// - Maximum 100 entries
// - Maximum 1MB per file
// - Maximum 50MB total size
class FileCache {
public:
  static const size_t MAX_ENTRIES = 100;
  static const size_t MAX_FILE_SIZE = 1024 * 1024;       // 1MB
  static const size_t MAX_TOTAL_SIZE = 50 * 1024 * 1024; // 50MB

  static FileCache &getInstance();
  static void destroyInstance();

  // Get cached file data; returns empty vector on miss or stale
  // Sets outMtime to cached mtime on hit, useful for validation
  std::vector<char> get(const std::string &path, time_t currentMtime);

  // Store file in cache; evicts LRU entries if limits exceeded
  void put(const std::string &path, const std::vector<char> &data,
           time_t mtime);

  // Clear all cached entries
  void clear();

  // Stats for debugging
  size_t getTotalSize() const { return totalSize; }
  size_t getEntryCount() const { return cache.size(); }

private:
  FileCache();
  ~FileCache();
  FileCache(const FileCache &);
  FileCache &operator=(const FileCache &);

  static FileCache *instance;

  // Main storage: path -> CacheEntry
  std::map<std::string, CacheEntry> cache;

  // LRU order: front = most recently used, back = least recently used
  std::list<std::string> lruOrder;

  // Quick lookup: path -> iterator in lruOrder
  std::map<std::string, std::list<std::string>::iterator> lruIterators;

  size_t totalSize;

  // Move path to front of LRU list (most recently used)
  void touch(const std::string &path);

  // Evict entries until under limits
  void evictIfNeeded(size_t newEntrySize);
};

#endif
