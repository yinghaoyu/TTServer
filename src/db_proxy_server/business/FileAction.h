#ifndef __FILEACTION_H__
#define __FILEACTION_H__

#include "ImPduBase.h"

namespace DB_PROXY
{
void hasOfflineFile(CImPdu *pPdu, uint32_t conn_uuid);
void addOfflineFile(CImPdu *pPdu, uint32_t conn_uuid);
void delOfflineFile(CImPdu *pPdu, uint32_t conn_uuid);
};  // namespace DB_PROXY

#endif
