#ifndef evePwnServer_HEADER
#define evePwnServer_HEADER

#include "orderStore.h"
#include <zmq.hpp>

class EvePwnServer {
    private :
        pthread_t thread;
        zmq::context_t context;
        zmq::socket_t socket;
        OrderStore * orderStore;

        void sendBuffer (msgpack::sbuffer & buffer);

    public :
        EvePwnServer (OrderStore * orderStore);

        static void * start (EvePwnServer * evePwnServer);
        void * listenLoop();
        void run ();
};


#endif