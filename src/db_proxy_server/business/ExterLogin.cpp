#include "ExterLogin.h"

const std::string strLoginUrl = "http://xxxx";
bool CExterLoginStrategy::doLogin(const std::string &strName, const std::string &strPass, IM::BaseDefine::UserInfo &user)
{
  bool bRet = false;
  (void) strName;
  (void) strPass;
  (void) user;
  return bRet;
}
