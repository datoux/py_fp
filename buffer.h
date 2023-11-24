/*
Copyright (c) 2023 Daniel Turecek <daniel@turecek.de>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef BUFFER_H
#define BUFFER_H
#include <memory>
#include <memory.h>
#include <vector>

template <class T> class Buffer
{
public:

    Buffer(size_t size = 0)
    {
        if (size)
            mBuff = std::vector<T>(size);
    }

    Buffer(const Buffer<T> &b)
    {
        mBuff.assign(b.mBuff.begin(), b.mBuff.end());
    }

    ~Buffer() {
    }

    Buffer<T> & operator=(const Buffer<T> &b) {
        if (this != &b)
            mBuff.assign(b.mBuff.begin(), b.mBuff.end());
        return *this;
    }

    bool operator==(const Buffer<T> &b) {
        return mBuff.size() == b.size() && memcmp(mBuff.data(), b.data(), byteSize()) == 0;
    }

    void setVal(T val) {
        mBuff.assign(mBuff.size(), val);
    }

    void zero() {
        mBuff.assign(mBuff.size(), 0);
    }

    void reinit(size_t size) {
        if (size == mBuff.size())
            return;

        mBuff.clear();
        mBuff.resize(size);
    }

    void reinit(size_t size, T val) {
        reinit(size);
        setVal(val);
    }

    template<typename U> void assignData(U *data, size_t size) {
        mBuff.assign(data, data + size);
    }

    void clear() {
        mBuff.clear();
    }

public:
    operator T*()                   { return mBuff.data(); }
    T* data()                       { return mBuff.data(); }
    const T* data() const           { return mBuff.data(); }
    size_t size() const             { return mBuff.size(); }
    size_t byteSize() const         { return mBuff.size() * sizeof(T); }
    const T& get(size_t i) const    { return mBuff[i]; }
    void set(size_t i, T val)       { mBuff[i] = val; }
    T& last()                       { return mBuff[mBuff.size() - 1]; }
    bool empty() const              { return mBuff.empty(); }

private:
    std::vector<T> mBuff;
};


#endif // BUFFER_H

