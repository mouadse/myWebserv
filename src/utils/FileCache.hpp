/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   FileCache.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: msennane <msennane@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/20 02:08:00 by msennane          #+#    #+#             */
/*   Updated: 2025/12/20 21:57:11 by msennane         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FILECACHE_HPP
#define FILECACHE_HPP

#include <ctime>
#include <list>
#include <map>
#include <string>
#include <vector>

struct CacheEntry {
  std::vector<char> data;
  time_t mtime;
  size_t size;
};

class FileCache {
public:
  static const size_t MAX_ENTRIES = 100;
  static const size_t MAX_FILE_SIZE = 1024 * 1024;
  static const size_t MAX_TOTAL_SIZE = 50 * 1024 * 1024;

  static FileCache &getInstance();
  static void destroyInstance();

  std::vector<char> get(const std::string &path, time_t currentMtime);

  void put(const std::string &path, const std::vector<char> &data,
           time_t mtime);

  void clear();

  size_t getTotalSize() const { return totalSize; }
  size_t getEntryCount() const { return cache.size(); }

private:
  FileCache();
  ~FileCache();
  FileCache(const FileCache &);
  FileCache &operator=(const FileCache &);

  static FileCache *instance;
  std::map<std::string, CacheEntry> cache;

  std::list<std::string> lruOrder;

  std::map<std::string, std::list<std::string>::iterator> lruIterators;

  size_t totalSize;

  void touch(const std::string &path);

  void evictIfNeeded(size_t newEntrySize);
};

#endif
