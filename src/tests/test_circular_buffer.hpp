#define BOOST_TEST_MODULE CircularBufferTest
#include <boost/test/included/unit_test.hpp>
#include "../circular_buffer.hpp"
#include "../inc/opus_frame.hpp"
BOOST_AUTO_TEST_CASE(test_push_pop) {
    CircularBuffer<int> buffer(3);

    buffer.push(1);
    buffer.push(2);
    buffer.push(3);
    BOOST_CHECK(buffer.is_full());
    BOOST_CHECK_EQUAL(buffer.pop(), 1);
    BOOST_CHECK_EQUAL(buffer.pop(), 2);
    BOOST_CHECK_EQUAL(buffer.pop(), 3);
    BOOST_CHECK(buffer.is_empty());
}

BOOST_AUTO_TEST_CASE(test_push_no_wait) {
    CircularBuffer<int> buffer(3);

    buffer.push(1);
    buffer.push(2);
    BOOST_CHECK(!buffer.is_full()); 
    buffer.push(3);
    buffer.push_no_wait(4); 
    BOOST_CHECK(buffer.is_full()); 
     buffer.push_no_wait(5); 

    BOOST_CHECK(buffer.is_full()); 
    BOOST_CHECK_EQUAL(buffer.pop(), 3);
    BOOST_CHECK_EQUAL(buffer.pop(), 4);
    BOOST_CHECK_EQUAL(buffer.pop(), 5);
}

BOOST_AUTO_TEST_CASE(test_size) {
    CircularBuffer<OpusFrame> buffer(3,OpusFrame(3));

    BOOST_CHECK_EQUAL(buffer.queue_size(), 0);
    buffer.push(OpusFrame(3));
    BOOST_CHECK_EQUAL(buffer.queue_size(), 1);
    buffer.push(OpusFrame(2));
    BOOST_CHECK_EQUAL(buffer.queue_size(), 2);
    buffer.push(OpusFrame(100));
    BOOST_CHECK_EQUAL(buffer.queue_size(), 3);

    buffer.pop();
    BOOST_CHECK_EQUAL(buffer.queue_size(), 2);
    buffer.pop();
    BOOST_CHECK_EQUAL(buffer.queue_size(), 1);
    auto item =buffer.pop();
    BOOST_CHECK_EQUAL(item.size(), 100);
}

BOOST_AUTO_TEST_CASE(test_is_empty_is_full) {
    CircularBuffer<int> buffer(3);

    BOOST_CHECK(buffer.is_empty());
    BOOST_CHECK(!buffer.is_full());

    buffer.push(1);
    BOOST_CHECK(!buffer.is_empty());
    BOOST_CHECK(!buffer.is_full());

    buffer.push(2);
    buffer.push(3);
    BOOST_CHECK(!buffer.is_empty());
    BOOST_CHECK(buffer.is_full());

    buffer.pop();
    BOOST_CHECK(!buffer.is_empty());
    BOOST_CHECK(!buffer.is_full());

    buffer.pop();
    buffer.pop();
    BOOST_CHECK(buffer.is_empty());
    BOOST_CHECK(!buffer.is_full());
}
