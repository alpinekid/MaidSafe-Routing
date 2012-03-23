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

#ifndef MAIDSAFE_ROUTING_SERVICE_H_
#define MAIDSAFE_ROUTING_SERVICE_H_

#include <memory>
#include "maidsafe/transport/managed_connection.h"

namespace maidsafe {

namespace routing {

namespace protobuf { class Message; }
class Routing;
class Rpcs;
class RoutingTable;

// Handle all incoming requests and send back reply
class Service {
 public:
  Service(std::shared_ptr<RoutingTable> routing_table,
           std::shared_ptr<transport::ManagedConnection> transport);
  void Ping(protobuf::Message &message);
  void Connect(protobuf::Message &message);
  void FindNodes(protobuf::Message &message);
 private:
  std::shared_ptr<RoutingTable> routing_table_;
  std::shared_ptr<transport::ManagedConnection> transport_;

};

}  // namespace routing

}  // namespace maidsafe


#endif  // MAIDSAFE_ROUTING_SERVICE_H_
