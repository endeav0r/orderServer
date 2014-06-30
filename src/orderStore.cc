#include "orderStore.h"

#include <unistd.h>

void Order :: pack (msgpack::packer <msgpack::sbuffer> & pk) {
    pk.pack_array(14);
    pk.pack_raw(16);
    pk.pack_raw_body((const char *) emdr, 16);
    pk.pack_uint32(typeID);
    pk.pack_double(price);
    pk.pack_uint64(volRemaining);
    pk.pack_int32(range);
    pk.pack_uint64(orderID);
    pk.pack_uint64(volEntered);
    pk.pack_uint64(minVolume);
    if (bid)
        pk.pack_true();
    else
        pk.pack_false();
    pk.pack_uint32(issueDate);
    pk.pack_uint32(duration);
    pk.pack_uint32(stationID);
    pk.pack_uint32(generatedAt);
    pk.pack_uint32(currentTime);
}


OrderStore :: OrderStore () {
    pthread_mutex_init(&readLock, NULL);
    pthread_mutex_init(&writeLock, NULL);
    readCount = 0;
}


OrderStore :: ~OrderStore () {
    while (orders.size() > 0) {
        Order * order = orders.front();
        delete order;
        orders.pop();
    }
}


void OrderStore :: absorbOrder (Order * order) {
    pthread_mutex_lock(&writeLock);

    pthread_mutex_lock(&readLock);
    while (readCount > 0) {
        pthread_mutex_unlock(&readLock);
        usleep(1000);
        pthread_mutex_lock(&readLock);
    }
    pthread_mutex_unlock(&readLock);

    orders.push(order);
    typeIDMap[order->g_typeID()].insert(order);
    stationIDMap[order->g_stationID()].insert(order);

    if (orders.size() > ORDERSTORE_SIZE) {
        order = orders.front();
        typeIDMap[order->g_typeID()].erase(order);
        stationIDMap[order->g_stationID()].erase(order);
        orders.pop();
        delete order;
    }

    pthread_mutex_unlock(&writeLock);
}


void OrderStore :: absorbOrders (std::list <Order *> & orders) {
    pthread_mutex_lock(&writeLock);

    pthread_mutex_lock(&readLock);
    while (readCount > 0) {
        pthread_mutex_unlock(&readLock);
        sleep(1);
        pthread_mutex_lock(&readLock);
    }
    pthread_mutex_unlock(&readLock);

    std::list <Order *> :: iterator it;
    for (it = orders.begin(); it != orders.end(); it++) {
        Order * order = *it;

        this->orders.push(order);
        typeIDMap[order->g_typeID()].insert(order);
        stationIDMap[order->g_stationID()].insert(order);
    }

    while (this->orders.size() > ORDERSTORE_SIZE) {
        Order * order = this->orders.front();
        typeIDMap[order->g_typeID()].erase(order);
        stationIDMap[order->g_stationID()].erase(order);
        this->orders.pop();
        delete order;
    }

    pthread_mutex_unlock(&writeLock);
}


bool OrderStore :: packTypeID (uint32_t typeID, msgpack::sbuffer & buffer) {
    pthread_mutex_lock(&writeLock);
    pthread_mutex_lock(&readLock);
    readCount++;
    pthread_mutex_unlock(&readLock);
    pthread_mutex_unlock(&writeLock);

    msgpack::packer<msgpack::sbuffer> pk(&buffer);

    std::unordered_set <Order *> :: iterator it;

    pk.pack_map(3);
    pk.pack(std::string("typeID"));
    pk.pack_uint32(typeID);
    pk.pack(std::string("action"));
    pk.pack(std::string("orders.typeID"));
    pk.pack(std::string("orders"));
    pk.pack_array(typeIDMap[typeID].size());

    for (it = typeIDMap[typeID].begin(); it != typeIDMap[typeID].end(); it++) {
        (*it)->pack(pk);
    }

    pthread_mutex_lock(&readLock);
    readCount--;
    pthread_mutex_unlock(&readLock);

    return true;
}


bool OrderStore :: packStationID (uint32_t stationID, msgpack::sbuffer & buffer) {
    pthread_mutex_lock(&writeLock);
    pthread_mutex_lock(&readLock);
    readCount++;
    pthread_mutex_unlock(&readLock);
    pthread_mutex_unlock(&writeLock);

    msgpack::packer<msgpack::sbuffer> pk(&buffer);

    std::unordered_set <Order *> :: iterator it;

    pk.pack_map(3);
    pk.pack(std::string("stationID"));
    pk.pack_uint32(stationID);
    pk.pack(std::string("action"));
    pk.pack(std::string("orders.stationID"));
    pk.pack(std::string("orders"));
    pk.pack_array(stationIDMap[stationID].size());

    for (it = stationIDMap[stationID].begin(); it != stationIDMap[stationID].end(); it++) {
        (*it)->pack(pk);
    }

    pthread_mutex_lock(&readLock);
    readCount--;
    pthread_mutex_unlock(&readLock);

    return true;
}