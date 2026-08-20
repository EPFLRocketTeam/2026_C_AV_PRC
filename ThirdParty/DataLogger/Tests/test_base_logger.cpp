
#include <gtest/gtest.h>

#include "ThirdParty/DataLogger/base.hpp"
#include "ThirdParty/DataLogger/Tests/test_helper.hpp"

enum MyLoggerKinds {
    LOG_HEALTH,
    LOG_INTEGER,
    LOG_DOUBLE,
    LOG_2048B
};

struct MyLogger : public BaseDataLogger<0xAE, MyLoggerKinds, SampleTestStorage> {
public:
    MyLogger () = default;
    MyLogger (SampleTestStorage& storage) : BaseDataLogger(storage) {}

    void logInteger (int value) {
        writeRecord(LOG_INTEGER, &value, sizeof(int));
    }
    void logDouble (double value) {
        writeRecord(LOG_DOUBLE, &value, sizeof(double));
    }

    void logSplitInTransaction () {
        uint8_t buffer[2048];
        for (size_t offset = 0; offset < 2048; offset ++) {
            buffer[offset] = (offset ^ (offset << 4)) * 0b1011 + 24;
        }

        writeRecord(LOG_2048B, buffer, 2048);
    }
};

TEST(TestBaseDataLogger, TestTransactionSimpleWrite) {
    MyLogger logger;
    
    g_now_us_ = 21;
    logger.logInteger(42);
    g_now_us_ = 0x11256032;
    logger.logDouble(3.14L);

    SampleTestStorage& storage = logger.getStorage();
    EXPECT_EQ(storage.str().size(), (size_t) 0);
    logger.tick();
    EXPECT_EQ(storage.str().size(), 2 * sizeof(LogHeader) + 4 + 8);

    uint8_t test_array[28] = {
        0xAE, 1, 4, 0, 21, 0, 0, 0,
        42, 0, 0, 0,
        0xAE, 2, 8, 0, 0x32, 0x60, 0x25, 0x11,
        0, 0, 0, 0, 0, 0, 0, 0 // copy 3.14L here
    };
    double db = 3.14L;
    memcpy(test_array + 20, &db, 8);

    std::string str = storage.str();
    for (size_t off = 0; off < sizeof(test_array); off ++) {
        EXPECT_EQ((uint8_t) str[off], test_array[off])
            << "At pos " << off;
    }
}

TEST(TestBaseDataLogger, TestWithFailure) {
    MyLogger logger;
    SuccessQueue queue;
    queue.push(true);
    queue.push(false);
    logger.getStorage() = SampleTestStorage(
        true,
        queue
    );
    
    g_now_us_ = 21;
    logger.logInteger(42);
    g_now_us_ = 0x11256032;
    logger.logDouble(3.14L);
    logger.tick();

    SampleTestStorage& storage = logger.getStorage();
    EXPECT_EQ(storage.str().size(), 12);

    uint8_t test_array[12] = {
        0xAE, 1, 4, 0, 21, 0, 0, 0,
        42, 0, 0, 0,
    };
    std::string str = storage.str();
    for (size_t off = 0; off < sizeof(test_array); off ++) {
        EXPECT_EQ((uint8_t) str[off], test_array[off])
            << "At pos " << off;
    }
}

TEST(TestBaseDataLogger, TestInTransaction) {
    MyLogger logger;
    
    g_now_us_ = 21;
    logger.logSplitInTransaction();
    logger.tick();

    SampleTestStorage& storage = logger.getStorage();
    EXPECT_EQ(storage.str().size(), 2056);
    
    uint8_t test_array[8] = {
        0xAE, 3, 0, 8, 21, 0, 0, 0,
    };
    std::string str = storage.str();
    for (size_t off = 0; off < sizeof(test_array); off ++) {
        EXPECT_EQ((uint8_t) str[off], test_array[off])
            << "At pos " << off;
    }
    for (size_t offset = 0; offset < 16; offset ++) {
        EXPECT_EQ((uint8_t) str[offset + 8], ((offset ^ (offset << 4)) * 0b1011 + 24) & 0xFF)
            << "At pos " << (offset + 8);
    }
}

