#ifndef __EXTERLOGIN_H__
#define __EXTERLOGIN_H__

#include "LoginStrategy.h"

class CExterLoginStrategy : public CLoginStrategy
{
 public:
  virtual bool doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo &user);
};
#endif
