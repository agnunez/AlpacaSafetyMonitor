#pragma once
#include <cstddef>
#include <cmath>

template<typename T, size_t N>
class CircularBuffer {
public:
    void add(T value) {
        _data[_index] = value;
        _index = (_index + 1) % N;
        if (_count < N) _count++;
    }

    T average() const {
        if (_count == 0) return T{};
        T sum = T{};
        for (size_t i = 0; i < _count; i++) sum += _data[i];
        return sum / static_cast<T>(_count);
    }

    T rms() const {
        if (_count == 0) return T{};
        T sum = T{};
        for (size_t i = 0; i < _count; i++) sum += _data[i] * _data[i];
        return std::sqrt(sum / static_cast<T>(_count));
    }

    T newest() const {
        if (_count == 0) return T{};
        return _data[(_index + N - 1) % N];
    }

    T oldest() const {
        if (_count == 0) return T{};
        return _data[(_index + N - _count) % N];
    }

    // Access element by age index: at(0)=oldest, at(count-1)=newest
    T at(size_t i) const {
        if (i >= _count) return T{};
        return _data[(_index + N - _count + i) % N];
    }

    size_t count() const { return _count; }
    size_t capacity() const { return N; }
    bool full() const { return _count == N; }

    void clear() {
        _index = 0;
        _count = 0;
    }

private:
    T _data[N] = {};
    size_t _index = 0;
    size_t _count = 0;
};
