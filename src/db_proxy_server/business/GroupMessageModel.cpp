#include <map>
#include <memory>
#include <set>

#include "../CachePool.h"
#include "../DBPool.h"
#include "AudioModel.h"
#include "Common.h"
#include "GroupMessageModel.h"
#include "GroupModel.h"
#include "MessageCounter.h"
#include "SessionModel.h"

using namespace std;

extern string strAudioEnc;

CGroupMessageModel *CGroupMessageModel::m_pInstance = NULL;

CGroupMessageModel::CGroupMessageModel() {}

CGroupMessageModel::~CGroupMessageModel() {}

CGroupMessageModel *CGroupMessageModel::getInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CGroupMessageModel();
  }

  return m_pInstance;
}

//  发送群消息接口
//  relateId     关系Id
//  fromId       发送者Id
//  groupId      群组Id
//  msgType      消息类型
//  createTime   消息创建时间
//  msgId        消息Id
//  msgContent 消息类容
//  成功返回true 失败返回false
bool CGroupMessageModel::sendMessage(uint32_t fromId,
                                     uint32_t groupId,
                                     IM::BaseDefine::MsgType msgType,
                                     uint32_t createTime,
                                     uint32_t msgId,
                                     const string &msgContent)
{
  bool ret = false;
  // 先在redis中查询用户是否在这个群组中
  if (CGroupModel::getInstance()->isInGroup(fromId, groupId))
  {
    CDBManager *mgr = CDBManager::getInstance();
    CDBConn *conn = mgr->GetDBConn("teamtalk_master");
    if (conn)
    {
      string tableName = "IMGroupMessage_" + int2string(groupId % 8);
      string sql = "insert into " + tableName +
                   " (`groupId`, `userId`, `msgId`, `content`, `type`, `status`, `updated`, `created`) "
                   "values(?, ?, ?, ?, ?, ?, ?, ?)";

      // 必须在释放连接前delete CPrepareStatement对象，否则有可能多个线程操作mysql对象，会crash

      std::shared_ptr<CPrepareStatement> stmt(new CPrepareStatement());
      if (stmt->Init(conn->GetMysql(), sql))
      {
        uint32_t status = 0;
        uint32_t type = msgType;
        uint32_t index = 0;
        stmt->SetParam(index++, groupId);
        stmt->SetParam(index++, fromId);
        stmt->SetParam(index++, msgId);
        stmt->SetParam(index++, msgContent);
        stmt->SetParam(index++, type);
        stmt->SetParam(index++, status);
        stmt->SetParam(index++, createTime);
        stmt->SetParam(index++, createTime);

        if (stmt->ExecuteUpdate())
        {
          CGroupModel::getInstance()->updateGroupChat(groupId);
          incMessageCount(fromId, groupId);
          clearMessageCount(fromId, groupId);
        }
        else
        {
          printf("insert message failed: %s\n", sql.c_str());
        }
      }
      mgr->RelDBConn(conn);
    }
    else
    {
      printf("no db connection for teamtalk_master\n");
    }
  }
  else
  {
    printf("not in the group.fromId=%u, groupId=%u", fromId, groupId);
  }
  return ret;
}

/**
 *  发送群组语音信息
 *
 *  @param nRelateId   关系Id
 *  @param nFromId     发送者Id
 *  @param nGroupId    群组Id
 *  @param nMsgType    消息类型
 *  @param nCreateTime 消息创建时间
 *  @param nMsgId      消息Id
 *  @param pMsgContent 指向语音类容的指针
 *  @param nMsgLen     语音消息长度
 *
 *  @return 成功返回true，失败返回false
 */
bool CGroupMessageModel::sendAudioMessage(uint32_t fromId,
                                          uint32_t groupId,
                                          IM::BaseDefine::MsgType msgType,
                                          uint32_t createTime,
                                          uint32_t msgId,
                                          const char *msgContent,
                                          uint32_t msgLen)
{
  if (msgLen <= 4)
  {
    return false;
  }

  if (!CGroupModel::getInstance()->isInGroup(fromId, groupId))
  {
    printf("not in the group.fromId=%u, groupId=%u\n", fromId, groupId);
    return false;
  }

  CAudioModel *pAudioModel = CAudioModel::getInstance();
  int nAudioId = pAudioModel->saveAudioInfo(fromId, groupId, createTime, msgContent, msgLen);

  bool bRet = true;
  if (nAudioId != -1)
  {
    string strMsg = int2string(nAudioId);
    bRet = sendMessage(fromId, groupId, msgType, createTime, msgId, strMsg);
  }
  else
  {
    bRet = false;
  }

  return bRet;
}

/**
 *  清除群组消息计数
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *
 *  @return 成功返回true，失败返回false
 */
