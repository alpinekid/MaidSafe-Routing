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

#include "maidsafe/common/utils.h"

#include "maidsafe/rudp/managed_connections.h"
#include "maidsafe/rudp/return_codes.h"

#include "maidsafe/routing/non_routing_table.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/routing/routing_pb.h"
#include "maidsafe/routing/routing_table.h"
#include "maidsafe/routing/rpcs.h"
#include "maidsafe/routing/utils.h"


namespace maidsafe {

namespace routing {

namespace {

void SendOn(protobuf::Message message,
            rudp::ManagedConnections &rudp,
            RoutingTable &routing_table,
            const NodeId &node_id,
            const Endpoint &endpoint,
            const bool &recursive_retry) {
  Endpoint peer_endpoint = endpoint;
  NodeId peer_node_id = node_id;
  std::string serialised_message(message.SerializeAsString());
  rudp::MessageSentFunctor message_sent_functor;
  if (!recursive_retry) {  //  send only once to direct endpoint
    message_sent_functor = [message, &routing_table, peer_node_id](bool message_sent) {
        if (message_sent)
          LOG(kInfo) << " Message sent, type: " << message.type()
                     << " to "
                     << HexSubstr(peer_node_id.String())
                     << " I am " << HexSubstr(routing_table.kKeys().identity)
                     << " [destination id : "
                     << HexSubstr(message.destination_id())
                     << "]";
        else
          LOG(kError) << " Failed to send message, type: " << message.type()
                      << " to "
                      << HexSubstr(peer_node_id.String())
                      << " I am " << HexSubstr(routing_table.kKeys().identity)
                      << " [destination id : "
                      << HexSubstr(message.destination_id())
                      << "]";
      };

  } else {
    message_sent_functor = [=, &rudp, &routing_table, &peer_endpoint](bool message_sent) {
        if (!message_sent) {
          NodeInfo new_node = routing_table.GetClosestNode(NodeId(message.destination_id()), 1);
          LOG(kError) << " Failed to send message, type: " << message.type()
                      << " to "
                      << HexSubstr(peer_node_id.String())
                      << " I am " << HexSubstr(routing_table.kKeys().identity)
                      << " [ destination id : "
                      << HexSubstr(message.destination_id())
                      << "]"
                      << " Retrying to send to : "
                      << HexSubstr(new_node.node_id.String());
          peer_endpoint = new_node.endpoint;
          rudp.Send(peer_endpoint, serialised_message, message_sent_functor);
        } else {
          LOG(kInfo) << " Message sent, type: " << message.type()
                     << " to "
                     << HexSubstr(peer_node_id.String())
                     << " I am " << HexSubstr(routing_table.kKeys().identity)
                     << " [ destination id : "
                     << HexSubstr(message.destination_id())
                     << "]";
        }
      };
  }
  LOG(kVerbose) << " >>>>>>>>> rudp send message to " << peer_endpoint << " <<<<<<<<<<<<<<<<<<<<";
  rudp.Send(peer_endpoint, serialised_message, message_sent_functor);
}

}  // anonymous namespace


void ProcessSend(protobuf::Message message,
                 rudp::ManagedConnections &rudp,
                 RoutingTable &routing_table,
                 NonRoutingTable &non_routing_table,
                 Endpoint direct_endpoint) {
  std::string signature;
  asymm::Sign(message.data(), routing_table.kKeys().private_key, &signature);
  message.set_signature(signature);

  // Direct endpoint message
  if (!direct_endpoint.address().is_unspecified()) {  // direct endpoint provided
    SendOn(message, rudp, routing_table, NodeId(), direct_endpoint, false);
    return;
  }

  // Normal messages
  if (message.has_destination_id()) {  // message has destination id
    std::vector<NodeInfo>
      non_routing_nodes(non_routing_table.GetNodesInfo(NodeId(message.destination_id())));
    if (!non_routing_nodes.empty()) {  // I have the destination id in my NRT
      LOG(kInfo) <<"I have destination node in my NRT";
      for (auto i : non_routing_nodes) {
        LOG(kVerbose) <<"Sending message to my NRT node with id endpoint : " << i.endpoint;
        SendOn(message, rudp, routing_table, i.node_id, i.endpoint, false);
      }
    } else if (routing_table.Size() > 0) {  //  getting closer nodes from my RT
      NodeInfo closest_node(routing_table.GetClosestNode(NodeId(message.destination_id()), 0));
      SendOn(message, rudp, routing_table, closest_node.node_id, closest_node.endpoint, true);
    } else {
      LOG(kError) << " No Endpoint to send to, Aborting Send!"
                  << " Attempt to send a type : " << message.type() << " message"
                  << " to " << HexSubstr(message.source_id())
                  << " From " << HexSubstr(routing_table.kKeys().identity);
    }
    return;
  }

  // Relay message responses only
  if (message.has_relay_id() && (IsResponse(message)) && message.has_relay()) {
    direct_endpoint = GetEndpointFromProtobuf(message.relay());
    message.set_destination_id(message.relay_id());  // so that peer identifies it as direct msg
    SendOn(message, rudp, routing_table, NodeId(message.relay_id()), direct_endpoint, false);
  }
}

}  // namespace routing

}  // namespace maidsafe
