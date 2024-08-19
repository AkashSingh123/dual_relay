#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace rust {
inline namespace cxxbridge1 {
// #include "rust/cxx.h"

#ifndef CXXBRIDGE1_PANIC
#define CXXBRIDGE1_PANIC
template <typename Exception>
void panic [[noreturn]] (const char *msg);
#endif // CXXBRIDGE1_PANIC

struct unsafe_bitcopy_t;

namespace {
template <typename T>
class impl;
} // namespace

class Opaque;

template <typename T>
::std::size_t size_of();
template <typename T>
::std::size_t align_of();

#ifndef CXXBRIDGE1_RUST_STRING
#define CXXBRIDGE1_RUST_STRING
class String final {
public:
  String() noexcept;
  String(const String &) noexcept;
  String(String &&) noexcept;
  ~String() noexcept;

  String(const std::string &);
  String(const char *);
  String(const char *, std::size_t);
  String(const char16_t *);
  String(const char16_t *, std::size_t);

  static String lossy(const std::string &) noexcept;
  static String lossy(const char *) noexcept;
  static String lossy(const char *, std::size_t) noexcept;
  static String lossy(const char16_t *) noexcept;
  static String lossy(const char16_t *, std::size_t) noexcept;

  String &operator=(const String &) &noexcept;
  String &operator=(String &&) &noexcept;

  explicit operator std::string() const;

  const char *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  const char *c_str() noexcept;

  std::size_t capacity() const noexcept;
  void reserve(size_t new_cap) noexcept;

  using iterator = char *;
  iterator begin() noexcept;
  iterator end() noexcept;

  using const_iterator = const char *;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  bool operator==(const String &) const noexcept;
  bool operator!=(const String &) const noexcept;
  bool operator<(const String &) const noexcept;
  bool operator<=(const String &) const noexcept;
  bool operator>(const String &) const noexcept;
  bool operator>=(const String &) const noexcept;

  void swap(String &) noexcept;

  String(unsafe_bitcopy_t, const String &) noexcept;

private:
  struct lossy_t;
  String(lossy_t, const char *, std::size_t) noexcept;
  String(lossy_t, const char16_t *, std::size_t) noexcept;
  friend void swap(String &lhs, String &rhs) noexcept { lhs.swap(rhs); }

  std::array<std::uintptr_t, 3> repr;
};
#endif // CXXBRIDGE1_RUST_STRING

#ifndef CXXBRIDGE1_RUST_SLICE
#define CXXBRIDGE1_RUST_SLICE
namespace detail {
template <bool>
struct copy_assignable_if {};

template <>
struct copy_assignable_if<false> {
  copy_assignable_if() noexcept = default;
  copy_assignable_if(const copy_assignable_if &) noexcept = default;
  copy_assignable_if &operator=(const copy_assignable_if &) &noexcept = delete;
  copy_assignable_if &operator=(copy_assignable_if &&) &noexcept = default;
};
} // namespace detail

template <typename T>
class Slice final
    : private detail::copy_assignable_if<std::is_const<T>::value> {
public:
  using value_type = T;

  Slice() noexcept;
  Slice(T *, std::size_t count) noexcept;

  Slice &operator=(const Slice<T> &) &noexcept = default;
  Slice &operator=(Slice<T> &&) &noexcept = default;

  T *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  T &operator[](std::size_t n) const noexcept;
  T &at(std::size_t n) const;
  T &front() const noexcept;
  T &back() const noexcept;

  Slice(const Slice<T> &) noexcept = default;
  ~Slice() noexcept = default;

  class iterator;
  iterator begin() const noexcept;
  iterator end() const noexcept;

  void swap(Slice &) noexcept;

private:
  class uninit;
  Slice(uninit) noexcept;
  friend impl<Slice>;
  friend void sliceInit(void *, const void *, std::size_t) noexcept;
  friend void *slicePtr(const void *) noexcept;
  friend std::size_t sliceLen(const void *) noexcept;

  std::array<std::uintptr_t, 2> repr;
};

template <typename T>
class Slice<T>::iterator final {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = typename std::add_pointer<T>::type;
  using reference = typename std::add_lvalue_reference<T>::type;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;
  reference operator[](difference_type) const noexcept;

  iterator &operator++() noexcept;
  iterator operator++(int) noexcept;
  iterator &operator--() noexcept;
  iterator operator--(int) noexcept;

  iterator &operator+=(difference_type) noexcept;
  iterator &operator-=(difference_type) noexcept;
  iterator operator+(difference_type) const noexcept;
  iterator operator-(difference_type) const noexcept;
  difference_type operator-(const iterator &) const noexcept;

  bool operator==(const iterator &) const noexcept;
  bool operator!=(const iterator &) const noexcept;
  bool operator<(const iterator &) const noexcept;
  bool operator<=(const iterator &) const noexcept;
  bool operator>(const iterator &) const noexcept;
  bool operator>=(const iterator &) const noexcept;

private:
  friend class Slice;
  void *pos;
  std::size_t stride;
};

template <typename T>
Slice<T>::Slice() noexcept {
  sliceInit(this, reinterpret_cast<void *>(align_of<T>()), 0);
}

template <typename T>
Slice<T>::Slice(T *s, std::size_t count) noexcept {
  assert(s != nullptr || count == 0);
  sliceInit(this,
            s == nullptr && count == 0
                ? reinterpret_cast<void *>(align_of<T>())
                : const_cast<typename std::remove_const<T>::type *>(s),
            count);
}

template <typename T>
T *Slice<T>::data() const noexcept {
  return reinterpret_cast<T *>(slicePtr(this));
}

template <typename T>
std::size_t Slice<T>::size() const noexcept {
  return sliceLen(this);
}

template <typename T>
std::size_t Slice<T>::length() const noexcept {
  return this->size();
}

template <typename T>
bool Slice<T>::empty() const noexcept {
  return this->size() == 0;
}

template <typename T>
T &Slice<T>::operator[](std::size_t n) const noexcept {
  assert(n < this->size());
  auto ptr = static_cast<char *>(slicePtr(this)) + size_of<T>() * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
T &Slice<T>::at(std::size_t n) const {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Slice index out of range");
  }
  return (*this)[n];
}

template <typename T>
T &Slice<T>::front() const noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
T &Slice<T>::back() const noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
typename Slice<T>::iterator::reference
Slice<T>::iterator::operator*() const noexcept {
  return *static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::pointer
Slice<T>::iterator::operator->() const noexcept {
  return static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::reference Slice<T>::iterator::operator[](
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ptr = static_cast<char *>(this->pos) + this->stride * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator++() noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator++(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator--() noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator--(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator+=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator-=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator+(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) + this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator-(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) - this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator::difference_type
Slice<T>::iterator::operator-(const iterator &other) const noexcept {
  auto diff = std::distance(static_cast<char *>(other.pos),
                            static_cast<char *>(this->pos));
  return diff / static_cast<typename Slice<T>::iterator::difference_type>(
                    this->stride);
}

template <typename T>
bool Slice<T>::iterator::operator==(const iterator &other) const noexcept {
  return this->pos == other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator!=(const iterator &other) const noexcept {
  return this->pos != other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<(const iterator &other) const noexcept {
  return this->pos < other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<=(const iterator &other) const noexcept {
  return this->pos <= other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>(const iterator &other) const noexcept {
  return this->pos > other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>=(const iterator &other) const noexcept {
  return this->pos >= other.pos;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::begin() const noexcept {
  iterator it;
  it.pos = slicePtr(this);
  it.stride = size_of<T>();
  return it;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::end() const noexcept {
  iterator it = this->begin();
  it.pos = static_cast<char *>(it.pos) + it.stride * this->size();
  return it;
}

template <typename T>
void Slice<T>::swap(Slice &rhs) noexcept {
  std::swap(*this, rhs);
}
#endif // CXXBRIDGE1_RUST_SLICE

#ifndef CXXBRIDGE1_RUST_BITCOPY_T
#define CXXBRIDGE1_RUST_BITCOPY_T
struct unsafe_bitcopy_t final {
  explicit unsafe_bitcopy_t() = default;
};
#endif // CXXBRIDGE1_RUST_BITCOPY_T

#ifndef CXXBRIDGE1_RUST_VEC
#define CXXBRIDGE1_RUST_VEC
template <typename T>
class Vec final {
public:
  using value_type = T;

  Vec() noexcept;
  Vec(std::initializer_list<T>);
  Vec(const Vec &);
  Vec(Vec &&) noexcept;
  ~Vec() noexcept;

  Vec &operator=(Vec &&) &noexcept;
  Vec &operator=(const Vec &) &;

  std::size_t size() const noexcept;
  bool empty() const noexcept;
  const T *data() const noexcept;
  T *data() noexcept;
  std::size_t capacity() const noexcept;

  const T &operator[](std::size_t n) const noexcept;
  const T &at(std::size_t n) const;
  const T &front() const noexcept;
  const T &back() const noexcept;

  T &operator[](std::size_t n) noexcept;
  T &at(std::size_t n);
  T &front() noexcept;
  T &back() noexcept;

  void reserve(std::size_t new_cap);
  void push_back(const T &value);
  void push_back(T &&value);
  template <typename... Args>
  void emplace_back(Args &&...args);
  void truncate(std::size_t len);
  void clear();

  using iterator = typename Slice<T>::iterator;
  iterator begin() noexcept;
  iterator end() noexcept;

  using const_iterator = typename Slice<const T>::iterator;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  void swap(Vec &) noexcept;

  Vec(unsafe_bitcopy_t, const Vec &) noexcept;

private:
  void reserve_total(std::size_t new_cap) noexcept;
  void set_len(std::size_t len) noexcept;
  void drop() noexcept;

  friend void swap(Vec &lhs, Vec &rhs) noexcept { lhs.swap(rhs); }

  std::array<std::uintptr_t, 3> repr;
};

template <typename T>
Vec<T>::Vec(std::initializer_list<T> init) : Vec{} {
  this->reserve_total(init.size());
  std::move(init.begin(), init.end(), std::back_inserter(*this));
}

template <typename T>
Vec<T>::Vec(const Vec &other) : Vec() {
  this->reserve_total(other.size());
  std::copy(other.begin(), other.end(), std::back_inserter(*this));
}

template <typename T>
Vec<T>::Vec(Vec &&other) noexcept : repr(other.repr) {
  new (&other) Vec();
}

template <typename T>
Vec<T>::~Vec() noexcept {
  this->drop();
}

template <typename T>
Vec<T> &Vec<T>::operator=(Vec &&other) &noexcept {
  this->drop();
  this->repr = other.repr;
  new (&other) Vec();
  return *this;
}

template <typename T>
Vec<T> &Vec<T>::operator=(const Vec &other) & {
  if (this != &other) {
    this->drop();
    new (this) Vec(other);
  }
  return *this;
}

template <typename T>
bool Vec<T>::empty() const noexcept {
  return this->size() == 0;
}

template <typename T>
T *Vec<T>::data() noexcept {
  return const_cast<T *>(const_cast<const Vec<T> *>(this)->data());
}

template <typename T>
const T &Vec<T>::operator[](std::size_t n) const noexcept {
  assert(n < this->size());
  auto data = reinterpret_cast<const char *>(this->data());
  return *reinterpret_cast<const T *>(data + n * size_of<T>());
}

template <typename T>
const T &Vec<T>::at(std::size_t n) const {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Vec index out of range");
  }
  return (*this)[n];
}

template <typename T>
const T &Vec<T>::front() const noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
const T &Vec<T>::back() const noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
T &Vec<T>::operator[](std::size_t n) noexcept {
  assert(n < this->size());
  auto data = reinterpret_cast<char *>(this->data());
  return *reinterpret_cast<T *>(data + n * size_of<T>());
}

template <typename T>
T &Vec<T>::at(std::size_t n) {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Vec index out of range");
  }
  return (*this)[n];
}

template <typename T>
T &Vec<T>::front() noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
T &Vec<T>::back() noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
void Vec<T>::reserve(std::size_t new_cap) {
  this->reserve_total(new_cap);
}

template <typename T>
void Vec<T>::push_back(const T &value) {
  this->emplace_back(value);
}

template <typename T>
void Vec<T>::push_back(T &&value) {
  this->emplace_back(std::move(value));
}

template <typename T>
template <typename... Args>
void Vec<T>::emplace_back(Args &&...args) {
  auto size = this->size();
  this->reserve_total(size + 1);
  ::new (reinterpret_cast<T *>(reinterpret_cast<char *>(this->data()) +
                               size * size_of<T>()))
      T(std::forward<Args>(args)...);
  this->set_len(size + 1);
}

template <typename T>
void Vec<T>::clear() {
  this->truncate(0);
}

template <typename T>
typename Vec<T>::iterator Vec<T>::begin() noexcept {
  return Slice<T>(this->data(), this->size()).begin();
}

template <typename T>
typename Vec<T>::iterator Vec<T>::end() noexcept {
  return Slice<T>(this->data(), this->size()).end();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::begin() const noexcept {
  return this->cbegin();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::end() const noexcept {
  return this->cend();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::cbegin() const noexcept {
  return Slice<const T>(this->data(), this->size()).begin();
}

template <typename T>
typename Vec<T>::const_iterator Vec<T>::cend() const noexcept {
  return Slice<const T>(this->data(), this->size()).end();
}

template <typename T>
void Vec<T>::swap(Vec &rhs) noexcept {
  using std::swap;
  swap(this->repr, rhs.repr);
}

template <typename T>
Vec<T>::Vec(unsafe_bitcopy_t, const Vec &bits) noexcept : repr(bits.repr) {}
#endif // CXXBRIDGE1_RUST_VEC

#ifndef CXXBRIDGE1_IS_COMPLETE
#define CXXBRIDGE1_IS_COMPLETE
namespace detail {
namespace {
template <typename T, typename = std::size_t>
struct is_complete : std::false_type {};
template <typename T>
struct is_complete<T, decltype(sizeof(T))> : std::true_type {};
} // namespace
} // namespace detail
#endif // CXXBRIDGE1_IS_COMPLETE

#ifndef CXXBRIDGE1_LAYOUT
#define CXXBRIDGE1_LAYOUT
class layout {
  template <typename T>
  friend std::size_t size_of();
  template <typename T>
  friend std::size_t align_of();
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return T::layout::size();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return sizeof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      size_of() {
    return do_size_of<T>();
  }
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return T::layout::align();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return alignof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      align_of() {
    return do_align_of<T>();
  }
};

template <typename T>
std::size_t size_of() {
  return layout::size_of<T>();
}

template <typename T>
std::size_t align_of() {
  return layout::align_of<T>();
}
#endif // CXXBRIDGE1_LAYOUT

namespace detail {
template <typename T, typename = void *>
struct operator_new {
  void *operator()(::std::size_t sz) { return ::operator new(sz); }
};

template <typename T>
struct operator_new<T, decltype(T::operator new(sizeof(T)))> {
  void *operator()(::std::size_t sz) { return T::operator new(sz); }
};
} // namespace detail

template <typename T>
union ManuallyDrop {
  T value;
  ManuallyDrop(T &&value) : value(::std::move(value)) {}
  ~ManuallyDrop() {}
};

template <typename T>
union MaybeUninit {
  T value;
  void *operator new(::std::size_t sz) { return detail::operator_new<T>{}(sz); }
  MaybeUninit() {}
  ~MaybeUninit() {}
};
} // namespace cxxbridge1
} // namespace rust

struct ForeignPosition;
struct ForeignCircle;
struct ForeignPolygon;
enum class ForeignGeozoneShape : ::std::uint8_t;
enum class ForeignZoneType : ::std::uint8_t;
struct ForeignOrientation;
struct ForeignGeneric32;
struct ForeignGeneric64;
struct ForeignBattery;
struct ForeignSpeed;
struct ForeignMessage;
struct ForeignSwampMessage;
struct ForeignDynamicMessage;
struct ForeignClientMessage;
struct ForeignNestedGeneric;
namespace shared {
  enum class ForeignMissionCommand : ::std::uint8_t;
  struct ForeignCommandMessage;
  struct ForeignPose;
  struct ForeignTarget;
  struct ForeignDiscoveryMessage;
  enum class ForeignPositionType : ::std::uint8_t;
  struct ForeignWaypoint;
  struct ForeignTrajectoryMessage;
  struct ForeignGeozone;
  struct ForeignGeozoneMessage;
  struct ForeignGeneric;
  struct ForeignFieldValue;
  struct ForeignReadyMessage;
  struct ForeignMissionInfoEnvelope;
  enum class InnerSensorMessage : ::std::uint8_t;
}

namespace shared {
#ifndef CXXBRIDGE1_ENUM_shared$ForeignMissionCommand
#define CXXBRIDGE1_ENUM_shared$ForeignMissionCommand
enum class ForeignMissionCommand : ::std::uint8_t {
  Start = 1,
  Pause = 2,
  Land_home = 3,
  Emergency_land = 4,
  Mode_11 = 5,
  Unknown = 0,
};
#endif // CXXBRIDGE1_ENUM_shared$ForeignMissionCommand

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignCommandMessage
#define CXXBRIDGE1_STRUCT_shared$ForeignCommandMessage
struct ForeignCommandMessage final {
  ::std::uint8_t command_type;
  ::std::uint8_t operator_id;
  ::rust::Vec<::std::uint8_t> participants;
  ::std::uint8_t mission_command;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignCommandMessage

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignPose
#define CXXBRIDGE1_STRUCT_shared$ForeignPose
struct ForeignPose final {
  float latitude;
  float longitude;
  float altitude;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignPose

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignTarget
#define CXXBRIDGE1_STRUCT_shared$ForeignTarget
struct ForeignTarget final {
  ::std::uint16_t target_id;
  bool active;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignTarget

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignDiscoveryMessage
#define CXXBRIDGE1_STRUCT_shared$ForeignDiscoveryMessage
struct ForeignDiscoveryMessage final {
  ::std::uint16_t participant_id;
  ::rust::String ip_address;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignDiscoveryMessage

#ifndef CXXBRIDGE1_ENUM_shared$ForeignPositionType
#define CXXBRIDGE1_ENUM_shared$ForeignPositionType
enum class ForeignPositionType : ::std::uint8_t {
  Global = 0,
  Local = 1,
  Target = 2,
};
#endif // CXXBRIDGE1_ENUM_shared$ForeignPositionType
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignPosition
#define CXXBRIDGE1_STRUCT_ForeignPosition
struct ForeignPosition final {
  ::shared::ForeignPositionType pt;
  double x;
  double y;
  double z;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignPosition

namespace shared {
#ifndef CXXBRIDGE1_STRUCT_shared$ForeignWaypoint
#define CXXBRIDGE1_STRUCT_shared$ForeignWaypoint
struct ForeignWaypoint final {
  float latitude;
  float longitude;
  float altitude;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignWaypoint

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignTrajectoryMessage
#define CXXBRIDGE1_STRUCT_shared$ForeignTrajectoryMessage
struct ForeignTrajectoryMessage final {
  ::rust::Vec<::shared::ForeignWaypoint> waypoints;
  ::rust::Vec<::std::uint8_t> recipients;
  bool is_swarm;
  ::std::uint64_t mission_id;
  ::std::uint64_t edit_id;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignTrajectoryMessage
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignCircle
#define CXXBRIDGE1_STRUCT_ForeignCircle
struct ForeignCircle final {
  ::shared::ForeignWaypoint center;
  float radius;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignCircle

#ifndef CXXBRIDGE1_STRUCT_ForeignPolygon
#define CXXBRIDGE1_STRUCT_ForeignPolygon
struct ForeignPolygon final {
  ::rust::Vec<::shared::ForeignWaypoint> boundary;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignPolygon

#ifndef CXXBRIDGE1_ENUM_ForeignGeozoneShape
#define CXXBRIDGE1_ENUM_ForeignGeozoneShape
enum class ForeignGeozoneShape : ::std::uint8_t {
  Circle = 0,
  Polygon = 1,
};
#endif // CXXBRIDGE1_ENUM_ForeignGeozoneShape

#ifndef CXXBRIDGE1_ENUM_ForeignZoneType
#define CXXBRIDGE1_ENUM_ForeignZoneType
enum class ForeignZoneType : ::std::uint8_t {
  Geofence = 0,
  Nofly = 1,
};
#endif // CXXBRIDGE1_ENUM_ForeignZoneType

namespace shared {
#ifndef CXXBRIDGE1_STRUCT_shared$ForeignGeozone
#define CXXBRIDGE1_STRUCT_shared$ForeignGeozone
struct ForeignGeozone final {
  ::std::uint32_t id;
  ::ForeignGeozoneShape shape;
  ::ForeignCircle circle;
  ::ForeignPolygon polygon;
  ::ForeignZoneType zone_type;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignGeozone

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignGeozoneMessage
#define CXXBRIDGE1_STRUCT_shared$ForeignGeozoneMessage
struct ForeignGeozoneMessage final {
  ::shared::ForeignGeozone zone;
  ::rust::Vec<::std::uint8_t> recipients;
  bool is_swarm;
  ::std::uint64_t mission_id;
  ::std::uint64_t edit_id;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignGeozoneMessage
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignOrientation
#define CXXBRIDGE1_STRUCT_ForeignOrientation
struct ForeignOrientation final {
  double qw;
  double qx;
  double qy;
  double qz;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignOrientation

namespace shared {
#ifndef CXXBRIDGE1_STRUCT_shared$ForeignGeneric
#define CXXBRIDGE1_STRUCT_shared$ForeignGeneric
struct ForeignGeneric final {
  ::rust::Vec<::std::uint8_t> state;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignGeneric
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignGeneric32
#define CXXBRIDGE1_STRUCT_ForeignGeneric32
struct ForeignGeneric32 final {
  ::rust::Vec<float> state;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignGeneric32

#ifndef CXXBRIDGE1_STRUCT_ForeignGeneric64
#define CXXBRIDGE1_STRUCT_ForeignGeneric64
struct ForeignGeneric64 final {
  ::rust::Vec<double> state;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignGeneric64

#ifndef CXXBRIDGE1_STRUCT_ForeignBattery
#define CXXBRIDGE1_STRUCT_ForeignBattery
struct ForeignBattery final {
  ::std::uint8_t level;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignBattery

#ifndef CXXBRIDGE1_STRUCT_ForeignSpeed
#define CXXBRIDGE1_STRUCT_ForeignSpeed
struct ForeignSpeed final {
  float value;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignSpeed

#ifndef CXXBRIDGE1_STRUCT_ForeignMessage
#define CXXBRIDGE1_STRUCT_ForeignMessage
struct ForeignMessage final {
  ::std::uint64_t id;
  ::rust::Vec<::rust::String> topics;
  ::std::uint64_t timestamp;
  ::rust::Vec<::std::uint8_t> payload;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignMessage

#ifndef CXXBRIDGE1_STRUCT_ForeignSwampMessage
#define CXXBRIDGE1_STRUCT_ForeignSwampMessage
struct ForeignSwampMessage final {
  ::std::uint64_t id;
  ::std::uint8_t sender;
  ::rust::String opcode;
  ::rust::Vec<::std::uint8_t> value;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignSwampMessage

namespace shared {
#ifndef CXXBRIDGE1_STRUCT_shared$ForeignFieldValue
#define CXXBRIDGE1_STRUCT_shared$ForeignFieldValue
struct ForeignFieldValue final {
  ::rust::String field;
  ::rust::String value;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignFieldValue

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignReadyMessage
#define CXXBRIDGE1_STRUCT_shared$ForeignReadyMessage
struct ForeignReadyMessage final {
  ::std::uint64_t mission_id;
  ::std::uint64_t edit_id;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignReadyMessage

#ifndef CXXBRIDGE1_STRUCT_shared$ForeignMissionInfoEnvelope
#define CXXBRIDGE1_STRUCT_shared$ForeignMissionInfoEnvelope
struct ForeignMissionInfoEnvelope final {
  ::std::uint64_t mission_id;
  ::std::uint64_t edit_id;
  bool is_start;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_shared$ForeignMissionInfoEnvelope
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignDynamicMessage
#define CXXBRIDGE1_STRUCT_ForeignDynamicMessage
struct ForeignDynamicMessage final {
  ::rust::Vec<::shared::ForeignFieldValue> data;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignDynamicMessage

#ifndef CXXBRIDGE1_STRUCT_ForeignClientMessage
#define CXXBRIDGE1_STRUCT_ForeignClientMessage
struct ForeignClientMessage final {
  ::std::uint64_t id;
  ::rust::String operation;
  ::rust::String topic;
  ::rust::Vec<::std::uint8_t> data;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignClientMessage

namespace shared {
#ifndef CXXBRIDGE1_ENUM_shared$InnerSensorMessage
#define CXXBRIDGE1_ENUM_shared$InnerSensorMessage
enum class InnerSensorMessage : ::std::uint8_t {
  Speed = 0,
  BatteryLevel = 1,
  Position = 2,
  Pose = 3,
  Orientation = 4,
  Generic = 5,
  Dynamic = 6,
  Unknown = 7,
};
#endif // CXXBRIDGE1_ENUM_shared$InnerSensorMessage
} // namespace shared

#ifndef CXXBRIDGE1_STRUCT_ForeignNestedGeneric
#define CXXBRIDGE1_STRUCT_ForeignNestedGeneric
struct ForeignNestedGeneric final {
  ::rust::Vec<::std::uint8_t> msg_ids;
  ::rust::Vec<::std::uint8_t> msg_vec;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ForeignNestedGeneric

namespace rust_part {
extern "C" {
void rust_part$cxxbridge1$build_command_message(::std::uint8_t operator_id, ::std::uint8_t command_type, ::rust::Vec<::std::uint8_t> *participants, ::std::uint8_t mission_command, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_nestedgeneric_message(::rust::Vec<::std::uint8_t> *msg_ids, ::rust::Vec<::std::uint8_t> *msg_vec, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_speed_message(float speed, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_battery_message(::std::uint8_t level, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_position_message(float x, float y, float z, bool is_global, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_target_position_message(float x, float y, float z, bool is_global, ::std::uint16_t target_id, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_orientation_message(float x, float y, float z, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_discovery_message(::std::uint16_t participant_id, ::rust::String *ip_address, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic_message(::rust::Vec<::std::uint8_t> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic32_message(::rust::Vec<float> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic64_message(::rust::Vec<double> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_swamp_message(::std::uint8_t sender, ::rust::String *opcode, ::rust::Vec<::std::uint8_t> *value, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_client_message(::rust::String *topic_name, ::rust::String *operation, ::rust::Vec<::std::uint8_t> *data, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_dynamic_message(::rust::Vec<::shared::ForeignFieldValue> *field_values, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_ready_message(::std::uint64_t mission_id, ::std::uint64_t edit_id, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$deserialize_nestedgeneric_message(::rust::Vec<::std::uint8_t> *value, ::ForeignNestedGeneric *return$) noexcept;

void rust_part$cxxbridge1$deserialize_command_message(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignCommandMessage *return$) noexcept;

::ForeignSpeed rust_part$cxxbridge1$deserialize_speed_message(::rust::Vec<::std::uint8_t> *value) noexcept;

void rust_part$cxxbridge1$deserialize_discovery_message(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignDiscoveryMessage *return$) noexcept;

::ForeignBattery rust_part$cxxbridge1$deserialize_battery_message(::rust::Vec<::std::uint8_t> *value) noexcept;

::ForeignPosition rust_part$cxxbridge1$deserialize_position_message(::rust::Vec<::std::uint8_t> *value) noexcept;

::ForeignPosition rust_part$cxxbridge1$deserialize_target_position_message(::rust::Vec<::std::uint8_t> *value) noexcept;

::ForeignOrientation rust_part$cxxbridge1$deserialize_orientation_message(::rust::Vec<::std::uint8_t> *value) noexcept;

void rust_part$cxxbridge1$deserialize_swamp_message(::rust::Vec<::std::uint8_t> *value, ::ForeignSwampMessage *return$) noexcept;

void rust_part$cxxbridge1$deserialize_client_message(::rust::Vec<::std::uint8_t> *value, ::ForeignClientMessage *return$) noexcept;

void rust_part$cxxbridge1$deserialize_generic_message(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignGeneric *return$) noexcept;

void rust_part$cxxbridge1$deserialize_generic32_message(::rust::Vec<::std::uint8_t> *value, ::ForeignGeneric32 *return$) noexcept;

void rust_part$cxxbridge1$deserialize_generic64_message(::rust::Vec<::std::uint8_t> *value, ::ForeignGeneric64 *return$) noexcept;

void rust_part$cxxbridge1$deserialize_dynamic_message(::rust::Vec<::std::uint8_t> *value, ::ForeignDynamicMessage *return$) noexcept;

void rust_part$cxxbridge1$deserialize_trajectory_message(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignTrajectoryMessage *return$) noexcept;

void rust_part$cxxbridge1$deserialize_geozone_message(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignGeozoneMessage *return$) noexcept;

::shared::ForeignReadyMessage rust_part$cxxbridge1$deserialize_ready_message(::rust::Vec<::std::uint8_t> *value) noexcept;

::shared::ForeignMissionInfoEnvelope rust_part$cxxbridge1$deserialize_mission_info_envelope_message(::rust::Vec<::std::uint8_t> *value) noexcept;

void rust_part$cxxbridge1$deserialize_message(::rust::Vec<::std::uint8_t> *value, ::ForeignMessage *return$) noexcept;

void rust_part$cxxbridge1$build_trajectory_message(::shared::ForeignTrajectoryMessage *ftr, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_geozone_message(::shared::ForeignGeozoneMessage *fgz, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_mission_info_envelope_message(::std::uint64_t mission_id, ::std::uint64_t edit_id, bool is_start, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_command(::std::uint8_t operator_id, ::std::uint8_t command_type, ::rust::Vec<::std::uint8_t> *participants, ::std::uint8_t mission_command, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_battery(::std::uint8_t level, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_position(float x, float y, float z, bool is_global, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_orientation(float x, float y, float z, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_discovery(::std::uint16_t participant_id, ::rust::String *ip_address, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic(::rust::Vec<::std::uint8_t> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic32(::rust::Vec<float> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$build_generic64(::rust::Vec<double> *state, ::rust::Vec<::std::uint8_t> *return$) noexcept;

void rust_part$cxxbridge1$deserialize_command(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignCommandMessage *return$) noexcept;

void rust_part$cxxbridge1$deserialize_discovery(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignDiscoveryMessage *return$) noexcept;

::ForeignBattery rust_part$cxxbridge1$deserialize_battery(::rust::Vec<::std::uint8_t> *value) noexcept;

::ForeignPosition rust_part$cxxbridge1$deserialize_position(::rust::Vec<::std::uint8_t> *value) noexcept;

::ForeignOrientation rust_part$cxxbridge1$deserialize_orientation(::rust::Vec<::std::uint8_t> *value) noexcept;

void rust_part$cxxbridge1$deserialize_generic(::rust::Vec<::std::uint8_t> *value, ::shared::ForeignGeneric *return$) noexcept;

void rust_part$cxxbridge1$deserialize_generic32(::rust::Vec<::std::uint8_t> *value, ::ForeignGeneric32 *return$) noexcept;

void rust_part$cxxbridge1$deserialize_generic64(::rust::Vec<::std::uint8_t> *value, ::ForeignGeneric64 *return$) noexcept;
} // extern "C"

::rust::Vec<::std::uint8_t> build_command_message(::std::uint8_t operator_id, ::std::uint8_t command_type, ::rust::Vec<::std::uint8_t> participants, ::std::uint8_t mission_command) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> participants$(::std::move(participants));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_command_message(operator_id, command_type, &participants$.value, mission_command, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_nestedgeneric_message(::rust::Vec<::std::uint8_t> msg_ids, ::rust::Vec<::std::uint8_t> msg_vec) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> msg_ids$(::std::move(msg_ids));
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> msg_vec$(::std::move(msg_vec));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_nestedgeneric_message(&msg_ids$.value, &msg_vec$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_speed_message(float speed) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_speed_message(speed, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_battery_message(::std::uint8_t level) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_battery_message(level, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_position_message(float x, float y, float z, bool is_global) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_position_message(x, y, z, is_global, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_target_position_message(float x, float y, float z, bool is_global, ::std::uint16_t target_id) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_target_position_message(x, y, z, is_global, target_id, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_orientation_message(float x, float y, float z) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_orientation_message(x, y, z, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_discovery_message(::std::uint16_t participant_id, ::rust::String ip_address) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_discovery_message(participant_id, &ip_address, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic_message(::rust::Vec<::std::uint8_t> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic_message(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic32_message(::rust::Vec<float> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<float>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic32_message(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic64_message(::rust::Vec<double> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<double>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic64_message(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_swamp_message(::std::uint8_t sender, ::rust::String opcode, ::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_swamp_message(sender, &opcode, &value$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_client_message(::rust::String topic_name, ::rust::String operation, ::rust::Vec<::std::uint8_t> data) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> data$(::std::move(data));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_client_message(&topic_name, &operation, &data$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_dynamic_message(::rust::Vec<::shared::ForeignFieldValue> field_values) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::shared::ForeignFieldValue>> field_values$(::std::move(field_values));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_dynamic_message(&field_values$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_ready_message(::std::uint64_t mission_id, ::std::uint64_t edit_id) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_ready_message(mission_id, edit_id, &return$.value);
  return ::std::move(return$.value);
}

::ForeignNestedGeneric deserialize_nestedgeneric_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignNestedGeneric> return$;
  rust_part$cxxbridge1$deserialize_nestedgeneric_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignCommandMessage deserialize_command_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignCommandMessage> return$;
  rust_part$cxxbridge1$deserialize_command_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignSpeed deserialize_speed_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_speed_message(&value$.value);
}

::shared::ForeignDiscoveryMessage deserialize_discovery_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignDiscoveryMessage> return$;
  rust_part$cxxbridge1$deserialize_discovery_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignBattery deserialize_battery_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_battery_message(&value$.value);
}

::ForeignPosition deserialize_position_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_position_message(&value$.value);
}

::ForeignPosition deserialize_target_position_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_target_position_message(&value$.value);
}

::ForeignOrientation deserialize_orientation_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_orientation_message(&value$.value);
}

::ForeignSwampMessage deserialize_swamp_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignSwampMessage> return$;
  rust_part$cxxbridge1$deserialize_swamp_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignClientMessage deserialize_client_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignClientMessage> return$;
  rust_part$cxxbridge1$deserialize_client_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignGeneric deserialize_generic_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignGeneric> return$;
  rust_part$cxxbridge1$deserialize_generic_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignGeneric32 deserialize_generic32_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignGeneric32> return$;
  rust_part$cxxbridge1$deserialize_generic32_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignGeneric64 deserialize_generic64_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignGeneric64> return$;
  rust_part$cxxbridge1$deserialize_generic64_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignDynamicMessage deserialize_dynamic_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignDynamicMessage> return$;
  rust_part$cxxbridge1$deserialize_dynamic_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignTrajectoryMessage deserialize_trajectory_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignTrajectoryMessage> return$;
  rust_part$cxxbridge1$deserialize_trajectory_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignGeozoneMessage deserialize_geozone_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignGeozoneMessage> return$;
  rust_part$cxxbridge1$deserialize_geozone_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignReadyMessage deserialize_ready_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_ready_message(&value$.value);
}

::shared::ForeignMissionInfoEnvelope deserialize_mission_info_envelope_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_mission_info_envelope_message(&value$.value);
}

::ForeignMessage deserialize_message(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignMessage> return$;
  rust_part$cxxbridge1$deserialize_message(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_trajectory_message(::shared::ForeignTrajectoryMessage ftr) noexcept {
  ::rust::ManuallyDrop<::shared::ForeignTrajectoryMessage> ftr$(::std::move(ftr));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_trajectory_message(&ftr$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_geozone_message(::shared::ForeignGeozoneMessage fgz) noexcept {
  ::rust::ManuallyDrop<::shared::ForeignGeozoneMessage> fgz$(::std::move(fgz));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_geozone_message(&fgz$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_mission_info_envelope_message(::std::uint64_t mission_id, ::std::uint64_t edit_id, bool is_start) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_mission_info_envelope_message(mission_id, edit_id, is_start, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_command(::std::uint8_t operator_id, ::std::uint8_t command_type, ::rust::Vec<::std::uint8_t> participants, ::std::uint8_t mission_command) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> participants$(::std::move(participants));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_command(operator_id, command_type, &participants$.value, mission_command, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_battery(::std::uint8_t level) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_battery(level, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_position(float x, float y, float z, bool is_global) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_position(x, y, z, is_global, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_orientation(float x, float y, float z) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_orientation(x, y, z, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_discovery(::std::uint16_t participant_id, ::rust::String ip_address) noexcept {
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_discovery(participant_id, &ip_address, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic(::rust::Vec<::std::uint8_t> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic32(::rust::Vec<float> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<float>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic32(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::rust::Vec<::std::uint8_t> build_generic64(::rust::Vec<double> state) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<double>> state$(::std::move(state));
  ::rust::MaybeUninit<::rust::Vec<::std::uint8_t>> return$;
  rust_part$cxxbridge1$build_generic64(&state$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignCommandMessage deserialize_command(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignCommandMessage> return$;
  rust_part$cxxbridge1$deserialize_command(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::shared::ForeignDiscoveryMessage deserialize_discovery(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignDiscoveryMessage> return$;
  rust_part$cxxbridge1$deserialize_discovery(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignBattery deserialize_battery(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_battery(&value$.value);
}

::ForeignPosition deserialize_position(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_position(&value$.value);
}

::ForeignOrientation deserialize_orientation(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  return rust_part$cxxbridge1$deserialize_orientation(&value$.value);
}

::shared::ForeignGeneric deserialize_generic(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::shared::ForeignGeneric> return$;
  rust_part$cxxbridge1$deserialize_generic(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignGeneric32 deserialize_generic32(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignGeneric32> return$;
  rust_part$cxxbridge1$deserialize_generic32(&value$.value, &return$.value);
  return ::std::move(return$.value);
}

::ForeignGeneric64 deserialize_generic64(::rust::Vec<::std::uint8_t> value) noexcept {
  ::rust::ManuallyDrop<::rust::Vec<::std::uint8_t>> value$(::std::move(value));
  ::rust::MaybeUninit<::ForeignGeneric64> return$;
  rust_part$cxxbridge1$deserialize_generic64(&value$.value, &return$.value);
  return ::std::move(return$.value);
}
} // namespace rust_part

extern "C" {
void cxxbridge1$rust_vec$shared$ForeignWaypoint$new(::rust::Vec<::shared::ForeignWaypoint> const *ptr) noexcept;
void cxxbridge1$rust_vec$shared$ForeignWaypoint$drop(::rust::Vec<::shared::ForeignWaypoint> *ptr) noexcept;
::std::size_t cxxbridge1$rust_vec$shared$ForeignWaypoint$len(::rust::Vec<::shared::ForeignWaypoint> const *ptr) noexcept;
::std::size_t cxxbridge1$rust_vec$shared$ForeignWaypoint$capacity(::rust::Vec<::shared::ForeignWaypoint> const *ptr) noexcept;
::shared::ForeignWaypoint const *cxxbridge1$rust_vec$shared$ForeignWaypoint$data(::rust::Vec<::shared::ForeignWaypoint> const *ptr) noexcept;
void cxxbridge1$rust_vec$shared$ForeignWaypoint$reserve_total(::rust::Vec<::shared::ForeignWaypoint> *ptr, ::std::size_t new_cap) noexcept;
void cxxbridge1$rust_vec$shared$ForeignWaypoint$set_len(::rust::Vec<::shared::ForeignWaypoint> *ptr, ::std::size_t len) noexcept;
void cxxbridge1$rust_vec$shared$ForeignWaypoint$truncate(::rust::Vec<::shared::ForeignWaypoint> *ptr, ::std::size_t len) noexcept;

void cxxbridge1$rust_vec$shared$ForeignFieldValue$new(::rust::Vec<::shared::ForeignFieldValue> const *ptr) noexcept;
void cxxbridge1$rust_vec$shared$ForeignFieldValue$drop(::rust::Vec<::shared::ForeignFieldValue> *ptr) noexcept;
::std::size_t cxxbridge1$rust_vec$shared$ForeignFieldValue$len(::rust::Vec<::shared::ForeignFieldValue> const *ptr) noexcept;
::std::size_t cxxbridge1$rust_vec$shared$ForeignFieldValue$capacity(::rust::Vec<::shared::ForeignFieldValue> const *ptr) noexcept;
::shared::ForeignFieldValue const *cxxbridge1$rust_vec$shared$ForeignFieldValue$data(::rust::Vec<::shared::ForeignFieldValue> const *ptr) noexcept;
void cxxbridge1$rust_vec$shared$ForeignFieldValue$reserve_total(::rust::Vec<::shared::ForeignFieldValue> *ptr, ::std::size_t new_cap) noexcept;
void cxxbridge1$rust_vec$shared$ForeignFieldValue$set_len(::rust::Vec<::shared::ForeignFieldValue> *ptr, ::std::size_t len) noexcept;
void cxxbridge1$rust_vec$shared$ForeignFieldValue$truncate(::rust::Vec<::shared::ForeignFieldValue> *ptr, ::std::size_t len) noexcept;
} // extern "C"

namespace rust {
inline namespace cxxbridge1 {
template <>
Vec<::shared::ForeignWaypoint>::Vec() noexcept {
  cxxbridge1$rust_vec$shared$ForeignWaypoint$new(this);
}
template <>
void Vec<::shared::ForeignWaypoint>::drop() noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$drop(this);
}
template <>
::std::size_t Vec<::shared::ForeignWaypoint>::size() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$len(this);
}
template <>
::std::size_t Vec<::shared::ForeignWaypoint>::capacity() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$capacity(this);
}
template <>
::shared::ForeignWaypoint const *Vec<::shared::ForeignWaypoint>::data() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$data(this);
}
template <>
void Vec<::shared::ForeignWaypoint>::reserve_total(::std::size_t new_cap) noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$reserve_total(this, new_cap);
}
template <>
void Vec<::shared::ForeignWaypoint>::set_len(::std::size_t len) noexcept {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$set_len(this, len);
}
template <>
void Vec<::shared::ForeignWaypoint>::truncate(::std::size_t len) {
  return cxxbridge1$rust_vec$shared$ForeignWaypoint$truncate(this, len);
}
template <>
Vec<::shared::ForeignFieldValue>::Vec() noexcept {
  cxxbridge1$rust_vec$shared$ForeignFieldValue$new(this);
}
template <>
void Vec<::shared::ForeignFieldValue>::drop() noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$drop(this);
}
template <>
::std::size_t Vec<::shared::ForeignFieldValue>::size() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$len(this);
}
template <>
::std::size_t Vec<::shared::ForeignFieldValue>::capacity() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$capacity(this);
}
template <>
::shared::ForeignFieldValue const *Vec<::shared::ForeignFieldValue>::data() const noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$data(this);
}
template <>
void Vec<::shared::ForeignFieldValue>::reserve_total(::std::size_t new_cap) noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$reserve_total(this, new_cap);
}
template <>
void Vec<::shared::ForeignFieldValue>::set_len(::std::size_t len) noexcept {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$set_len(this, len);
}
template <>
void Vec<::shared::ForeignFieldValue>::truncate(::std::size_t len) {
  return cxxbridge1$rust_vec$shared$ForeignFieldValue$truncate(this, len);
}
} // namespace cxxbridge1
} // namespace rust

