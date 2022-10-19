#ifndef AUDIO_MODEL_H_
#define AUDIO_MODEL_H_

#include <list>
#include <map>
#include "IM.BaseDefine.pb.h"
#include "public_define.h"
#include "util.h"

using namespace std;

class CAudioModel
{
 public:
  virtual ~CAudioModel();

  static CAudioModel *getInstance();
  void setUrl(string &url);

  bool readAudios(list<IM::BaseDefine::MsgInfo> &messages);

  int saveAudioInfo(uint32_t fromId, uint32_t toId, uint32_t createTime, const char *audioData, uint32_t audioLen);

 private:
  CAudioModel();
  bool readAudioContent(uint32_t duration, uint32_t size, const string &path, IM::BaseDefine::MsgInfo &message);

 private:
  static CAudioModel *m_pInstance;
  string m_strFileSite;
};

#endif /* AUDIO_MODEL_H_ */
