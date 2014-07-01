#include "evePwnServer.h"

#include <msgpack.h>
#include <iostream>

EvePwnServer :: EvePwnServer (OrderStore * orderStore)
    : context (zmq::context_t(1)),
      socket (zmq::socket_t(context, ZMQ_REP)) {

    this->orderStore = orderStore;

    socket.bind("tcp://*:38279");
}


void EvePwnServer :: sendBuffer (msgpack::sbuffer & buffer) {
    zmq::message_t response (buffer.size());
    memcpy(response.data(), buffer.data(), response.size());
    socket.send(response);
}


void * EvePwnServer :: listenLoop () {
    while (true) {
        // receive zmq message
        zmq::message_t request;
        socket.recv(&request);

        msgpack::unpacked msg;
        msgpack::unpack(&msg, (const char *) request.data(), request.size());

        msgpack::object obj = msg.get();

        std::map <std::string, msgpack::object> umap;
        obj.convert(&umap);

        if (umap.count("action") == 0)
            continue;

        std::string action;
        uint32_t id;

        umap["action"].convert(&action);

        std::cout << "action=" << action << std::endl;

        if (action == "orders.typeID") {
            if (umap.count("typeID") == 0)
                continue;
            umap["typeID"].convert(&id);

            msgpack::sbuffer buffer;
            msgpack::packer<msgpack::sbuffer> pk(&buffer);

            pk.pack_map(3);
            pk.pack(std::string("typeID"));
            pk.pack_uint32(id);
            pk.pack(std::string("action"));
            pk.pack(std::string("orders.typeID"));
            pk.pack(std::string("orders"));

            orderStore->packTypeID(id, pk);

            std::cout << "orders.typeID id=" << id << " size=" << buffer.size() << std::endl;

            sendBuffer(buffer);
        }
        else if (action == "orders.typeIDs") {
            if (umap.count("typeIDs") == 0)
                continue;
            std::vector <uint32_t> typeIDs;
            umap["typeIDs"].convert(&typeIDs);

            msgpack::sbuffer buffer;
            msgpack::packer<msgpack::sbuffer> pk(&buffer);

            pk.pack_map(2);
            pk.pack(std::string("action"));
            pk.pack(std::string("orders.typeIDs"));
            pk.pack(std::string("orders"));
            pk.pack_map(typeIDs.size());

            std::vector <uint32_t> :: iterator it;
            for (it = typeIDs.begin(); it != typeIDs.end(); it++) {
                pk.pack_uint32(*it);
                orderStore->packTypeID(*it, pk);
            }

            std::cout << "orders.typeIDs size=" << buffer.size() << std::endl;

            sendBuffer(buffer);
        }
        else if (action == "orders.stationID") {
            if (umap.count("stationID") == 0)
                continue;
            umap["stationID"].convert(&id);

            msgpack::sbuffer buffer;
            msgpack::packer<msgpack::sbuffer> pk(&buffer);

            pk.pack_map(3);
            pk.pack(std::string("stationID"));
            pk.pack_uint32(id);
            pk.pack(std::string("action"));
            pk.pack(std::string("orders.stationID"));
            pk.pack(std::string("orders"));

            orderStore->packStationID(id, pk);

            std::cout << "orders.stationID id=" << id << " size=" << buffer.size() << std::endl;

            sendBuffer(buffer);
        }
    }

    return NULL;
}


void * EvePwnServer :: start (EvePwnServer * evePwnServer) {
    evePwnServer->listenLoop();
    return NULL;
}


void EvePwnServer :: run () {
    pthread_create(&thread, NULL, (void * (*) (void *)) EvePwnServer::start, this);
}