/*
 * Copyright (C) 2020-present ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <seastar/core/gate.hh>
#include "raft/raft.hh"
#include "message/messaging_service_fwd.hh"
#include "utils/UUID.hh"
#include "service/raft/raft_group_registry.hh"

namespace service {

// Scylla-specific implementation of raft RPC module.
//
// Uses `netw::messaging_service` as an underlying implementation for
// actually sending RPC messages.
class raft_rpc : public raft::rpc {
    raft::group_id _group_id;
    raft::server_id _server_id;
    netw::messaging_service& _messaging;
    raft_group_registry& _raft_gr;
    seastar::gate _shutdown_gate;

    raft_group_registry::ticker_type::time_point timeout() {
        return raft_group_registry::ticker_type::clock::now() + raft_group_registry::tick_interval * (raft::ELECTION_TIMEOUT.count() / 2);
    }

public:
    explicit raft_rpc(netw::messaging_service& ms, raft_group_registry& raft_gr, raft::group_id gid, raft::server_id srv_id);

    future<raft::snapshot_reply> send_snapshot(raft::server_id server_id, const raft::install_snapshot& snap, seastar::abort_source& as) override;
    future<> send_append_entries(raft::server_id id, const raft::append_request& append_request) override;
    void send_append_entries_reply(raft::server_id id, const raft::append_reply& reply) override;
    void send_vote_request(raft::server_id id, const raft::vote_request& vote_request) override;
    void send_vote_reply(raft::server_id id, const raft::vote_reply& vote_reply) override;
    void send_timeout_now(raft::server_id id, const raft::timeout_now& timeout_now) override;
    void send_read_quorum(raft::server_id id, const raft::read_quorum& check_quorum) override;
    void send_read_quorum_reply(raft::server_id id, const raft::read_quorum_reply& check_quorum_reply) override;
    future<raft::read_barrier_reply> execute_read_barrier_on_leader(raft::server_id id) override;

    void add_server(raft::server_id id, raft::server_info info) override;
    void remove_server(raft::server_id id) override;
    future<> abort() override;

    // Dispatchers to the `rpc_server` upon receiving an rpc message
    void append_entries(raft::server_id from, raft::append_request append_request);
    void append_entries_reply(raft::server_id from, raft::append_reply reply);
    void request_vote(raft::server_id from, raft::vote_request vote_request);
    void request_vote_reply(raft::server_id from, raft::vote_reply vote_reply);
    void timeout_now_request(raft::server_id from, raft::timeout_now timeout_now);
    void read_quorum_request(raft::server_id from, raft::read_quorum check_quorum);
    void read_quorum_reply(raft::server_id from, raft::read_quorum_reply check_quorum_reply);
    future<raft::read_barrier_reply> execute_read_barrier(raft::server_id);

    future<raft::snapshot_reply> apply_snapshot(raft::server_id from, raft::install_snapshot snp);
};

} // end of namespace service
