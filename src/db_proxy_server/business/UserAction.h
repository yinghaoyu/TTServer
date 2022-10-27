#ifndef __USER_ACTION_H__
#define __USER_ACTION_H__

#include "ImPduBase.h"

namespace DB_PROXY
{
void getUserInfo(CImPdu *pPdu, uint32_t conn_uuid);
void getChangedUser(CImPdu *pPdu, uint32_t conn_uuid);
void changeUserSignInfo(CImPdu *pPdu, uint32_t conn_uuid);
void doPushShield(CImPdu *pPdu, uint32_t conn_uuid);
void doQueryPushShield(CImPdu *pPdu, uint32_t conn_uuid);
};  // namespace DB_PROXY

#endif /* __USER_ACTION_H__ */
