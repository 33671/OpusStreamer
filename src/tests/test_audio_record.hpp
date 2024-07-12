#include <chrono>
#include <thread>
#include <boost/test/included/unit_test.hpp>
#include "../inc/audio_recorder.hpp"
BOOST_AUTO_TEST_CASE(audio_record)
{
    auto buffer = std::make_shared<CircularBufferBroadcast<std::optional<OpusFrame>>>(5000 / 20);
    auto test_sub = buffer->subscribe();
    AudioRecorder recorder(buffer);
    auto start = std::chrono::steady_clock::now();
    BOOST_CHECK(recorder.start());

    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    recorder.stop();
    auto end = std::chrono::steady_clock::now();
    int total_size = 1;
    while (true) {
        // printf("here:%zu\n",test_sub->queue_size());
        auto result = test_sub->pop();
        if (result.has_value()) {
            total_size++;
        } else {
            break;
        }
    }

    auto elapsed = duration_cast<std::chrono::milliseconds>(end - start).count();
    BOOST_CHECK((total_size * 20 < elapsed) && total_size * 20 > 2000);

}