bool CGroupMessageModel::clearMessageCount(uint32_t userId, uint32_t groupId)
{
  bool bRet = false;
  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    string strGroupKey = int2string(groupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
    map<string, string> mapGroupCount;
    bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
    pCacheManager->RelCacheConn(pCacheConn);
    if (bRet)
    {
      string strUserKey = int2string(userId) + "_" + int2string(groupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
      if (strReply.empty())
      {
        printf("hmset %s failed !\n", strUserKey.c_str());
      }
      else
      {
        bRet = true;
      }
    }
    else
    {
      printf("hgetAll %s failed !\n", strGroupKey.c_str());
    }
  }
  else
  {
    printf("no cache connection for unread\n");
  }
  return bRet;
}

/**
 *  增加群消息计数
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *
 *  @return 成功返回true，失败返回false
 */
bool CGroupMessageModel::incMessageCount(uint32_t nUserId, uint32_t nGroupId)
{
  bool bRet = false;
  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
    pCacheConn->hincrBy(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD, 1);
    map<string, string> mapGroupCount;
    bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
    if (bRet)
    {
      string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
      if (!strReply.empty())
      {
        bRet = true;
      }
      else
      {
        printf("hmset %s failed !\n", strUserKey.c_str());
      }
    }
    else
    {
      printf("hgetAll %s failed!\n", strGroupKey.c_str());
    }
    pCacheManager->RelCacheConn(pCacheConn);
  }
  else
  {
    printf("no cache connection for unread\n");
  }
  return bRet;
}

/**
 *  获取群组消息列表
 *
 *  @param nUserId  用户Id
 *  @param nGroupId 群组Id
 *  @param nMsgId   开始的msgId(最新的msgId)
 *  @param nMsgCnt  获取的长度
 *  @param lsMsg    消息列表
 */
void CGroupMessageModel::getMessage(uint32_t nUserId, uint32_t nGroupId, uint32_t nMsgId, uint32_t nMsgCnt, list<IM::BaseDefine::MsgInfo> &lsMsg)
{
  //根据 count 和 lastId 获取信息
  string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);

  CDBManager *pDBManager = CDBManager::getInstance();
  CDBConn *pDBConn = pDBManager->GetDBConn("teamtalk_slave");
  if (pDBConn)
  {
    uint32_t nUpdated = CGroupModel::getInstance()->getUserJoinTime(nGroupId, nUserId);
    //如果nMsgId 为0 表示客户端想拉取最新的nMsgCnt条消息
    string strSql;
    if (nMsgId == 0)
    {
      strSql = "select * from " + strTableName + " where groupId = " + int2string(nGroupId) + " and status = 0 and created>=" + int2string(nUpdated) +
               " order by created desc, id desc limit " + int2string(nMsgCnt);
    }
    else
    {
      strSql = "select * from " + strTableName + " where groupId = " + int2string(nGroupId) + " and msgId<=" + int2string(nMsgId) +
               " and status = 0 and created>=" + int2string(nUpdated) + " order by created desc, id desc limit " + int2string(nMsgCnt);
    }

    CResultSet *pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
    if (pResultSet)
    {
      map<uint32_t, IM::BaseDefine::MsgInfo> mapAudioMsg;
      while (pResultSet->Next())
      {
        IM::BaseDefine::MsgInfo msg;
        msg.set_msg_id(pResultSet->GetInt("msgId"));
        msg.set_from_session_id(pResultSet->GetInt("userId"));
        msg.set_create_time(pResultSet->GetInt("created"));
        IM::BaseDefine::MsgType nMsgType = IM::BaseDefine::MsgType(pResultSet->GetInt("type"));
        if (IM::BaseDefine::MsgType_IsValid(nMsgType))
        {
          msg.set_msg_type(nMsgType);
          msg.set_msg_data(pResultSet->GetString("content"));
          lsMsg.push_back(msg);
        }
        else
        {
          // log("invalid msgType. userId=%u, groupId=%u, msgType=%u", nUserId, nGroupId, nMsgType);
        }
      }
      delete pResultSet;
    }
    else
    {
      // log("no result set for sql: %s", strSql.c_str());
    }
    pDBManager->RelDBConn(pDBConn);
    if (!lsMsg.empty())
    {
      CAudioModel::getInstance()->readAudios(lsMsg);
    }
  }
  else
  {
    // log("no db connection for teamtalk_slave");
  }
}

/**
 *  获取用户群未读消息计数
 *
 *  @param nUserId       用户Id
 *  @param nTotalCnt     总条数
 *  @param lsUnreadCount 每个会话的未读信息包含了条数，最后一个消息的Id，最后一个消息的类型，最后一个消息的类容
 */
