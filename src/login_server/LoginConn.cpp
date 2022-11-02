#include "LoginConn.h"
#include "IM.Login.pb.h"
#include "IM.Other.pb.h"
#include "IM.Server.pb.h"
#include "public_define.h"
using namespace IM::BaseDefine;
static ConnMap_t g_client_conn_map;
static ConnMap_t g_msg_serv_conn_map;
static uint32_t g_total_online_user_cnt = 0;  // 并发在线总人数
map<uint32_t, msg_serv_info_t *> g_msg_serv_info;

void login_conn_timer_callback(void *callback_data, uint8_t msg, uint32_t handle, void *pParam)
{
  uint64_t cur_time = get_tick_count();
  // 调用每个客户端连接的时间函数
  for (ConnMap_t::iterator it = g_client_conn_map.begin(); it != g_client_conn_map.end();)
  {
    ConnMap_t::iterator it_old = it;
    it++;

    CLoginConn *pConn = (CLoginConn *) it_old->second;
    pConn->OnTimer(cur_time);
  }
  // 调用每个消息服务端的时间函数
  for (ConnMap_t::iterator it = g_msg_serv_conn_map.begin(); it != g_msg_serv_conn_map.end();)
  {
    ConnMap_t::iterator it_old = it;
    it++;

    CLoginConn *pConn = (CLoginConn *) it_old->second;
    pConn->OnTimer(cur_time);
  }
}

void init_login_conn()
{
  // 为客户端和消息服务端的连接注册超时处理
  netlib_register_timer(login_conn_timer_callback, NULL, 1000);
}

CLoginConn::CLoginConn() {}

CLoginConn::~CLoginConn() {}

void CLoginConn::Close()
{
  if (m_handle != NETLIB_INVALID_HANDLE)
  {
    // close socket
    netlib_close(m_handle);
    if (m_conn_type == LOGIN_CONN_TYPE_CLIENT)
    {
      // 清除客户端映射表
      g_client_conn_map.erase(m_handle);
    }
    else
    {
      g_msg_serv_conn_map.erase(m_handle);

      // 把所有客户端连接信息从这个消息服务器里面剔除
      map<uint32_t, msg_serv_info_t *>::iterator it = g_msg_serv_info.find(m_handle);
      if (it != g_msg_serv_info.end())
      {
        msg_serv_info_t *pMsgServInfo = it->second;

        g_total_online_user_cnt -= pMsgServInfo->cur_conn_cnt;
        printf("onclose from MsgServer: %s:%u\n", pMsgServInfo->hostname.c_str(), pMsgServInfo->port);
        delete pMsgServInfo;
        g_msg_serv_info.erase(it);
      }
    }
  }

  ReleaseRef();
}

void CLoginConn::OnConnect2(net_handle_t handle, int conn_type)
{
  m_handle = handle;
  m_conn_type = conn_type;
  ConnMap_t *conn_map = &g_msg_serv_conn_map;
  if (conn_type == LOGIN_CONN_TYPE_CLIENT)
  {
    conn_map = &g_client_conn_map;
  }
  else

    conn_map->insert(make_pair(handle, this));

  netlib_option(handle, NETLIB_OPT_SET_CALLBACK, (void *) imconn_callback);
  netlib_option(handle, NETLIB_OPT_SET_CALLBACK_DATA, (void *) conn_map);
}

void CLoginConn::OnClose()
{
  Close();
}

void CLoginConn::OnTimer(uint64_t curr_tick)
{
  if (m_conn_type == LOGIN_CONN_TYPE_CLIENT)
  {
    // 客户端连接超时，则关闭这个连接
    if (curr_tick > m_last_recv_tick + CLIENT_TIMEOUT)
    {
      Close();
    }
  }
  else
  {
    // 往消息服务器发送心跳包
    if (curr_tick > m_last_send_tick + SERVER_HEARTBEAT_INTERVAL)
    {
      IM::Other::IMHeartBeat msg;
      CImPdu pdu;
      pdu.SetPBMsg(&msg);
      pdu.SetServiceId(SID_OTHER);
      pdu.SetCommandId(CID_OTHER_HEARTBEAT);
      SendPdu(&pdu);
    }

    // 消息服务端超时，则关闭这个连接
    if (curr_tick > m_last_recv_tick + SERVER_TIMEOUT)
    {
      printf("connection to MsgServer timeout\n");
      Close();
    }
  }
}

void CLoginConn::HandlePdu(CImPdu *pPdu)
{
  switch (pPdu->GetCommandId())
  {
  case CID_OTHER_HEARTBEAT:
    break;
  case CID_OTHER_MSG_SERV_INFO:
    _HandleMsgServInfo(pPdu);
    break;
  case CID_OTHER_USER_CNT_UPDATE:
    _HandleUserCntUpdate(pPdu);
    break;
  case CID_LOGIN_REQ_MSGSERVER:
    _HandleMsgServRequest(pPdu);
    break;

  default:
    // log("wrong msg, cmd id=%d ", pPdu->GetCommandId());
    break;
  }
}

