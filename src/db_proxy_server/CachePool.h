/*
 * CachePool.h
 *
 *  Created on: 2014年7月24日
 *      Author: ziteng
 *  Modify By ZhangYuanhao
 *  2015-01-13
 *  Add some redis command
 */

#ifndef CACHEPOOL_H_
#define CACHEPOOL_H_

#include <vector>
#include "../base/util.h"
#include "ThreadPool.h"
#include "hiredis.h"

class CachePool;

class CacheConn
{
 public:
  CacheConn(CachePool *pCachePool);
  virtual ~CacheConn();

  int Init();
  const char *GetPoolName();

  string get(string key);
  string setex(string key, int timeout, string value);
  string set(string key, string &value);

  // 批量获取
  bool mget(const vector<string> &keys, map<string, string> &ret_value);
  // 判断一个key是否存在
  bool isExists(string &key);

  // 在field域删除key
  long hdel(string key, string field);
  // 在field域获取key对应的value
  string hget(string key, string field);
  // 从所有域中获取key对应的value
  bool hgetAll(string key, map<string, string> &ret_value);
  // 在field域设置key-value
  long hset(string key, string field, string value);

  // 给field域key-value加value
  long hincrBy(string key, string field, long value);
  // 给key-value加value
  long incrBy(string key, long value);
  // hash.first表示域名，hash.second表示value，给map中所有域增加key-value
  string hmset(string key, map<string, string> &hash);
  // 获取指定域中的key-value
  bool hmget(string key, list<string> &fields, list<string> &ret_value);

  // 原子加减1
  long incr(string key);
  long decr(string key);

  // 在list中头插一个value
  long lpush(string key, string value);
  // 在list中尾插一个value
  long rpush(string key, string value);
  long llen(string key);
  // 获取list指定区间的元素
  // 0 表示第一个元素， 1 表示第二个元素，以此类推
  // -1 表示最后一个元素， -2 表示倒数第二个元素，以此类推
  bool lrange(string key, long start, long end, list<string> &ret_value);

 private:
  CachePool *m_pCachePool;
  redisContext *m_pContext;
  uint64_t m_last_connect_time;
};

class CachePool
{
 public:
  CachePool(const char *pool_name, const char *server_ip, int server_port, int db_num, int max_conn_cnt);
  virtual ~CachePool();

  int Init();

  CacheConn *GetCacheConn();
  void RelCacheConn(CacheConn *pCacheConn);

  const char *GetPoolName() { return m_pool_name.c_str(); }
  const char *GetServerIP() { return m_server_ip.c_str(); }
  int GetServerPort() { return m_server_port; }
  int GetDBNum() { return m_db_num; }

 private:
  string m_pool_name;
  string m_server_ip;
  int m_server_port;
  int m_db_num;

  int m_cur_conn_cnt;
  int m_max_conn_cnt;
  list<CacheConn *> m_free_list;
  CThreadNotify m_free_notify;
};

class CacheManager
{
 public:
  virtual ~CacheManager();

  static CacheManager *getInstance();

  int Init();
  CacheConn *GetCacheConn(const char *pool_name);
  void RelCacheConn(CacheConn *pCacheConn);

 private:
  CacheManager();

 private:
  static CacheManager *s_cache_manager;
  map<string, CachePool *> m_cache_pool_map;
};

#endif /* CACHEPOOL_H_ */
