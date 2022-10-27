#ifndef GROUP_MESSAGE_MODEL_H_
#define GROUP_MESSAGE_MODEL_H_

#include <list>
#include <string>

#include "AudioModel.h"
#include "GroupModel.h"
#include "IM.BaseDefine.pb.h"
#include "ImPduBase.h"
#include "util.h"

using namespace std;

class CGroupMessageModel
{
 public:
  virtual ~CGroupMessageModel();
  static CGroupMessageModel *getInstance();

  bool sendMessage(uint32_t fromId, uint32_t groupId, IM::BaseDefine::MsgType msgType, uint32_t createTime, uint32_t msgId, const string &msgContent);
  bool sendAudioMessage(uint32_t fromId,
                        uint32_t groupId,
                        IM::BaseDefine::MsgType msgType,
                        uint32_t createTime,
                        uint32_t msgId,
                        const char *msgContent,
                        uint32_t msgLen);
  void getMessage(uint32_t userId, uint32_t groupId, uint32_t msgId, uint32_t msgCnt, list<IM::BaseDefine::MsgInfo> &lsMsg);
  bool clearMessageCount(uint32_t userId, uint32_t groupId);
  uint32_t getMsgId(uint32_t groupId);
  void getUnreadMsgCount(uint32_t userId, uint32_t &totalCnt, list<IM::BaseDefine::UnreadInfo> &unreadCount);
  void getLastMsg(uint32_t groupId, uint32_t &msgId, string &strMsgData, IM::BaseDefine::MsgType &msgType, uint32_t &fromId);
  void getUnReadCntAll(uint32_t userId, uint32_t &totalCnt);
  void getMsgByMsgId(uint32_t userId, uint32_t groupId, const list<uint32_t> &msgIds, list<IM::BaseDefine::MsgInfo> &messages);
  bool resetMsgId(uint32_t groupId);

 private:
  CGroupMessageModel();
  bool incMessageCount(uint32_t userId, uint32_t groupId);

 private:
  static CGroupMessageModel *m_pInstance;
};

#endif /* MESSAGE_MODEL_H_ */
