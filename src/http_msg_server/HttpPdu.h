/*
 * HttpPdu.h
 *
 *  Created on: 2013-10-1
 *      Author: ziteng@mogujie.com
 */

#ifndef HTTPPDU_H_
#define HTTPPDU_H_

#include <list>
#include "IM.BaseDefine.pb.h"
#include "ImPduBase.h"
#include "util.h"

// jsonp parameter parser
class CPostDataParser
{
 public:
  CPostDataParser() {}
  virtual ~CPostDataParser() {}

  bool Parse(const char *content);

  char *GetValue(const char *key);

 private:
  std::map<std::string, std::string> m_post_map;
};

char *PackSendResult(uint32_t error_code, const char *error_msg = "");
char *PackSendCreateGroupResult(uint32_t error_code, const char *error_msg, uint32_t group_id);
char *PackGetUserIdByNickNameResult(uint32_t result, std::list<IM::BaseDefine::UserInfo> user_list);
#endif /* HTTPPDU_H_ */