void CGroupMessageModel::getUnreadMsgCount(uint32_t nUserId, uint32_t &nTotalCnt, list<IM::BaseDefine::UnreadInfo> &lsUnreadCount)
{
  list<uint32_t> lsGroupId;
  CGroupModel::getInstance()->getUserGroupIds(nUserId, lsGroupId, 0);
  uint32_t nCount = 0;

  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    for (auto it = lsGroupId.begin(); it != lsGroupId.end(); ++it)
    {
      uint32_t nGroupId = *it;
      string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strGroupCnt = pCacheConn->hget(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
      if (strGroupCnt.empty())
      {
        //                //log("hget %s : count failed !", strGroupKey.c_str());
        continue;
      }
      uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));

      string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strUserCnt = pCacheConn->hget(strUserKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);

      uint32_t nUserCnt = (strUserCnt.empty() ? 0 : ((uint32_t) atoi(strUserCnt.c_str())));
      if (nGroupCnt >= nUserCnt)
      {
        nCount = nGroupCnt - nUserCnt;
      }
      if (nCount > 0)
      {
        IM::BaseDefine::UnreadInfo cUnreadInfo;
        cUnreadInfo.set_session_id(nGroupId);
        cUnreadInfo.set_session_type(IM::BaseDefine::SESSION_TYPE_GROUP);
        cUnreadInfo.set_unread_cnt(nCount);
        nTotalCnt += nCount;
        string strMsgData;
        uint32_t nMsgId;
        IM::BaseDefine::MsgType nType;
        uint32_t nFromId;
        getLastMsg(nGroupId, nMsgId, strMsgData, nType, nFromId);
        if (IM::BaseDefine::MsgType_IsValid(nType))
        {
          cUnreadInfo.set_latest_msg_id(nMsgId);
          cUnreadInfo.set_latest_msg_data(strMsgData);
          cUnreadInfo.set_latest_msg_type(nType);
          cUnreadInfo.set_latest_msg_from_user_id(nFromId);
          lsUnreadCount.push_back(cUnreadInfo);
        }
        else
        {
          // log("invalid msgType. userId=%u, groupId=%u, msgType=%u, msgId=%u", nUserId, nGroupId, nType, nMsgId);
        }
      }
    }
    pCacheManager->RelCacheConn(pCacheConn);
  }
  else
  {
    // log("no cache connection for unread");
  }
}

/**
 *  获取一个群组的msgId，自增，通过redis控制
 *
 *  @param nGroupId 群Id
 *
 *  @return 返回msgId
 */
uint32_t CGroupMessageModel::getMsgId(uint32_t nGroupId)
{
  uint32_t nMsgId = 0;
  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    // TODO
    string strKey = "group_msg_id_" + int2string(nGroupId);
    nMsgId = pCacheConn->incrBy(strKey, 1);
    pCacheManager->RelCacheConn(pCacheConn);
  }
  else
  {
    // log("no cache connection for unread");
  }
  return nMsgId;
}

/**
 *  获取一个群的最后一条消息
 *
 *  @param nGroupId   群Id
 *  @param nMsgId     最后一条消息的msgId,引用
 *  @param strMsgData 最后一条消息的内容,引用
 *  @param nMsgType   最后一条消息的类型,引用
 */
void CGroupMessageModel::getLastMsg(uint32_t nGroupId, uint32_t &nMsgId, string &strMsgData, IM::BaseDefine::MsgType &nMsgType, uint32_t &nFromId)
{
  string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);

  CDBManager *pDBManager = CDBManager::getInstance();
  CDBConn *pDBConn = pDBManager->GetDBConn("teamtalk_slave");
  if (pDBConn)
  {
    string strSql = "select msgId, type,userId, content from " + strTableName + " where groupId = " + int2string(nGroupId) +
                    " and status = 0 order by created desc, id desc limit 1";

    CResultSet *pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
    if (pResultSet)
    {
      while (pResultSet->Next())
      {
        nMsgId = pResultSet->GetInt("msgId");
        nMsgType = IM::BaseDefine::MsgType(pResultSet->GetInt("type"));
        nFromId = pResultSet->GetInt("userId");
        if (nMsgType == IM::BaseDefine::MSG_TYPE_GROUP_AUDIO)
        {
          // "[语音]"加密后的字符串
          strMsgData = strAudioEnc;
        }
        else
        {
          strMsgData = pResultSet->GetString("content");
        }
      }
      delete pResultSet;
    }
    else
    {
      // log("no result set for sql: %s", strSql.c_str());
    }
    pDBManager->RelDBConn(pDBConn);
  }
  else
  {
    // log("no db connection for teamtalk_slave");
  }
}

/**
 *  获取某个用户所有群的所有未读计数之和
 *
 *  @param nUserId   用户Id
 *  @param nTotalCnt 未读计数之后,引用
 */
