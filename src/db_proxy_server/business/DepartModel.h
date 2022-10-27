#ifndef __DEPARTMODEL_H__
#define __DEPARTMODEL_H__

#include "IM.BaseDefine.pb.h"
#include "ImPduBase.h"

class CDepartModel
{
 public:
  static CDepartModel *getInstance();
  ~CDepartModel() {}
  void getChgedDeptId(uint32_t &nLastTime, list<uint32_t> &lsChangedIds);
  void getDepts(list<uint32_t> &lsDeptIds, list<IM::BaseDefine::DepartInfo> &lsDepts);
  void getDept(uint32_t nDeptId, IM::BaseDefine::DepartInfo &cDept);

 private:
  CDepartModel(){};

 private:
  static CDepartModel *m_pInstance;
};

#endif
