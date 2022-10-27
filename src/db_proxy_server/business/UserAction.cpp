#include <list>
#include <map>

#include "../DBPool.h"
#include "../ProxyConn.h"
#include "../SyncCenter.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Buddy.pb.h"
#include "IM.Login.pb.h"
#include "UserModel.h"
#include "public_define.h"

namespace DB_PROXY
{
void getUserInfo(CImPdu *pPdu, uint32_t conn_uuid)
{
  IM::Buddy::IMUsersInfoReq msg;
  IM::Buddy::IMUsersInfoRsp msgResp;
  if (msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
  {
    CImPdu *pPduRes = new CImPdu;

    uint32_t from_user_id = msg.user_id();
    uint32_t userCount = msg.user_id_list_size();
    std::list<uint32_t> idList;
    for (uint32_t i = 0; i < userCount; ++i)
    {
      idList.push_back(msg.user_id_list(i));
    }
    std::list<IM::BaseDefine::UserInfo> lsUser;
    CUserModel::getInstance()->getUsers(idList, lsUser);
    msgResp.set_user_id(from_user_id);
    for (list<IM::BaseDefine::UserInfo>::iterator it = lsUser.begin(); it != lsUser.end(); ++it)
    {
      IM::BaseDefine::UserInfo *pUser = msgResp.add_user_info_list();
      //            *pUser = *it;

      pUser->set_user_id(it->user_id());
      pUser->set_user_gender(it->user_gender());
      pUser->set_user_nick_name(it->user_nick_name());
      pUser->set_avatar_url(it->avatar_url());

      pUser->set_sign_info(it->sign_info());
      pUser->set_department_id(it->department_id());
      pUser->set_email(it->email());
      pUser->set_user_real_name(it->user_real_name());
      pUser->set_user_tel(it->user_tel());
      pUser->set_user_domain(it->user_domain());
      pUser->set_status(it->status());
    }
    printf("userId=%u, userCnt=%u\n", from_user_id, userCount);
    msgResp.set_attach_data(msg.attach_data());
    pPduRes->SetPBMsg(&msgResp);
    pPduRes->SetSeqNum(pPdu->GetSeqNum());
    pPduRes->SetServiceId(IM::BaseDefine::SID_BUDDY_LIST);
    pPduRes->SetCommandId(IM::BaseDefine::CID_BUDDY_LIST_USER_INFO_RESPONSE);
    CProxyConn::AddResponsePdu(conn_uuid, pPduRes);
  }
  else
  {
    printf("parse pb failed\n");
  }
}

void getChangedUser(CImPdu *pPdu, uint32_t conn_uuid)
{
  IM::Buddy::IMAllUserReq msg;
  IM::Buddy::IMAllUserRsp msgResp;
  if (msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
  {
    CImPdu *pPduRes = new CImPdu;

    uint32_t nReqId = msg.user_id();
    uint32_t nLastTime = msg.latest_update_time();
    uint32_t nLastUpdate = CSyncCenter::getInstance()->getLastUpdate();

    list<IM::BaseDefine::UserInfo> lsUsers;
    if (nLastUpdate > nLastTime)
    {
      list<uint32_t> lsIds;
      CUserModel::getInstance()->getChangedId(nLastTime, lsIds);
      CUserModel::getInstance()->getUsers(lsIds, lsUsers);
    }
    msgResp.set_user_id(nReqId);
    msgResp.set_latest_update_time(nLastTime);
    for (list<IM::BaseDefine::UserInfo>::iterator it = lsUsers.begin(); it != lsUsers.end(); ++it)
    {
      IM::BaseDefine::UserInfo *pUser = msgResp.add_user_list();
      //            *pUser = *it;
      pUser->set_user_id(it->user_id());
      pUser->set_user_gender(it->user_gender());
      pUser->set_user_nick_name(it->user_nick_name());
      pUser->set_avatar_url(it->avatar_url());
      pUser->set_sign_info(it->sign_info());
      pUser->set_department_id(it->department_id());
      pUser->set_email(it->email());
      pUser->set_user_real_name(it->user_real_name());
      pUser->set_user_tel(it->user_tel());
      pUser->set_user_domain(it->user_domain());
      pUser->set_status(it->status());
    }
    printf("userId=%u,nLastUpdate=%u, last_time=%u, userCnt=%u\n", nReqId, nLastUpdate, nLastTime, msgResp.user_list_size());
    msgResp.set_attach_data(msg.attach_data());
    pPduRes->SetPBMsg(&msgResp);
    pPduRes->SetSeqNum(pPdu->GetSeqNum());
    pPduRes->SetServiceId(IM::BaseDefine::SID_BUDDY_LIST);
    pPduRes->SetCommandId(IM::BaseDefine::CID_BUDDY_LIST_ALL_USER_RESPONSE);
    CProxyConn::AddResponsePdu(conn_uuid, pPduRes);
  }
  else
  {
    printf("parse pb failed\n");
  }
}

void changeUserSignInfo(CImPdu *pPdu, uint32_t conn_uuid)
{
  IM::Buddy::IMChangeSignInfoReq req;
  IM::Buddy::IMChangeSignInfoRsp resp;
  if (req.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
  {
    uint32_t user_id = req.user_id();
    const string &sign_info = req.sign_info();

    bool result = CUserModel::getInstance()->updateUserSignInfo(user_id, sign_info);

    resp.set_user_id(user_id);
    resp.set_result_code(result ? 0 : 1);
    if (result)
    {
      resp.set_sign_info(sign_info);
      printf("changeUserSignInfo sucess, user_id=%u, sign_info=%s\n", user_id, sign_info.c_str());
    }
    else
    {
      printf("changeUserSignInfo false, user_id=%u, sign_info=%s\n", user_id, sign_info.c_str());
    }

    CImPdu *pdu_resp = new CImPdu();
    resp.set_attach_data(req.attach_data());
    pdu_resp->SetPBMsg(&resp);
    pdu_resp->SetSeqNum(pPdu->GetSeqNum());
    pdu_resp->SetServiceId(IM::BaseDefine::SID_BUDDY_LIST);
    pdu_resp->SetCommandId(IM::BaseDefine::CID_BUDDY_LIST_CHANGE_SIGN_INFO_RESPONSE);
    CProxyConn::AddResponsePdu(conn_uuid, pdu_resp);
  }
  else
  {
    printf("changeUserSignInfo: IMChangeSignInfoReq ParseFromArray failed!!!\n");
  }
}
void doPushShield(CImPdu *pPdu, uint32_t conn_uuid)
{
  IM::Login::IMPushShieldReq req;
  IM::Login::IMPushShieldRsp resp;
  if (req.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
  {
    uint32_t user_id = req.user_id();
    uint32_t shield_status = req.shield_status();
    // const string& sign_info = req.sign_info();

    bool result = CUserModel::getInstance()->updatePushShield(user_id, shield_status);

    resp.set_user_id(user_id);
    resp.set_result_code(result ? 0 : 1);
    if (result)
    {
      resp.set_shield_status(shield_status);
      printf("doPushShield sucess, user_id=%u, shield_status=%u\n", user_id, shield_status);
    }
    else
    {
      printf("doPushShield false, user_id=%u, shield_status=%u\n", user_id, shield_status);
    }

    CImPdu *pdu_resp = new CImPdu();
    resp.set_attach_data(req.attach_data());
    pdu_resp->SetPBMsg(&resp);
    pdu_resp->SetSeqNum(pPdu->GetSeqNum());
    pdu_resp->SetServiceId(IM::BaseDefine::SID_LOGIN);
    pdu_resp->SetCommandId(IM::BaseDefine::CID_LOGIN_RES_PUSH_SHIELD);
    CProxyConn::AddResponsePdu(conn_uuid, pdu_resp);
  }
  else
  {
    printf("doPushShield: IMPushShieldReq ParseFromArray failed!!!\n");
  }
}

void doQueryPushShield(CImPdu *pPdu, uint32_t conn_uuid)
{
  IM::Login::IMQueryPushShieldReq req;
  IM::Login::IMQueryPushShieldRsp resp;
  if (req.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
  {
    uint32_t user_id = req.user_id();
    uint32_t shield_status = 0;

    bool result = CUserModel::getInstance()->getPushShield(user_id, &shield_status);

    resp.set_user_id(user_id);
    resp.set_result_code(result ? 0 : 1);
    if (result)
    {
      resp.set_shield_status(shield_status);
      printf("doQueryPushShield sucess, user_id=%u, shield_status=%u\n", user_id, shield_status);
    }
    else
    {
      printf("doQueryPushShield false, user_id=%u\n", user_id);
    }

    CImPdu *pdu_resp = new CImPdu();
    resp.set_attach_data(req.attach_data());
    pdu_resp->SetPBMsg(&resp);
    pdu_resp->SetSeqNum(pPdu->GetSeqNum());
    pdu_resp->SetServiceId(IM::BaseDefine::SID_LOGIN);
    pdu_resp->SetCommandId(IM::BaseDefine::CID_LOGIN_RES_QUERY_PUSH_SHIELD);
    CProxyConn::AddResponsePdu(conn_uuid, pdu_resp);
  }
  else
  {
    printf("doQueryPushShield: IMQueryPushShieldReq ParseFromArray failed!!!\n");
  }
}
};  // namespace DB_PROXY
