
// ---------------------------------------------------------------------------
// IStore<T> — identical shape to flight_computer::IStore<T>.
// ---------------------------------------------------------------------------

#pragma once

namespace prc {

template <typename T> class IStore {
public:
  virtual ~IStore() = default;

  inline void set(const T &value) { data_ = value; };
  inline const T &get() const { return data_; };
  inline T *get_ref() { return &data_; };

protected:
  T data_;
};

};
