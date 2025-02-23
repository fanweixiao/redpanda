/*
 * Copyright 2020 Vectorized, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#pragma once

#include "cluster/controller.h"
#include "cluster/metadata_cache.h"
#include "cluster/rm_group_proxy.h"
#include "cluster/tm_stm.h"
#include "cluster/topics_frontend.h"
#include "cluster/tx_gateway.h"
#include "cluster/types.h"
#include "config/configuration.h"
#include "model/metadata.h"
#include "seastarx.h"

namespace cluster {

class tx_gateway_frontend final
  : public ss::peering_sharded_service<tx_gateway_frontend> {
public:
    tx_gateway_frontend(
      ss::smp_service_group,
      ss::sharded<cluster::partition_manager>&,
      ss::sharded<cluster::shard_table>&,
      ss::sharded<cluster::metadata_cache>&,
      ss::sharded<rpc::connection_cache>&,
      ss::sharded<partition_leaders_table>&,
      cluster::controller*,
      ss::sharded<cluster::id_allocator_frontend>&,
      rm_group_proxy*,
      ss::sharded<cluster::rm_partition_frontend>&);

    ss::future<std::optional<model::node_id>> get_tx_broker();
    ss::future<init_tm_tx_reply>
      init_tm_tx(kafka::transactional_id, model::timeout_clock::duration);
    ss::future<cluster::init_tm_tx_reply> init_tm_tx_locally(
      kafka::transactional_id, model::timeout_clock::duration);
    ss::future<add_paritions_tx_reply> add_partition_to_tx(
      add_paritions_tx_request, model::timeout_clock::duration);
    ss::future<add_offsets_tx_reply>
      add_offsets_to_tx(add_offsets_tx_request, model::timeout_clock::duration);
    ss::future<end_tx_reply>
      end_txn(end_tx_request, model::timeout_clock::duration);

    ss::future<> stop();

private:
    ss::gate _gate;
    ss::smp_service_group _ssg;
    ss::sharded<cluster::partition_manager>& _partition_manager;
    ss::sharded<cluster::shard_table>& _shard_table;
    ss::sharded<cluster::metadata_cache>& _metadata_cache;
    ss::sharded<rpc::connection_cache>& _connection_cache;
    ss::sharded<partition_leaders_table>& _leaders;
    cluster::controller* _controller;
    ss::sharded<cluster::id_allocator_frontend>& _id_allocator_frontend;
    rm_group_proxy* _rm_group_proxy;
    ss::sharded<cluster::rm_partition_frontend>& _rm_partition_frontend;
    int16_t _metadata_dissemination_retries;
    std::chrono::milliseconds _metadata_dissemination_retry_delay_ms;

    ss::future<bool> try_create_tx_topic();

    ss::future<checked<tm_transaction, tx_errc>> get_ongoing_tx(
      ss::shared_ptr<tm_stm>,
      model::producer_identity,
      kafka::transactional_id,
      model::timeout_clock::duration);

    ss::future<cluster::init_tm_tx_reply> dispatch_init_tm_tx(
      model::node_id, kafka::transactional_id, model::timeout_clock::duration);
    ss::future<cluster::init_tm_tx_reply> do_init_tm_tx(
      ss::shard_id, kafka::transactional_id, model::timeout_clock::duration);
    ss::future<cluster::init_tm_tx_reply> do_init_tm_tx(
      ss::shared_ptr<tm_stm>,
      kafka::transactional_id,
      model::timeout_clock::duration);

    ss::future<checked<cluster::tm_transaction, tx_errc>> do_end_txn(
      end_tx_request,
      ss::shared_ptr<cluster::tm_stm>,
      model::timeout_clock::duration,
      ss::lw_shared_ptr<ss::shared_promise<tx_errc>>);
    ss::future<checked<cluster::tm_transaction, tx_errc>> do_abort_tm_tx(
      ss::shared_ptr<cluster::tm_stm>,
      cluster::tm_transaction,
      model::timeout_clock::duration,
      ss::lw_shared_ptr<ss::shared_promise<tx_errc>>);
    ss::future<checked<cluster::tm_transaction, tx_errc>> do_commit_tm_tx(
      ss::shared_ptr<cluster::tm_stm>,
      cluster::tm_transaction,
      model::timeout_clock::duration,
      ss::lw_shared_ptr<ss::shared_promise<tx_errc>>);
    ss::future<tx_errc>
      recommit_tm_tx(tm_transaction, model::timeout_clock::duration);
    ss::future<tx_errc>
      reabort_tm_tx(tm_transaction, model::timeout_clock::duration);

    ss::future<add_paritions_tx_reply> do_add_partition_to_tx(
      ss::shared_ptr<tm_stm>,
      add_paritions_tx_request,
      model::timeout_clock::duration);
    ss::future<add_paritions_tx_reply> do_add_partition_to_tx(
      tm_transaction,
      ss::shared_ptr<tm_stm>,
      add_paritions_tx_request,
      model::timeout_clock::duration);
    ss::future<add_offsets_tx_reply> do_add_offsets_to_tx(
      ss::shared_ptr<tm_stm>,
      add_offsets_tx_request,
      model::timeout_clock::duration);
    friend tx_gateway;
};
} // namespace cluster
