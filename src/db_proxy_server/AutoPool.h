#ifndef __AUTOPOOl_H__
#define __AUTOPOOl_H__

class CDBConn;
class CacheConn;

class CAutoDB
{
 public:
  CAutoDB(const char *pDBName, CDBConn **pDBConn);
  ~CAutoDB();

 private:
  CDBConn *m_pDBConn;
};

class CAutoCache
{
  CAutoCache(const char *pCacheName, CacheConn **pCacheConn);
  ~CAutoCache();

 private:
  CacheConn *m_pCacheConn;
};
#endif /*defined(__AUTOPOOl_H__) */