void CLoginConn::_HandleMsgServInfo(CImPdu *pPdu)
{
  // 接收消息服务器上报的信息
  msg_serv_info_t *pMsgServInfo = new msg_serv_info_t;
  IM::Server::IMMsgServInfo msg;
  msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

  pMsgServInfo->ip_addr1 = msg.ip1();
  pMsgServInfo->ip_addr2 = msg.ip2();
  pMsgServInfo->port = msg.port();
  pMsgServInfo->max_conn_cnt = msg.max_conn_cnt();
  pMsgServInfo->cur_conn_cnt = msg.cur_conn_cnt();
  pMsgServInfo->hostname = msg.host_name();
  g_msg_serv_info.insert(make_pair(m_handle, pMsgServInfo));

  g_total_online_user_cnt += pMsgServInfo->cur_conn_cnt;

  printf("MsgServInfo, ip_addr1=%s, ip_addr2=%s, port=%d, max_conn_cnt=%d, cur_conn_cnt=%d, hostname: %s.\n", pMsgServInfo->ip_addr1.c_str(),
         pMsgServInfo->ip_addr2.c_str(), pMsgServInfo->port, pMsgServInfo->max_conn_cnt, pMsgServInfo->cur_conn_cnt, pMsgServInfo->hostname.c_str());
}

void CLoginConn::_HandleUserCntUpdate(CImPdu *pPdu)
{
  // 统计客户端在线、离线数量
  map<uint32_t, msg_serv_info_t *>::iterator it = g_msg_serv_info.find(m_handle);
  if (it != g_msg_serv_info.end())
  {
    msg_serv_info_t *pMsgServInfo = it->second;
    IM::Server::IMUserCntUpdate msg;
    msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

    uint32_t action = msg.user_action();
    if (action == USER_CNT_INC)
    {
      pMsgServInfo->cur_conn_cnt++;
      g_total_online_user_cnt++;
    }
    else
    {
      pMsgServInfo->cur_conn_cnt--;
      g_total_online_user_cnt--;
    }

    printf("%s:%d, cur_cnt=%u, total_cnt=%u\n", pMsgServInfo->hostname.c_str(), pMsgServInfo->port, pMsgServInfo->cur_conn_cnt, g_total_online_user_cnt);
  }
}

void CLoginConn::_HandleMsgServRequest(CImPdu *pPdu)
{
  IM::Login::IMMsgServReq msg;
  msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength());

  printf("HandleMsgServReq.\n");

  // no MessageServer available
  if (g_msg_serv_info.size() == 0)
  {
    IM::Login::IMMsgServRsp msg;
    msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NO_MSG_SERVER);
    CImPdu pdu;
    pdu.SetPBMsg(&msg);
    pdu.SetServiceId(SID_LOGIN);
    pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
    pdu.SetSeqNum(pPdu->GetSeqNum());
    SendPdu(&pdu);
    Close();
    return;
  }

  // 找出一个压力最小的消息服务器
  msg_serv_info_t *pMsgServInfo;
  uint32_t min_user_cnt = (uint32_t) -1;
  map<uint32_t, msg_serv_info_t *>::iterator it_min_conn = g_msg_serv_info.end(), it;

  for (it = g_msg_serv_info.begin(); it != g_msg_serv_info.end(); it++)
  {
    pMsgServInfo = it->second;
    if ((pMsgServInfo->cur_conn_cnt < pMsgServInfo->max_conn_cnt) && (pMsgServInfo->cur_conn_cnt < min_user_cnt))
    {
      it_min_conn = it;
      min_user_cnt = pMsgServInfo->cur_conn_cnt;
    }
  }

  if (it_min_conn == g_msg_serv_info.end())
  {
    // 所有消息服务器满载
    printf("All TCP MsgServer are full\n");
    IM::Login::IMMsgServRsp msg;
    msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_MSG_SERVER_FULL);
    CImPdu pdu;
    pdu.SetPBMsg(&msg);
    pdu.SetServiceId(SID_LOGIN);
    pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
    pdu.SetSeqNum(pPdu->GetSeqNum());
    SendPdu(&pdu);
  }
  else
  {
    // 告知消息服务器的ip地址及端口号
    IM::Login::IMMsgServRsp msg;
    msg.set_result_code(::IM::BaseDefine::REFUSE_REASON_NONE);
    msg.set_prior_ip(it_min_conn->second->ip_addr1);
    msg.set_backip_ip(it_min_conn->second->ip_addr2);
    msg.set_port(it_min_conn->second->port);
    CImPdu pdu;
    pdu.SetPBMsg(&msg);
    pdu.SetServiceId(SID_LOGIN);
    pdu.SetCommandId(CID_LOGIN_RES_MSGSERVER);
    pdu.SetSeqNum(pPdu->GetSeqNum());
    SendPdu(&pdu);
  }

  Close();  // after send MsgServResponse, active close the connection
}
