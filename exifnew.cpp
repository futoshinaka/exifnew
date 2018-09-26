#include <cstdint>
#include <cstring>
#include <vector>

struct TiffEntry {
    uint16_t id;
    uint16_t type;
    uint32_t count;
    uint32_t data;
};

class TiffTagEntry {
  public:
    TiffTagEntry() = default;
    virtual ~TiffTagEntry();
    virtual size_t GetSize() { return 0; }

  private:
};

template <size_t N> struct TiffLong : public TiffTagEntry {
    static constexpr uint16_t type = 1;
    static constexpr size_t basesize = 4;
    static constexpr size_t count = N;
    int32_t data[N];
    virtual size_t GetSize() override { return sizeof(data[N]); }
};

template <size_t N> struct TiffShort : public TiffTagEntry {
    static constexpr uint16_t type = 2;
    static constexpr size_t basesize = 2;
    static constexpr size_t count = N;
    int16_t data[N];
    virtual size_t GetSize() override { return sizeof(data[N]); }
};

TiffTagEntry::~TiffTagEntry() {}

class TiffIfd : public TiffTagEntry {
  public:
    TiffIfd() = default;
    virtual ~TiffIfd() override;
    virtual void RegistTag() {}
    void WriteTag(void* address);

  private:
    constexpr bool is_tag_offset_write(uint32_t basesize,
                                       uint32_t count) const {
        return (basesize * count > 4) ? true : false;
    }

    bool is_tag_offset_write(const TiffEntry& entry) {
        //書くのがめんどくさいだけ。
        (void)entry;
        return true;
    }

    template <typename T> void regist_tag(const T& data, uint16_t id) {
        dataSize += data.GetSize();
        if (is_tag_offset_write(T::basesize, T::count)) {
            ptr.emplace_back(new T(data)); //データコピー
            base.emplace_back(id, T::type, T::count, 0x0);
        } else {

            //たぶん、あかん。
            // memcpyか。
            // 面倒だが特殊化する？
            base.emplace_back(id, T::type, T::count, data.data);
        }
    }

    template <typename T> void regist_tag(T& data, uint16_t id) {
        dataSize += data.GetSize();
        if (is_tag_offset_write(T::basesize, T::count)) {
            ptr.emplace_back(&data); //ポインタ保持
            base.emplace_back(id, T::type, T::count, 0x0);
        } else {
            //たぶん、あかん。
            // memcpyか。
            // 面倒だが特殊化する？
            base.emplace_back(id, T::type, T::count, data.data);
        }
    }
    uint16_t GetType() { return 3; }

  private:
    virtual size_t GetSize() override { return dataSize; }
    size_t dataSize = 0;
    std::vector<TiffEntry> base;    /// 実際に記載するデータ形式
    std::vector<TiffTagEntry*> ptr; /// データへのポインタ.
};

template <> void TiffIfd::regist_tag<TiffIfd>(TiffIfd& data, uint16_t id) {
    dataSize += data.GetSize();
    ptr.emplace_back(&data); //ポインタ保持
    base.emplace_back(id, data.GetType(), data.GetSize(), 0x0u);
}

TiffIfd::~TiffIfd() = default;

void TiffIfd::WriteTag(void* address) {
    size_t entry_count = base.size();
    size_t offset = entry_count * sizeof(TiffEntry);
    char* wptr = reinterpret_cast<char*>(address);

    for (TiffEntry& elem : base) {
        if (is_tag_offset_write(elem)) {
            elem.data = offset;
            offset += elem.data * elem.count;
            ///ちゃんと計算しないといけない。
        }
        memcpy(reinterpret_cast<void*>(address), reinterpret_cast<void*>(&elem),
               sizeof(elem));
        wptr += sizeof(TiffEntry);
    }

    for (TiffTagEntry* elem : ptr) {
        size_t offsetWriteSize = elem->GetSize();
        if (offsetWriteSize > 0) {
            memcpy(wptr, reinterpret_cast<void*>(elem), offsetWriteSize);
            wptr += offsetWriteSize;
        }
    }
}
