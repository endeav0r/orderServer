#include "emdrListener.h"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <jansson.h>
#include <pthread.h>
#include <zlib.h>

#define CHUNK (1<<18)

EmdrListener :: EmdrListener (OrderStore * orderStore)
    : context (zmq::context_t(1)),
      socket (zmq::socket_t(context, ZMQ_SUB)) {

    this->orderStore = orderStore;

    socket.connect("tcp://relay-us-central-1.eve-emdr.com:8050");
    socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
}


uint32_t parseISO8601 (const char * iso8601string) {
    struct tm t;
    t.tm_wday = 0;
    t.tm_yday = 0;
    t.tm_isdst = 0;

    int offsetHour;
    int offsetMinute;

    sscanf(iso8601string,
          "%d-%d-%dT%d:%d:%d+%d:%d",
          &(t.tm_year),
          &(t.tm_mon),
          &(t.tm_mday),
          &(t.tm_hour),
          &(t.tm_min),
          &(t.tm_sec),
          &offsetHour,
          &offsetMinute);

    time_t timestamp = mktime(&t);
    timestamp += (offsetHour * 60 * 60);
    timestamp += (offsetMinute * 60);

    return timestamp;
}


void EmdrListener :: listenLoop () {
    unsigned char * zlibOut = new unsigned char[CHUNK];

    while (true) {
        // receive zmq message
        zmq::message_t message;
        socket.recv(&message);


        // deflate the message
        unsigned char zlibOut[CHUNK];
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;

        strm.next_in   = Z_NULL;
        strm.avail_in  = 0;

        int ret = inflateInit(&strm);

        if (ret != Z_OK) {
            std::cerr << "inflateInit error " << ret << std::endl;
            break;
        }

        std::string json_text;
        bool error;

        unsigned char * data = (unsigned char *) message.data();
        size_t data_size = message.size();

        size_t bytesDecompressed = 0;
        do {
            strm.avail_in  = data_size - bytesDecompressed;
            strm.next_in   = &(data[bytesDecompressed]);

            do {
                strm.avail_out = CHUNK;
                strm.next_out  = zlibOut;

                error = true;

                ret = inflate(&strm, Z_NO_FLUSH);
                switch (ret) {
                case Z_NEED_DICT :
                    ret = Z_DATA_ERROR;
                case Z_DATA_ERROR :
                    std::cerr << "zlib Z_DATA_ERROR" << std::endl;
                    break;
                case Z_MEM_ERROR :
                    std::cerr << "zlib Z_MEM_ERROR" << std::endl;
                    break;
                default :
                    error = false;
                    break;
                }
                json_text += std::string((char *) zlibOut, CHUNK - strm.avail_out);

                if (error)
                    break;

            } while (strm.avail_out == 0);

            bytesDecompressed += CHUNK - strm.avail_out;

            if (error)
                break;
        } while (ret != Z_STREAM_END);

        // bad deflation, skip json
        if (error)
            continue;

        // start decoding the json
        json_error_t json_error;
        json_t * root = json_loads(json_text.c_str(), 0, &json_error);

        if (root == NULL) {
            std::cerr << "--------------------------------" << std::endl;
            std::cerr << "json error:     " << json_error.text << std::endl;
            std::cerr << "error source:   " << json_error.source << std::endl;
            std::cerr << "error line:     " << json_error.line << std::endl;
            std::cerr << "error column:   " << json_error.column << std::endl;
            std::cerr << "error position: " << json_error.position << std::endl;
            continue;
        }

        json_t * j_resultType = json_object_get(root, "resultType");
        if (strcmp("orders", json_string_value(j_resultType)) == 0) {
            json_t * j_columns = json_object_get(root, "columns");

            // get emdr key
            unsigned char emdr[16];
            memset(emdr, 0, 16);

            json_t * j_uploadKeys = json_object_get(root, "uploadKeys");
            for (size_t key_i = 0; key_i < json_array_size(j_uploadKeys); key_i++) {
                json_t * j_uploadKey = json_array_get(j_uploadKeys, key_i);
                json_t * j_uploadKeyName = json_object_get(j_uploadKey, "name");
                const char * uploadKeyName = json_string_value(j_uploadKeyName);
                if (strcmp(uploadKeyName, "emdr") == 0) {
                    json_t * j_uploadKeyKey = json_object_get(j_uploadKey, "key");
                    const char * emdrString = json_string_value(j_uploadKeyKey);

                    for (size_t ei = 0; ei < 16; ei++) {
                        char high = emdrString[ei*2];
                        char low  = emdrString[ei*2+1];

                        if ((high >= '0') && (high <= '9'))
                            high -= '0';
                        else if ((high >= 'a') && (high <= 'f'))
                            high -= 'a';
                        else
                            high = 0;

                        if ((low >= '0') && (low <= '9'))
                            low -= '0';
                        else if ((low >= 'a') && (low <= 'f'))
                            low -= 'a';
                        else
                            low = 0;

                        high <<= 4;

                        emdr[ei] = high | low;
                    }

                    break;
                }
            }

            // get location of order columns
            int pos_price = -1,   pos_volRemaining = -1, pos_range = -1,
                pos_orderID = -1, pos_volEntered = -1,   pos_minVolume = -1,
                pos_bid = -1,     pos_issueDate = -1,    pos_duration = -1,
                pos_stationID = -1;

            for (size_t i = 0; i < json_array_size(j_columns); i++) {
                json_t * j_columnName = json_array_get(j_columns, i);
                const char * columnName = json_string_value(j_columnName);

                if (strcmp(columnName, "price") == 0)
                    pos_price = i;
                else if (strcmp(columnName, "volRemaining") == 0)
                    pos_volRemaining = i;
                else if (strcmp(columnName, "range") == 0)
                    pos_range = i;
                else if (strcmp(columnName, "orderID") == 0)
                    pos_orderID = i;
                else if (strcmp(columnName, "volEntered") == 0)
                    pos_volEntered = i;
                else if (strcmp(columnName, "minVolume") == 0)
                    pos_minVolume = i;
                else if (strcmp(columnName, "bid") == 0)
                    pos_bid = i;
                else if (strcmp(columnName, "issueDate") == 0)
                    pos_issueDate = i;
                else if (strcmp(columnName, "duraton") == 0)
                    pos_duration = i;
                else if (strcmp(columnName, "stationID") == 0)
                    pos_stationID = i;
            }

            // grab that currentTime
            json_t * j_currentTime = json_object_get(root, "currentTime");
            uint32_t currentTime = json_integer_value(j_currentTime);

            // create all of the orders from rowsets
            std::list <Order *> orders;

            json_t * j_rowsets = json_object_get(root, "rowsets");
            for (size_t rowset_i = 0; rowset_i < json_array_size(j_rowsets); rowset_i++) {
                json_t * j_rowset = json_array_get(j_rowsets, rowset_i);
                json_t * j_typeID = json_object_get(j_rowset, "typeID");
                json_t * j_generatedAt = json_object_get(j_rowset, "generatedAt");
                json_t * j_rows   = json_object_get(j_rowset, "rows");

                uint32_t typeID = json_integer_value(j_typeID);
                uint32_t generatedAt = parseISO8601(json_string_value(j_generatedAt));

                for (size_t row_i = 0; row_i < json_array_size(j_rows); row_i++) {
                    json_t * j_row = json_array_get(j_rows, row_i);

                    json_t * j_price = json_array_get(j_row, pos_price);
                    json_t * j_volRemaining = json_array_get(j_row, pos_volRemaining);
                    json_t * j_range = json_array_get(j_row, pos_range);
                    json_t * j_orderID = json_array_get(j_row, pos_orderID);
                    json_t * j_volEntered = json_array_get(j_row, pos_volEntered);
                    json_t * j_minVolume = json_array_get(j_row, pos_minVolume);
                    json_t * j_bid = json_array_get(j_row, pos_bid);
                    json_t * j_issueDate = json_array_get(j_row, pos_issueDate);
                    json_t * j_duration = json_array_get(j_row, pos_duration);
                    json_t * j_stationID = json_array_get(j_row, pos_stationID);

                    double price = json_real_value(j_price);
                    uint64_t volRemaining = json_integer_value(j_volRemaining);
                    int range = json_integer_value(j_range);
                    uint64_t orderID = json_integer_value(j_orderID);
                    uint64_t volEntered = json_integer_value(j_volEntered);
                    uint64_t minVolume = json_integer_value(j_minVolume);
                    bool bid = json_is_true(j_bid) ? true : false;
                    uint32_t issueDate = parseISO8601(json_string_value(j_issueDate));
                    uint32_t duration = json_integer_value(j_duration);
                    uint32_t stationID = json_integer_value(j_stationID);

                    Order * order = new Order (emdr, typeID, price, volRemaining,
                                               range, orderID, volEntered, minVolume,
                                               bid, issueDate, duration, stationID,
                                               generatedAt, currentTime);
                    orders.push_back(order);
                }
            }

            // put these orders in the orderStore
            orderStore->absorbOrders(orders);
        }

        json_decref(root);

    }

    delete[] zlibOut;
}


void * EmdrListener :: start (EmdrListener * emdrListener) {
    emdrListener->listenLoop();
    return NULL;
}


void EmdrListener :: run () {
    pthread_create(&thread, NULL, (void * (*) (void *)) EmdrListener::start, this);
}