void CGroupMessageModel::getUnReadCntAll(uint32_t nUserId, uint32_t &nTotalCnt)
{
  list<uint32_t> lsGroupId;
  CGroupModel::getInstance()->getUserGroupIds(nUserId, lsGroupId, 0);
  uint32_t nCount = 0;

  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    for (auto it = lsGroupId.begin(); it != lsGroupId.end(); ++it)
    {
      uint32_t nGroupId = *it;
      string strGroupKey = int2string(nGroupId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strGroupCnt = pCacheConn->hget(strGroupKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);
      if (strGroupCnt.empty())
      {
        //                //log("hget %s : count failed !", strGroupKey.c_str());
        continue;
      }
      uint32_t nGroupCnt = (uint32_t)(atoi(strGroupCnt.c_str()));

      string strUserKey = int2string(nUserId) + "_" + int2string(nGroupId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
      string strUserCnt = pCacheConn->hget(strUserKey, GROUP_COUNTER_SUBKEY_COUNTER_FIELD);

      uint32_t nUserCnt = (strUserCnt.empty() ? 0 : ((uint32_t) atoi(strUserCnt.c_str())));
      if (nGroupCnt >= nUserCnt)
      {
        nCount = nGroupCnt - nUserCnt;
      }
      if (nCount > 0)
      {
        nTotalCnt += nCount;
      }
    }
    pCacheManager->RelCacheConn(pCacheConn);
  }
  else
  {
    // log("no cache connection for unread");
  }
}

void CGroupMessageModel::getMsgByMsgId(uint32_t nUserId, uint32_t nGroupId, const list<uint32_t> &lsMsgId, list<IM::BaseDefine::MsgInfo> &lsMsg)
{
  if (!lsMsgId.empty())
  {
    if (CGroupModel::getInstance()->isInGroup(nUserId, nGroupId))
    {
      CDBManager *pDBManager = CDBManager::getInstance();
      CDBConn *pDBConn = pDBManager->GetDBConn("teamtalk_slave");
      if (pDBConn)
      {
        string strTableName = "IMGroupMessage_" + int2string(nGroupId % 8);
        uint32_t nUpdated = CGroupModel::getInstance()->getUserJoinTime(nGroupId, nUserId);
        string strClause;
        bool bFirst = true;
        for (auto it = lsMsgId.begin(); it != lsMsgId.end(); ++it)
        {
          if (bFirst)
          {
            bFirst = false;
            strClause = int2string(*it);
          }
          else
          {
            strClause += ("," + int2string(*it));
          }
        }

        string strSql = "select * from " + strTableName + " where groupId=" + int2string(nGroupId) + " and msgId in (" + strClause +
                        ") and status=0 and created >= " + int2string(nUpdated) + " order by created desc, id desc limit 100";
        CResultSet *pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
        if (pResultSet)
        {
          while (pResultSet->Next())
          {
            IM::BaseDefine::MsgInfo msg;
            msg.set_msg_id(pResultSet->GetInt("msgId"));
            msg.set_from_session_id(pResultSet->GetInt("userId"));
            msg.set_create_time(pResultSet->GetInt("created"));
            IM::BaseDefine::MsgType nMsgType = IM::BaseDefine::MsgType(pResultSet->GetInt("type"));
            if (IM::BaseDefine::MsgType_IsValid(nMsgType))
            {
              msg.set_msg_type(nMsgType);
              msg.set_msg_data(pResultSet->GetString("content"));
              lsMsg.push_back(msg);
            }
            else
            {
              // log("invalid msgType. userId=%u, groupId=%u, msgType=%u", nUserId, nGroupId, nMsgType);
            }
          }
          delete pResultSet;
        }
        else
        {
          // log("no result set for sql:%s", strSql.c_str());
        }
        pDBManager->RelDBConn(pDBConn);
        if (!lsMsg.empty())
        {
          CAudioModel::getInstance()->readAudios(lsMsg);
        }
      }
      else
      {
        // log("no db connection for teamtalk_slave");
      }
    }
    else
    {
      // log("%u is not in group:%u", nUserId, nGroupId);
    }
  }
  else
  {
    // log("msgId is empty.");
  }
}

bool CGroupMessageModel::resetMsgId(uint32_t nGroupId)
{
  bool bRet = false;
  CacheManager *pCacheManager = CacheManager::getInstance();
  CacheConn *pCacheConn = pCacheManager->GetCacheConn("unread");
  if (pCacheConn)
  {
    string strKey = "group_msg_id_" + int2string(nGroupId);
    string strValue = "0";
    string strReply = pCacheConn->set(strKey, strValue);
    if (strReply == strValue)
    {
      bRet = true;
    }
    pCacheManager->RelCacheConn(pCacheConn);
  }
  return bRet;
}
