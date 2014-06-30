#ifndef emdrListener_HEADER
#define emdrListener_HEADER

#include "orderStore.h"
#include <zmq.hpp>

class EmdrListener {
    private :
        pthread_t thread;
        zmq::context_t context;
        zmq::socket_t socket;
        OrderStore * orderStore;

    public :
        EmdrListener (OrderStore * orderStore);

        static void * start (EmdrListener * emdrListener);
        void listenLoop();
        void run ();
};


#endif