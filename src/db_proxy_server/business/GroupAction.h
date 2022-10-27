#ifndef GROUPACTION_H_
#define GROUPACTION_H_

#include "ImPduBase.h"

namespace DB_PROXY
{
void createGroup(CImPdu *pPdu, uint32_t conn_uuid);

void getNormalGroupList(CImPdu *pPdu, uint32_t conn_uuid);

void getGroupInfo(CImPdu *pPdu, uint32_t conn_uuid);

void modifyMember(CImPdu *pPdu, uint32_t conn_uuid);

void setGroupPush(CImPdu *pPdu, uint32_t conn_uuid);

void getGroupPush(CImPdu *pPdu, uint32_t conn_uuid);

};  // namespace DB_PROXY

#endif
