#include <iostream>
#include <unistd.h>

#include "emdrListener.h"
#include "evePwnServer.h"
#include "orderStore.h"


int main (int argc, char * argv[]) {
    OrderStore * orderStore = new OrderStore();
    EmdrListener emdrListener(orderStore);
    EvePwnServer evePwnServer(orderStore);

    emdrListener.run();
    evePwnServer.run();

    while (true) {
        usleep(60*1000000);
        std::cout << "orderStore->size() = " << orderStore->size() << std::endl;
    }

    return 0;
}