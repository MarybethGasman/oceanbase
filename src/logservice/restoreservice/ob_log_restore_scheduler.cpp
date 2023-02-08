/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#include "ob_log_restore_scheduler.h"
#include "lib/ob_define.h"
#include "lib/ob_errno.h"
#include "lib/oblog/ob_log_module.h"
#include "ob_remote_fetch_log_worker.h"
#include "ob_log_restore_allocator.h"
#include "observer/omt/ob_tenant_config_mgr.h"
#include <cstdint>

namespace oceanbase
{
namespace logservice
{
using namespace oceanbase::common;
ObLogRestoreScheduler::ObLogRestoreScheduler() :
  inited_(false),
  tenant_id_(OB_INVALID_TENANT_ID),
  worker_(NULL)
{}

ObLogRestoreScheduler::~ObLogRestoreScheduler()
{
  destroy();
}

int ObLogRestoreScheduler::init(const uint64_t tenant_id,
    ObLogRestoreAllocator *allocator,
    ObRemoteFetchWorker *worker)
{
  int ret = OB_SUCCESS;
  if (inited_) {
    ret = OB_INIT_TWICE;
    CLOG_LOG(WARN, "ObLogRestoreScheduler init twice", K(ret), K(inited_));
  } else if (OB_INVALID_TENANT_ID == tenant_id
      || NULL == allocator
      || NULL == worker) {
    ret = OB_INVALID_ARGUMENT;
    CLOG_LOG(WARN, "invalid argument", K(ret), K(tenant_id), K(allocator), K(worker));
  } else {
    tenant_id_ = tenant_id;
    allocator_ = allocator;
    worker_ = worker;
    inited_ = true;
    CLOG_LOG(INFO, "ObLogRestoreScheduler init succ", K(tenant_id_));
  }
  return ret;
}

void ObLogRestoreScheduler::destroy()
{
  inited_ = false;
  tenant_id_ = OB_INVALID_TENANT_ID;
  worker_ = NULL;
}

int ObLogRestoreScheduler::schedule()
{
  (void)modify_thread_count_();
  (void)purge_cached_buffer_();
  return OB_SUCCESS;
}

int ObLogRestoreScheduler::modify_thread_count_()
{
  int ret = OB_SUCCESS;
  omt::ObTenantConfigGuard tenant_config(TENANT_CONF(tenant_id_));
  const int64_t restore_concurrency =
    tenant_config.is_valid() ? tenant_config->log_restore_concurrency : 1L;
  if (OB_FAIL(worker_->modify_thread_count(std::max(1L, restore_concurrency)))) {
    CLOG_LOG(WARN, "modify worker thread failed", K(ret));
  }
  return ret;
}

int ObLogRestoreScheduler::purge_cached_buffer_()
{
  int ret = OB_SUCCESS;
  allocator_->weed_out_iterator_buffer();
  return ret;
}
} // namespace logservice
} // namespace oceanbase