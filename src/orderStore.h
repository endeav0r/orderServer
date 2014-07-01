#ifndef orderStore_HEADER
#define orderStore_HEADER

#include <cstring>
#include <inttypes.h>
#include <list>
#include <msgpack.hpp>
#include <pthread.h>
#include <queue>
#include <unordered_set>

#define ORDERSTORE_SIZE (1 << 20)

class Order {
    private :
        uint8_t  emdr[16];
        uint32_t typeID;
        double price;           // 8 8
        uint64_t volRemaining;  // 8 16
        int range;              // 4 20
        uint64_t orderID;       // 8 28
        uint64_t volEntered;    // 8 36
        uint64_t minVolume;     // 8 44
        bool bid;               // 4 48
        time_t issueDate;       // 4 52
        uint32_t duration;      // 4 56
        uint32_t stationID;     // 4 60
        uint32_t generatedAt;
        uint32_t currentTime;
    public :
        Order (uint8_t * emdr,
               uint32_t typeID,
               double price,
               uint64_t volRemaining,
               int range,
               uint64_t orderID,
               uint64_t volEntered,
               uint64_t minVolume,
               bool bid,
               time_t issueDate,
               uint32_t duration,
               uint32_t stationID,
               uint32_t generatedAt,
               uint32_t currentTime)
            :
                typeID (typeID),
                price (price),
                volRemaining (volRemaining),
                range (range),
                orderID (orderID),
                volEntered (volEntered),
                minVolume (minVolume),
                bid (bid),
                issueDate (issueDate),
                duration (duration),
                stationID (stationID),
                generatedAt (generatedAt),
                currentTime (currentTime) {
                    memcpy(this->emdr, emdr, 16);
                }
        Order () {}

        uint8_t * g_emdr          () { return emdr; }
        uint32_t  g_typeID        () { return typeID; }
        double    g_price         () { return price; }
        uint64_t  g_volRemaining  () { return volRemaining; }
        int       g_range         () { return range; }
        uint64_t  g_orderID       () { return orderID; }
        uint64_t  g_volEntered    () { return volEntered; }
        uint64_t  g_minVolume     () { return minVolume; }
        bool      g_bid           () { return bid; }
        time_t    g_issueDate     () { return issueDate; }
        uint32_t  g_duration      () { return duration; }
        uint32_t  g_stationID     () { return stationID; }
        uint32_t  g_generatedAt   () { return generatedAt; }
        uint32_t  g_currentTime   () { return currentTime; }

        void pack (msgpack::packer <msgpack::sbuffer> & pk);
};


class OrderStore {
    private :
        std::queue <Order *> orders;
        std::map   <uint32_t, std::unordered_set <Order *>> typeIDMap;
        std::map   <uint32_t, std::unordered_set <Order *>> stationIDMap;

        pthread_mutex_t readLock;
        pthread_mutex_t writeLock;
        unsigned int readCount;

    public :
        OrderStore ();
        ~OrderStore ();
        void absorbOrder (Order * order);
        void absorbOrders (std::list <Order *> & orders);
        uint32_t size () { return orders.size(); }
        bool packTypeID    (uint32_t typeID, msgpack::packer <msgpack::sbuffer> & pk);
        bool packStationID (uint32_t stationID, msgpack::packer <msgpack::sbuffer> &pk);
};

#endif