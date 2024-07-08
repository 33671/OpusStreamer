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
    // OpusFrame()
    // {
    //     printf("here");
    //     this->data = std::make_shared<std::vector<uint8_t>>(255);
    // }
    OpusFrame(const size_t size)
    {
        this->data = std::make_shared<std::vector<uint8_t>>(size);
    }
    OpusFrame(std::initializer_list<uint8_t> initList)
        : data(std::make_shared<std::vector<uint8_t>>(initList))
    {
    }
    uint8_t* data_ptr()
    {
        return data->data();
    }

    size_t size()
    {
        return data->size();
    }
    void resize(const size_t _Newsize)
    {
        data->resize(_Newsize);
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