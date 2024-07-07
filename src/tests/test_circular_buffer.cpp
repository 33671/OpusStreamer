#define BOOST_TEST_MODULE CircularBufferTest
#include <boost/test/included/unit_test.hpp>
#include "../inc/circular_buffer.h"
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
    CircularBuffer<int> buffer(3);

    BOOST_CHECK_EQUAL(buffer.size(), 0);
    buffer.push(1);
    BOOST_CHECK_EQUAL(buffer.size(), 1);
    buffer.push(2);
    BOOST_CHECK_EQUAL(buffer.size(), 2);
    buffer.push(3);
    BOOST_CHECK_EQUAL(buffer.size(), 3);

    buffer.pop();
    BOOST_CHECK_EQUAL(buffer.size(), 2);
    buffer.pop();
    BOOST_CHECK_EQUAL(buffer.size(), 1);
    buffer.pop();
    BOOST_CHECK_EQUAL(buffer.size(), 0);
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
