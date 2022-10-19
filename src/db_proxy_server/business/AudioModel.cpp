#include "AudioModel.h"
#include "../DBPool.h"
#include "../HttpClient.h"

#include <memory>

using namespace std;

CAudioModel *CAudioModel::m_pInstance = NULL;

CAudioModel::CAudioModel() {}

CAudioModel::~CAudioModel() {}

// not thread safe
CAudioModel *CAudioModel::getInstance()
{
  if (!m_pInstance)
  {
    m_pInstance = new CAudioModel();
  }

  return m_pInstance;
}

// 设置语音存储的url地址
void CAudioModel::setUrl(string &url)
{
  m_strFileSite = url;
  if (m_strFileSite.back() != '/')
  {
    m_strFileSite += "/";
  }
}

// 读取语音消息
// 根据语音的Id，读出内容存到messages
bool CAudioModel::readAudios(list<IM::BaseDefine::MsgInfo> &messages)
{
  if (messages.empty())
  {
    return true;
  }
  bool ret = false;
  CDBManager *mgr = CDBManager::getInstance();
  CDBConn *conn = mgr->GetDBConn("teamtalk_slave");
  if (conn)
  {
    for (auto it = messages.begin(); it != messages.end();)
    {
      IM::BaseDefine::MsgType nType = it->msg_type();
      if ((IM::BaseDefine::MSG_TYPE_GROUP_AUDIO == nType) || (IM::BaseDefine::MSG_TYPE_SINGLE_AUDIO == nType))
      {
        string sql = "select * from IMAudio where id=" + it->msg_data();
        CResultSet *resultSet = conn->ExecuteQuery(sql.c_str());
        if (resultSet)
        {
          while (resultSet->Next())
          {
            uint32_t duration = resultSet->GetInt("duration");
            uint32_t size = resultSet->GetInt("size");
            string path = resultSet->GetString("path");
            readAudioContent(duration, size, path, *it);
          }
          ++it;
          delete resultSet;
        }
        else
        {
          printf("no result for sql:%s\n", sql.c_str());
          it = messages.erase(it);
        }
      }
      else
      {
        ++it;
      }
    }
    mgr->RelDBConn(conn);
    ret = true;
  }
  else
  {
    puts("no connection for teamtalk_slave");
  }
  return ret;
}

// 存储语音消息
// nFromId     发送者Id
// nToId       接收者Id
// nCreateTime 发送时间
// pAudioData  指向语音消息的指针
// nAudioLen   语音消息的长度
// 成功返回语音id，失败返回-1
int CAudioModel::saveAudioInfo(uint32_t fromId, uint32_t toId, uint32_t createTime, const char *audioData, uint32_t audioLen)
{
  // parse audio data
  uint32_t costTime = CByteStream::ReadUint32((uchar_t *) audioData);
  uchar_t *realData = (uchar_t *) audioData + sizeof(uint32_t);
  uint32_t realLen = audioLen - sizeof(uint32_t);
  int audioId = -1;

  CHttpClient httpClient;
  string strPath = httpClient.UploadByteFile(m_strFileSite, realData, realLen);
  if (!strPath.empty())
  {
    CDBManager *mgr = CDBManager::getInstance();
    CDBConn *conn = mgr->GetDBConn("teamtalk_master");
    if (conn)
    {
      uint32_t pos = 0;
      string sql =
          "insert into IMAudio(`fromId`, `toId`, `path`, `size`, `duration`, `created`) "
          "values(?, ?, ?, ?, ?, ?)";
      replace_mark(sql, fromId, pos);
      replace_mark(sql, toId, pos);
      replace_mark(sql, strPath, pos);
      replace_mark(sql, realLen, pos);
      replace_mark(sql, costTime, pos);
      replace_mark(sql, createTime, pos);
      if (conn->ExecuteUpdate(sql.c_str()))
      {
        audioId = conn->GetInsertId();
        printf("audioId=%d\n", audioId);
      }
      else
      {
        printf("sql failed: %s\n", sql.c_str());
      }
      mgr->RelDBConn(conn);
    }
    else
    {
      puts("no db connection for teamtalk_master");
    }
  }
  else
  {
    puts("upload file failed");
  }
  return audioId;
}

// 读取语音的具体内容
// nCostTime 语音时长
// nSize     语音大小
// strPath   语音存储的url
// cMsg      msg结构体
// 成功返回true，失败返回false
bool CAudioModel::readAudioContent(uint32_t duration, uint32_t size, const string &path, IM::BaseDefine::MsgInfo &message)
{
  if (path.empty() || duration == 0 || size == 0)
  {
    return false;
  }

  // 分配内存，写入音频时长
  AudioMsgInfo audioMsg;
  shared_ptr<uchar_t> data(new uchar_t[sizeof(uint32_t) + size]);
  audioMsg.data = data.get();
  CByteStream::WriteUint32(audioMsg.data, duration);
  audioMsg.data_len = 4;
  audioMsg.fileSize = size;

  // 获取音频数据，写入上面分配的内存
  CHttpClient httpClient;
  if (!httpClient.DownloadByteFile(path, &audioMsg))
  {
    return false;
  }

  printf("download_path=%s, data_len=%d\n", path.c_str(), audioMsg.data_len);
  message.set_msg_data((const char *) audioMsg.data, audioMsg.data_len);

  return true;
}
