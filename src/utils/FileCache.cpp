/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileCache.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/20 02:08:00 by msennane          #+#    #+#             */
/*   Updated: 2025/12/20 21:56:05 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "FileCache.hpp"

FileCache *FileCache::instance = 0;

FileCache::FileCache() : totalSize(0) {}

FileCache::~FileCache() { clear(); }

FileCache &FileCache::getInstance() {
  if (instance == 0) {
    instance = new FileCache();
  }
  return *instance;
}

void FileCache::destroyInstance() {
  if (instance != 0) {
    delete instance;
    instance = 0;
  }
}

std::vector<char> FileCache::get(const std::string &path, time_t currentMtime) {
  std::map<std::string, CacheEntry>::iterator it = cache.find(path);
  if (it == cache.end()) {
    // Cache miss
    return std::vector<char>();
  }

  if (it->second.mtime != currentMtime) {
    totalSize -= it->second.size;

    std::map<std::string, std::list<std::string>::iterator>::iterator lruIt =
        lruIterators.find(path);
    if (lruIt != lruIterators.end()) {
      lruOrder.erase(lruIt->second);
      lruIterators.erase(lruIt);
    }

    cache.erase(it);
    return std::vector<char>();
  }

  touch(path);
  return it->second.data;
}

void FileCache::put(const std::string &path, const std::vector<char> &data,
                    time_t mtime) {
  size_t dataSize = data.size();

  if (dataSize > MAX_FILE_SIZE) {
    return;
  }

  std::map<std::string, CacheEntry>::iterator it = cache.find(path);
  if (it != cache.end()) {
    totalSize -= it->second.size;
    it->second.data = data;
    it->second.mtime = mtime;
    it->second.size = dataSize;
    totalSize += dataSize;
    touch(path);
    return;
  }

  evictIfNeeded(dataSize);

  CacheEntry entry;
  entry.data = data;
  entry.mtime = mtime;
  entry.size = dataSize;
  cache[path] = entry;
  totalSize += dataSize;

  lruOrder.push_front(path);
  lruIterators[path] = lruOrder.begin();
}

void FileCache::clear() {
  cache.clear();
  lruOrder.clear();
  lruIterators.clear();
  totalSize = 0;
}

void FileCache::touch(const std::string &path) {
  std::map<std::string, std::list<std::string>::iterator>::iterator it =
      lruIterators.find(path);
  if (it != lruIterators.end()) {
    lruOrder.erase(it->second);
    lruOrder.push_front(path);
    it->second = lruOrder.begin();
  }
}

void FileCache::evictIfNeeded(size_t newEntrySize) {
  while (!lruOrder.empty() && (cache.size() >= MAX_ENTRIES ||
                               totalSize + newEntrySize > MAX_TOTAL_SIZE)) {
    std::string victim = lruOrder.back();
    lruOrder.pop_back();
    lruIterators.erase(victim);

    std::map<std::string, CacheEntry>::iterator it = cache.find(victim);
    if (it != cache.end()) {
      totalSize -= it->second.size;
      cache.erase(it);
    }
  }
}
