//Copyright (c) 2021 OceanBase
// OceanBase is licensed under Mulan PubL v2.
// You can use this software according to the terms and conditions of the Mulan PubL v2.
// You may obtain a copy of Mulan PubL v2 at:
//          http://license.coscl.org.cn/MulanPubL-2.0
// THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
// MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
// See the Mulan PubL v2 for more details.
#ifndef OB_STORAGE_COMPACTION_MEDIUM_COMPACTION_FUNC_H_
#define OB_STORAGE_COMPACTION_MEDIUM_COMPACTION_FUNC_H_

#include "storage/ls/ob_ls.h"
#include "storage/compaction/ob_partition_merge_policy.h"
#include "share/tablet/ob_tablet_filter.h"
#include "share/ob_tablet_meta_table_compaction_operator.h"
#include "storage/tablet/ob_tablet.h"

namespace oceanbase
{
namespace compaction
{

class ObMediumCompactionScheduleFunc
{
public:
  ObMediumCompactionScheduleFunc(ObLS &ls, ObTablet &tablet)
    : allocator_("MediumSchedule"),
      ls_(ls),
      tablet_(tablet),
      filters_inited_(false),
      filters_()
  {}
  ~ObMediumCompactionScheduleFunc() {}

  static int schedule_tablet_medium_merge(
      ObLS &ls,
      ObTablet &tablet,
      const int64_t major_frozen_scn = 0,
      const bool schedule_with_memtable = false);
  static int get_latest_storage_schema_from_memtable(
    ObIAllocator &allocator,
    const ObIArray<ObITable *> &memtables,
    ObStorageSchema &storage_schema);
  static int get_medium_info_list_from_memtable(
      ObIAllocator &allocator,
      const ObIArray<ObITable *> &memtables,
      ObMediumCompactionInfoList &medium_list);
  static int get_palf_role(const share::ObLSID &ls_id, ObRole &role);
  static int get_table_schema_to_merge(
    const ObTablet &tablet,
    const int64_t schema_version,
    ObIAllocator &allocator,
    ObMediumCompactionInfo &medium_info);

  int schedule_next_medium_for_leader(const int64_t major_snapshot);

  int decide_medium_snapshot(
      const bool is_major,
      const ObAdaptiveMergePolicy::AdaptiveMergeReason merge_reason = ObAdaptiveMergePolicy::AdaptiveMergeReason::NONE);

  int check_medium_finish();

  int64_t to_string(char* buf, const int64_t buf_len) const;

protected:
  int get_status_from_inner_table(share::ObTabletCompactionScnInfo &ret_info);
  int prepare_medium_info(const ObGetMergeTablesResult &result, ObMediumCompactionInfo &medium_info);
  int init_parallel_range(
      const ObGetMergeTablesResult &result,
      ObMediumCompactionInfo &medium_info);
  static int prepare_iter_for_major(
      ObTablet &tablet,
      const ObGetMergeTablesResult &result,
      ObMediumCompactionInfo &medium_info,
      ObTableStoreIterator &table_iter);
  static int prepare_iter_for_medium(
      ObTablet &tablet,
      const ObGetMergeTablesResult &result,
      ObMediumCompactionInfo &medium_info,
      ObTableStoreIterator &table_iter);
  int submit_medium_clog(ObMediumCompactionInfo &medium_info);
  int check_medium_meta_table(
      const int64_t medium_snapshot,
      const share::ObLSID &ls_id,
      const ObTabletID &tablet_id,
      bool &merge_finish);
  int init_tablet_filters();
  static int check_medium_checksum_table(
      const int64_t medium_snapshot,
      const share::ObLSID &ls_id,
      const ObTabletID &tablet_id);
  static int choose_medium_snapshot(
      ObLS &ls,
      ObTablet &tablet,
      const ObAdaptiveMergePolicy::AdaptiveMergeReason &merge_reason,
      ObIAllocator &allocator,
      ObMediumCompactionInfo &medium_info,
      ObGetMergeTablesResult &result);
  static int choose_major_snapshot(
      ObLS &ls,
      ObTablet &tablet,
      const ObAdaptiveMergePolicy::AdaptiveMergeReason &merge_reason,
      ObIAllocator &allocator,
      ObMediumCompactionInfo &medium_info,
      ObGetMergeTablesResult &result);
  static int check_need_merge_and_schedule(
      ObLS &ls,
      ObTablet &tablet,
      const int64_t schedule_scn,
      const ObMediumCompactionInfo::ObCompactionType compaction_type);
  int schedule_next_medium_primary_cluster(const int64_t major_snapshot);

  int get_max_reserved_snapshot(int64_t &max_reserved_snapshot);
  static int get_schedule_medium_from_memtable(
    ObTablet &tablet,
    const int64_t major_frozen_snapshot,
    int64_t &schedule_medium_scn,
    ObMediumCompactionInfo::ObCompactionType &compaction_type);
  static int get_table_id(
      ObMultiVersionSchemaService &schema_service,
      const ObTabletID &tablet_id,
      const int64_t schema_version,
      uint64_t &table_id);
  static const int64_t DEFAULT_SYNC_SCHEMA_CLOG_TIMEOUT = 1000L * 1000L; // 1s
  static const int64_t DEFAULT_SCHEDULE_MEDIUM_INTERVAL = 60L * 1000L * 1000L; // 60s
  static const int64_t SCHEDULE_RANGE_INC_ROW_COUNT_PERCENRAGE_THRESHOLD = 10L;
  static const int64_t SCHEDULE_RANGE_ROW_COUNT_THRESHOLD = 1000 *1000L; // 100w
  static const int64_t MEDIUM_FUNC_CNT = 2;
  typedef int (*ChooseMediumScn)(
      ObLS &ls,
      ObTablet &tablet,
      const ObAdaptiveMergePolicy::AdaptiveMergeReason &merge_reason,
      ObIAllocator &allocator,
      ObMediumCompactionInfo &medium_info,
      ObGetMergeTablesResult &result);
  static ChooseMediumScn choose_medium_scn[MEDIUM_FUNC_CNT];

  typedef int (*PrepareTableIter)(
      ObTablet &tablet,
      const ObGetMergeTablesResult &result,
      ObMediumCompactionInfo &medium_info,
      ObTableStoreIterator &table_iter);
  static PrepareTableIter prepare_table_iter[MEDIUM_FUNC_CNT];

private:
  ObArenaAllocator allocator_;
  ObLS &ls_;
  ObTablet &tablet_;
  bool filters_inited_;
  share::ObTabletReplicaFilterHolder filters_;
};

} //namespace compaction
} // namespace oceanbase

#endif // OB_STORAGE_COMPACTION_MEDIUM_COMPACTION_FUNC_H_