TEST(TestBaseDataLogger, TestTransactionFailure) {
    for (bool bHeader : { false, true }) {
        for (bool bPayload : { false, true }) {
            if (bHeader && bPayload) continue ;

            MyLogger logger;
            SuccessQueue queue;
            queue.push(bHeader);
            queue.push(bPayload);
            logger.getStorage() = SampleTestStorage(
                true,
                queue
            );
            
            logger.logSplitInTransaction();
            logger.tick();

            SampleTestStorage& storage = logger.getStorage();
            EXPECT_EQ(storage.str().size(), 0);
        }
    }
}

#define RUN_TEST(SZE, ...) { \
    uint8_t test_array[SZE] = __VA_ARGS__; \
    EXPECT_EQ(storage.str().size(), SZE); \
    std::string str = storage.str(); \
    for (size_t off = 0; off < SZE; off ++) { \
        EXPECT_EQ((uint8_t) str[off], test_array[off]) \
            << "At pos " << off; \
    } \
    storage.clear(); \
}


TEST(TestBaseDataLogger, TestHealth) {
    MyLogger logger;
    SuccessQueue queue;
    queue.push(true);  // health
    queue.push(false); // int
    queue.push(true);  // health
    queue.push(true);  // int
    queue.push(true);  // double
    queue.push(true);  // health
    queue.push(true);  // 2048b - header
    queue.push(true);  // 2048b - payload
    queue.push(true);  // health
    logger.getStorage() = SampleTestStorage(
        true,
        queue
    );
    
    SampleTestStorage& storage = logger.getStorage();

    g_now_us_ = 0; g_next_us_ = 2;
    logger.logStorageHealth();
    logger.tick();

    RUN_TEST(28, {
        0xAE, 0, 20, 0, 0, 0, 0, 0,

        0, 0, 0, 0, // bytes_written
        0, 0, 0, 0, // write_count_
        0, 0, 0, 0, // write_fail_count_
        0, 0, 0, 0, // max_write_time_us_
        0, 0, 0, 0  // tick_count_
    })

    g_next_us_ = 4;
    logger.logInteger(0);
    logger.tick();
    
    RUN_TEST(0, {})

    g_next_us_ = 8;
    logger.logStorageHealth();
    logger.tick();

    RUN_TEST(28, {
        0xAE, 0, 20, 0, 4, 0, 0, 0,

        28, 0, 0, 0, // bytes_written
        2, 0, 0, 0,  // write_count_
        1, 0, 0, 0,  // write_fail_count_
        2, 0, 0, 0,  // max_write_time_us_
        2, 0, 0, 0   // tick_count_
    })

    logger.logInteger(0);
    logger.tick();
    logger.logDouble(0.L);
    logger.tick();

    storage.clear();

    logger.logStorageHealth();
    logger.tick();
    
    RUN_TEST(28, {
        0xAE, 0, 20, 0, 8, 0, 0, 0,

        84, 0, 0, 0, // bytes_written
        5, 0, 0, 0,  // write_count_
        1, 0, 0, 0,  // write_fail_count_
        4, 0, 0, 0,  // max_write_time_us_
        5, 0, 0, 0   // tick_count_
    })

    g_next_us_ = 1000;
    logger.logSplitInTransaction();
    logger.tick();
    
    storage.clear();
    
    logger.logStorageHealth();
    logger.tick();
    
    RUN_TEST(28, {
        0xAE, 0, 20, 0, 232, 3, 0, 0,

        120, 8, 0, 0, // bytes_written
        7, 0, 0, 0,  // write_count_
        1, 0, 0, 0,  // write_fail_count_
        224, 3, 0, 0,  // max_write_time_us_
        7, 0, 0, 0   // tick_count_
    })
}
