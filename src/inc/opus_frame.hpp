#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <vector>
#ifndef  OPUSFRAME
#define OPUSFRAME
class OpusFrame {
public:
    OpusFrame(const std::shared_ptr<std::vector<uint8_t>>& vec)
        : data(vec)
    {
    }
    OpusFrame()
    {
        data = std::make_shared<std::vector<uint8_t>>(0);
    }
    OpusFrame(const size_t size)
    {
        data = std::make_shared<std::vector<uint8_t>>(size);
    }
    OpusFrame(std::initializer_list<uint8_t> initList)
        : data(std::make_shared<std::vector<uint8_t>>(initList))
    {
    }
    uint8_t* data_ptr()
    {
        return data->data();
    }
    void clear()
    {
        data->clear();
    }
    void push_back(const uint8_t& item)
    {
        data->push_back(item);
    }
    size_t size()
    {
        return data->size();
    }
    void resize(const size_t size)
    {
        data->resize(size);
    }

    const std::vector<uint8_t>& vector() const
    {
        return *data;
    }

    const uint8_t* data_ptr() const
    {
        return data->data();
    }

    void print() const
    {
        for (uint8_t& value : *data) {
            printf("%02x ", value);
        }
        std::cout << std::endl;
    }

private:
    std::shared_ptr<std::vector<uint8_t>> data;
};
#endif