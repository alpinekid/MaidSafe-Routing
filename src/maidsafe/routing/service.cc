/*******************************************************************************
 *  Copyright 2012 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ******************************************************************************/

#include "maidsafe/routing/service.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/routing/node_id.h"
#include "maidsafe/routing/routing.pb.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/rpcs.h"
#include "maidsafe/routing/log.h"

namespace maidsafe {

namespace routing {

Service::Service(std::shared_ptr<RoutingTable> routing_table,
           std::shared_ptr<transport::ManagedConnection> transport)
    : routing_table_(routing_table), transport_(transport) {}

void Service::Ping(protobuf::Message &message) {
  if (message.destination_id() != routing_table_->kKeys().identity)
    return;  // not for us and we should not pass it on.
  protobuf::PingResponse ping_response;
  protobuf::PingRequest ping_request;

  if (!ping_request.ParseFromString(message.data()))
    return;
  ping_response.set_pong(true);
  ping_response.set_original_request(message.data());
  ping_response.set_original_signature(message.signature());
  ping_response.set_timestamp(GetTimeStamp());
  message.set_data(ping_response.SerializeAsString());
  message.set_destination_id(message.source_id());
  message.set_source_id(routing_table_->kKeys().identity);
  routing_table_->SendOn(message);
}

void Service::Connect(protobuf::Message &message) {
  if (message.destination_id() != routing_table_->kKeys().identity)
    return;  // not for us and we should not pass it on.
  protobuf::ConnectRequest connect_request;
  if (!connect_request.ParseFromString(message.data()))
    return;  // no need to reply

  if (connect_request.bootstrap()) {
             // Already connected
             return;  // FIXME
  }
  if (connect_request.client()) {
                 // connect here !!
                 // add to client bucket
  }
  NodeInfo node;
  node.node_id = NodeId(connect_request.contact().node_id());
  if (routing_table_->CheckNode(node)) {
    return; // no need to reply
  }
  // OK we will try to connect to all the endpoints supplied
 // for (int i = connect_request.contact()
  //transport_->AddConnection(transport::Endpoint(connect_request.contact().endpoint().ip(), connect_request.contact().endpoint().port()));
}

void Service::FindNodes(protobuf::Message &message) {
  protobuf::FindNodesRequest find_nodes;
  protobuf::FindNodesResponse found_nodes;
  std::vector<NodeId>
        nodes(routing_table_->GetClosestNodes(NodeId(message.destination_id()),
                 static_cast<uint16_t>(find_nodes.num_nodes_requested())));

  for (auto it = nodes.begin(); it != nodes.end(); ++it)
    found_nodes.add_nodes((*it).String());
  if (routing_table_->Size() < routing_table_->ClosestNodesSize())
    found_nodes.add_nodes(routing_table_->kKeys().identity); // small network send our ID
  found_nodes.set_original_request(message.data());
  found_nodes.set_original_signature(message.signature());
  found_nodes.set_timestamp(GetTimeStamp());
  message.set_destination_id(message.source_id());
  message.set_source_id(routing_table_->kKeys().identity);
  message.set_data(found_nodes.SerializeAsString());
  message.set_direct(true);
  message.set_response(true);
  message.set_replication(1);
  message.set_type(1);
  routing_table_->SendOn(message);
}

}  // namespace routing

}  // namespace maidsafe
