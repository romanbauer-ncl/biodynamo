#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include <sstream>
#include <utility>
#include <typeinfo>

#include <Vc/Vc>
#include <omp.h>

#include "timing.h"
#include "timing_aggregator.h"

using bdm::Timing;
using bdm::TimingAggregator;

// benchmark function declarations
void benchmarkSoaCell(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic);
void benchmarkAosoaCell(const size_t num_cells, const size_t iterations,
          TimingAggregator* statistic);
void benchmarkPlainSoa(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic);
void benchmarkPlainAosoa(const size_t num_cells, const size_t iterations,
          TimingAggregator* statistic);

template<typename T>
class ScalarRefWrapper {
public:
  ScalarRefWrapper(T&& data) : data_(std::move(data)) {}

  ScalarRefWrapper(const ScalarRefWrapper<T>& other) : data_(std::move(other.data_)) {}
    //TODO add all operators

  T&& operator[](std::size_t index) {
    return std::move(data_);
  }

  const T&& operator[](std::size_t index) const {
    return std::move(data_);
  }

  ScalarRefWrapper<T>& operator=(const ScalarRefWrapper<T>& other) {
    if (this != &other) {
      std::move(data_) = other.data_;
    }
    return *this;
  }

  // TODO remove
  void foo(T&& ref, int idx) {
    std::move(ref) = idx;
  }

  T* begin() { return &data_; }
  T* end() { return &data_+1; }

  const T* begin() const { return &data_; }
  const T* end() const { return &data_+1; }

private:
  T&& data_;
};

// template<typename T>
// class VcScalarRefWrapper {
// public:
//   VcScalarRefWrapper(T& data, size_t index) : data_(data), index_(index) {}
//
//     //TODO add all operators
//
//   decltype(std::declval<T>()[0])&& operator[](std::size_t index) {
//     return std::move(data_[index_]);
//   }
//
//   const decltype(std::declval<T>()[0])&& operator[](std::size_t index) const {
//     return std::move(data_[index_]);
//   }
//
//   VcScalarRefWrapper<T>& operator=(const VcScalarRefWrapper<T>& other) {
//     if (this != &other) {
//       std::move(data_[index_]) = other.data_[other.index_];
//     }
//     return *this;
//   }
//
//   template<typename U>
//   Vc_ALWAYS_INLINE auto operator<(const U& u) const -> decltype(std::declval<T>() < u) {
//     return data_ < u;
//   }
//
//   template<typename U>
//   Vc_ALWAYS_INLINE auto operator*(const U& u) const -> decltype(std::declval<T>() * u) {
//     return data_ * u;
//   }
//
//   template<typename U>
//   Vc_ALWAYS_INLINE VcScalarRefWrapper<T>& operator+=(const U& u) {
//     std::move(data_[index_]) += u;
//     return *this;
//   }
//
//   // TODO remove
//   void foo(T&& ref, int idx) {
//     std::move(ref) = idx;
//   }
//
// private:
//   T& data_;
//   std::size_t index_;
// };

template<typename T>
class VcScalarRefWrapper {
public:
  VcScalarRefWrapper(Vc::double_v& data, size_t index) : data_(data), index_(index) {}

    //TODO add all operators

  VcScalarRefWrapper<T>& operator[](std::size_t index) {
    return *this;
  }
  //  Vc::SimdArray<double, 1> operator[](std::size_t index) {
  //    return Vc::SimdArray<double, 1>(data_[index_]);
  // }

  // const decltype(std::declval<Vc::double_v>()[0])&& operator[](std::size_t index) const {
  const VcScalarRefWrapper<T>& operator[](std::size_t index) const {
    return *this;
  }

  // Vc::SimdArray<double, 1> operator[](std::size_t index) const {
  //   return Vc::SimdArray<double, 1>(data_[index_]);
  // }

  VcScalarRefWrapper<T>& operator=(const VcScalarRefWrapper<T>& other) {
    if (this != &other) {
      std::move(data_[index_]) = other.data_[other.index_];
    }
    return *this;
  }

  VcScalarRefWrapper<T>& operator=(const Vc::SimdArray<double, 1>& other) {
    std::move(data_[index_]) = other[0];
    return *this;
  }

  template<typename U>
  Vc_ALWAYS_INLINE auto operator<(const U& u) const -> decltype(std::declval<T>() < u) {
    return data_ < u;
  }

  template<typename U>
  Vc_ALWAYS_INLINE auto operator*(const U& u) const -> decltype(std::declval<T>() * u) {
    return data_ * u;
  }

  // Vc_ALWAYS_INLINE auto operator*(const VcScalarRefWrapper<T>& other) const -> decltype(std::declval<Vc::double_v>() * std::declval<Vc::double_v>()) {
  //   return data_ * other.data_;
  // }

  template<typename U>
  Vc_ALWAYS_INLINE VcScalarRefWrapper<T>& operator+=(const U& u) {
    std::move(data_[index_]) += u;
    return *this;
  }

  friend std::ostream &operator<<( std::ostream &out, const VcScalarRefWrapper<T> &wrapper ) {
    out << wrapper.data_[wrapper.index_];
    return out;
  }

  // TODO remove
  // void foo(T&& ref, int idx) {
  //   std::move(ref) = idx;
  // }

private:
  Vc::double_v& data_;
  std::size_t index_;
};

template<typename T>
class SoaRefWrapper { // FIXME can we use the same refwrapper for scalar and vector??
public:
  SoaRefWrapper(T& data) : data_(data) {}

  //TODO add all operators

  Vc_ALWAYS_INLINE  typename T::value_type& operator[](std::size_t index) {
    return data_[index];
  }

  Vc_ALWAYS_INLINE  const typename T::value_type& operator[](std::size_t index) const {
    return data_[index];
  }

  template<typename U>
  Vc_ALWAYS_INLINE  auto operator<=(const U& u) const -> decltype(std::declval<typename T::value_type>() <= u) {
    return data_ <= u;
  }

  template<typename U>
  Vc_ALWAYS_INLINE  auto operator<(const U& u) const -> decltype(std::declval<typename T::value_type>() < u) {
    return data_ < u;
  }

  template<typename U>
  Vc_ALWAYS_INLINE  SoaRefWrapper<T>& operator+=(const U& u) {
    data_ += u;
    return *this;
  }

  Vc_ALWAYS_INLINE  SoaRefWrapper<T>& operator=(const SoaRefWrapper<T>& other) {
    if (this != &other) {
      data_ = other.data_;
    }
    return *this;
  }

  friend std::ostream &operator<<( std::ostream &out, const SoaRefWrapper<T> &wrapper ) {
    out << wrapper.data_;
    return out;
  }

// FIXME
  typename T::iterator begin() { return data_.begin(); }
  typename T::iterator end() { return data_.end(); }

  typename T::const_iterator begin() const { return data_.cbegin(); }
  typename T::const_iterator end() const { return data_.cend(); }

private:
  T& data_;
};

/// This class represents an array with exactly one element
/// Needed for AOSOA: Objects will store a single e.g. real_v instead of N
/// instances. However code was written for SOA and expects an array interface
/// which is exposed with this class.
/// Makes it easy for the compiler to optimize out the extra call to operator[]
/// Didn't work with std::array<T, 1>
template<typename T>
class OneElementArray {
public:
  OneElementArray() : data_() {}
  OneElementArray(const T& data) : data_(data) {}
  OneElementArray(T&& data) : data_(data) {}

  Vc_ALWAYS_INLINE T& operator[](const size_t idx) {
    return data_;
  }

  Vc_ALWAYS_INLINE const T& operator[](const size_t idx) const {
    return data_;
  }

  T* begin() { return &data_; }
  T* end() { return &data_+1; }

  const T* begin() const { return &data_; }
  const T* end() const { return &data_+1; }
private:
  T data_;
};

template<typename T>
class OneElementArray1 {
public:
  OneElementArray1() : data_() {}
  OneElementArray1(T&& data) : data_(data) {}

  // Vc_ALWAYS_INLINE Vc::SimdArray<double, 1> operator[](const size_t idx) {
  //   return Vc::SimdArray<double, 1>(data_[idx]);
  // }
  //
  // Vc_ALWAYS_INLINE Vc::SimdArray<double, 1> operator[](const size_t idx) const {
  //   return Vc::SimdArray<double, 1>(data_[idx]);
  // }
  Vc_ALWAYS_INLINE T& operator[](const size_t idx) {
    return data_;
  }

  Vc_ALWAYS_INLINE const T& operator[](const size_t idx) const {
    return data_;
  }

  T* begin() { return &data_; }
  T* end() { return &data_+1; }

  const T* begin() const { return &data_; }
  const T* end() const { return &data_+1; }
private:
  T data_;
};

template<bool condition, typename T, typename U>
struct type_ternary_operator {};

template<typename T, typename U>
struct type_ternary_operator<true, T, U> {
  typedef T type;
};

template<typename T, typename U>
struct type_ternary_operator<false, T, U> {
  typedef U type;
};

struct VcBackend {
  typedef double real_t;
  static const size_t kVecLen = Vc::double_v::Size;
  typedef Vc::double_v real_v;
  template<typename T> using container =  std::array<T, kVecLen>;
  template<typename T, typename Allocator=std::allocator<T>> using soa = OneElementArray<T>;
};

struct VcSoaBackend {
  typedef double real_t;
  static const size_t kVecLen = VcBackend::kVecLen;
  typedef VcBackend::real_v real_v;
  template<typename T> using container = typename VcBackend::template container<T>;
  template<typename T, typename Allocator=std::allocator<T> > using soa =  std::vector<T, Allocator>;
};

struct VcSoaRefBackend {
  typedef double real_t;
  static const size_t kVecLen = VcBackend::kVecLen;
  typedef VcBackend::real_v real_v;
  template<typename T> using container =  typename VcSoaBackend::template container<T>;
  template<typename T, typename Allocator=std::allocator<T> > using soa =  SoaRefWrapper<typename VcSoaBackend::template soa<T, Allocator> >;
};

struct ScalarBackend {
  typedef double real_t;
  static const size_t kVecLen = 1;
  typedef Vc::SimdArray<double, kVecLen> real_v; // TODO change to OneElementArray?
  template<typename T> using container = OneElementArray<T>;
  template<typename T, typename Allocator=std::allocator<T>> using soa = OneElementArray<T>;
};

struct ScalarRefBackend {  //TODO rename to VcScalarRefBackend
  typedef double real_t;
  static const size_t kVecLen = 1;
  typedef Vc::SimdArray<double, 1> real_v;
  template<typename T> using container =  T;
  template<typename T, typename Allocator=std::allocator<T>> using soa = typename type_ternary_operator<std::is_same<T, real_v>::value, OneElementArray1<VcScalarRefWrapper<T> >, OneElementArray<SoaRefWrapper<T> > >::type;  // TODO replace SoaRefWrapper with container specification
};

template<typename T, typename U=T>
T iif(const decltype(std::declval<T>() < 0)& condition, const T& true_value, const U& false_value) {}

template<>
typename VcBackend::real_v iif<typename VcBackend::real_v>(const decltype(std::declval<typename VcBackend::real_v>() < std::declval<typename VcBackend::real_v>())& condition, const typename VcBackend::real_v& true_value, const typename VcBackend::real_v& false_value) {
  return Vc::iif(condition, true_value, false_value);
}

template<>
typename ScalarRefBackend::real_v iif<typename ScalarRefBackend::real_v>(const decltype(std::declval<typename ScalarRefBackend::real_v>() < std::declval<typename ScalarRefBackend::real_v>())& condition, const typename ScalarRefBackend::real_v& true_value, const typename ScalarRefBackend::real_v& false_value) {
  throw "not yet implemented exception";
}

struct Neurite {
  Neurite() :id(0) {}
  Neurite(size_t id) : id(id) {}
  size_t id = 0;
};

// <generated>
// makes it more convenient to use
template<typename Delegate>
class CellScalarWrapper {
public:
  // FIXME verify that Delegate is Cell<ScalarRefBackend>
  explicit CellScalarWrapper(const Delegate& delegate) : delegate_{delegate} {}

  double GetDiameter() const {
    return delegate_.GetDiameter()[0];
  }

  void SetDiameter(double diameter) {
    delegate_.SetDiameter({diameter});
  }

  double GetVolume() const {
    return delegate_.GetVolume()[0];
  }

  void SetVolume(double volume) {
    delegate_.SetVolume({volume});
  }

  void UpdateVolume() {
    delegate_.UpdateVolume();
  }

  void ChangeVolume(double speed) {
    delegate_.ChangeVolume({speed});
  }

  const std::vector<Neurite>& GetNeurites() const {
    return delegate_.GetNeurites()[0];
  }

  void SetNeurites(const std::vector<Neurite>& neurites) {
    delegate_.SetNeurites({neurites});
  }

  void UpdateNeurites() {
    delegate_.UpdateNeurites();
  }

  friend std::ostream &operator<<( std::ostream &out, const CellScalarWrapper<Delegate> &cell ) {
    out << cell.delegate_;
    return out;
  }

  //TODO begin and end needed?

private:
  Delegate delegate_;
};

template<typename Delegate>
class CellScalarWrapperRef {
public:
  // FIXME verify that Delegate is Cell<ScalarRefBackend>
  explicit CellScalarWrapperRef(const Delegate& delegate) : delegate_{delegate} {}

  double GetDiameter() const {
    return delegate_.GetDiameter()[0];
  }

  void SetDiameter(double diameter) {
    typename VcBackend::real_v vec(diameter); // TODO huge overhead
    delegate_.SetDiameter(VcScalarRefWrapper<typename VcBackend::real_v>(vec, 0));
  }

  double GetVolume() const {
    return delegate_.GetVolume()[0];
  }

  void SetVolume(double volume) {
    typename VcBackend::real_v vec(volume);
    delegate_.SetVolume(VcScalarRefWrapper<typename VcBackend::real_v>(vec, 0));
  }

  void UpdateVolume() {
    delegate_.UpdateVolume();
  }

  void ChangeVolume(double speed) {
    typename VcBackend::real_v vec(speed);
    delegate_.ChangeVolume(VcScalarRefWrapper<typename VcBackend::real_v>(vec, 0));
  }

  const std::vector<Neurite>& GetNeurites() const {
    return delegate_.GetNeurites()[0];
  }

  void SetNeurites(std::vector<Neurite>& neurites) {  //FIXME if neurites is defined as const binding to ref doesn't work
    delegate_.SetNeurites({neurites});
  }

  void UpdateNeurites() {
    delegate_.UpdateNeurites();
  }

  friend std::ostream &operator<<( std::ostream &out, const CellScalarWrapperRef<Delegate> &cell ) {
    out << cell.delegate_;
    return out;
  }

private:
  Delegate delegate_;
};
// </generated>

template<typename Backend>
class Cell {
public:
  using real_v = typename Backend::real_v;
  template<typename T>
  using container = typename Backend::template container<T>;

  template<typename T, typename Allocator = std::allocator<T> >
  using soa =typename Backend::template soa<T, Allocator>;

  template <typename T>
  friend class Cell;

  // <required interface>
  static CellScalarWrapper<Cell<ScalarBackend> > NewScalar() { // todo perfect forwarding for ctor arguments
    return CellScalarWrapper<Cell<ScalarBackend> >(Cell<ScalarBackend>());
  }

  template<typename T=Backend>
  Cell(Cell<VcBackend>& other, size_t idx, typename std::enable_if<std::is_same<T, ScalarRefBackend>::value>::type* = 0) :
    diameter_(VcScalarRefWrapper<Vc::SimdArray<double, 1> >(other.diameter_[idx_], idx)), // TODO fix template param
    volume_(VcScalarRefWrapper<Vc::SimdArray<double, 1> >(other.volume_[idx_], idx)),  // TODO fix template param
    neurites_(other.neurites_[idx_][idx]) {
  }

  CellScalarWrapper<Cell<ScalarRefBackend> > GetScalar(size_t idx) { // TODO VcBackend versino of operator[]
    return CellScalarWrapper<Cell<ScalarRefBackend> >(Cell<ScalarRefBackend>(*this, idx));
  }

  //TODO make protected
  template<typename T=Backend>
  Cell(Cell<VcSoaBackend>& other, typename std::enable_if<std::is_same<T, VcSoaRefBackend>::value>::type* = 0) :
    diameter_(other.diameter_),
    volume_(other.volume_),
    neurites_(other.neurites_) {
  }

  Vc_ALWAYS_INLINE
  Cell<VcSoaRefBackend> GetSoaRef() { // TODO only for SoaBackends
    return Cell<VcSoaRefBackend>(*this);
  }
  //
  void push_back(const Cell<VcBackend>& other) {
    diameter_.push_back(other.diameter_[0]);
    volume_.push_back(other.volume_[0]);
    neurites_.push_back(other.neurites_[0]);
  } //FIXME different version for VcBackend and SoaBackend

  Cell<Backend>& operator[](int index) {  // TODO only for SoaBackend; TODO add version for VcBackend
    idx_ = index;
    return *this;
  }

  // std::enable_if<std::is_same<Backend, SoaBackend>::value || std::is_same<Backend, VcSoaRefBackend>::value, size_t> size() const {
  //   return diameter_.size();
  // }
  // </required interface>

  Cell() {}

  const real_v& GetDiameter() const {
    return diameter_[idx_];
  }

  void SetDiameter(const real_v& diameter) {
    diameter_[idx_] = diameter;
  }

  const real_v& GetVolume() const {
    return volume_[idx_];
  }

  void SetVolume(const real_v& volume) {
    volume_[idx_] = volume;
  }

  void UpdateVolume() {
    for(size_t i = 0; i < Backend::kVecLen; i++) {
      //volume_[idx_][i] = diameter_[idx_][i] * diameter_[idx_][i] * diameter_[idx_][i] * 4 / 3 * 3.14;
    }
  }

  void ChangeVolume(const real_v& speed) {
    volume_[idx_] += speed  * 0.01;
    volume_[idx_] = iif(volume_[idx_] < 5.2359877E-7, real_v(5.2359877E-7), volume_[idx_]);

     // UpdateDiameter();
     // for (size_t j = 0; j < Vc::double_v::Size; j++) {
     //   vc_diameter[idx][j] = std::cbrt(vc_volume[idx][j] * 6 / 3.14);
     // }
  }

  const container<std::vector<Neurite> >& GetNeurites() const {
    return neurites_[idx_];
  }

  void SetNeurites(const container<std::vector<Neurite> >& neurites) {
    neurites_[idx_] = neurites;
  }

  void UpdateNeurites() {
    for (auto& neurites : neurites_[idx_]) {
        for (auto& neurite : neurites) {
          neurite.id++;
        }
    }
  }

  friend std::ostream &operator<<( std::ostream &out, const Cell<Backend> &cell ) {
    out << "  cell : " << std::endl;
    out << "    diameter: \t" << cell.diameter_[cell.idx_] << std::endl;
    out << "    volume: \t" << cell.volume_[cell.idx_] << std::endl;

    out << "    neurites: \t";
    for (auto& neurites : cell.neurites_[cell.idx_]) {
        std::cout << "{";
        for (auto& neurite : neurites) {
          std::cout << neurite.id << ", ";
        }
        std::cout << "}, ";
    }

    return out;
  }

  private:
    std::size_t idx_ = 0;
    soa<real_v, Vc::Allocator<real_v> > diameter_;  // TODO change backend so soa takes Vc::Allocator if a real_v is passed as template parameter
    soa<real_v, Vc::Allocator<real_v> > volume_;
    soa<container<std::vector<Neurite> > > neurites_;
};

template<typename T>
void ClientCodeExample(T* cells) {
  auto&& cell = (*cells)[0];
  cell.SetDiameter(Vc::double_v(34));
  cell.SetVolume(Vc::double_v(56));
  cell.ChangeVolume(Vc::double_v(7));
  cell.UpdateVolume();
  std::vector<Neurite> neurites_1;
  neurites_1.push_back(Neurite(987));
  neurites_1.push_back(Neurite(654));
  typename VcBackend::container<std::vector<Neurite> > neurites;
  neurites[1] = neurites_1;
  cell.SetNeurites(neurites);
  cell.UpdateNeurites();
  auto& cell_neurites = cell.GetNeurites();
}

int main(int argc, char** argv) {
  // vector cell
  std::cout << std::endl << "-----------------------------------" << std::endl;
  std::cout << "vector cell" << std::endl;
  Cell<VcBackend> cell;
  std::cout << "initial vector cell " << std::endl << cell << std::endl;
  cell.SetDiameter(Vc::double_v(10));
  cell.SetVolume(Vc::double_v(12));
  cell.ChangeVolume(Vc::double_v(3));
  std::cout << "vector cell " << std::endl << cell << std::endl;
  cell.UpdateVolume();
  std::cout << "cell.GetVolume(): " << cell.GetVolume() << std::endl;
  std::vector<Neurite> neurites_0;
  neurites_0.push_back(Neurite(123));
  neurites_0.push_back(Neurite(456));
  typename VcBackend::container<std::vector<Neurite> > neurites;
  neurites[0] = neurites_0;
  cell.SetNeurites(neurites);
  cell.UpdateNeurites();
  auto& cell_neurites = cell.GetNeurites();
  std::cout << "VcBackend after operations " << std::endl << cell << std::endl;

  // different memory layout and client code
  std::cout << std::endl << "-----------------------------------" << std::endl;
  std::cout << "different memory layout and client code" << std::endl;
  std::cout << "Original VcBackend cell" << std::endl <<  cell << std::endl;
  Cell<VcSoaBackend> soa_cells;
  soa_cells.push_back(cell);
  std::cout << "Vector cell stored in SOA memory layout" << std::endl;
  std::cout << soa_cells[0] << std::endl;
  ClientCodeExample(&soa_cells);
  std::cout << "after client code invocation" << std::endl;
  std::cout << soa_cells[0] << std::endl;

  std::vector<Cell<VcBackend>, Vc::Allocator<Cell<VcBackend> > > aosoa_cells;
  aosoa_cells.push_back(cell);
  std::cout << "Vector cell stored in AOSOA memory layout" << std::endl;
  std::cout << aosoa_cells[0] << std::endl;
  ClientCodeExample(&aosoa_cells);
  std::cout << "after client code invocation" << std::endl;
  std::cout << soa_cells[0] << std::endl;

  // scalar cell
  std::cout << std::endl << "-----------------------------------" << std::endl;
  std::cout << "scalar cell" << std::endl;
  auto scalar = Cell<ScalarBackend>::NewScalar();
  std::cout << "initial scalar cell" << std::endl << scalar << std::endl;
  scalar.SetDiameter(10);
  scalar.SetVolume(91);
  scalar.ChangeVolume(3);
  std::cout << "scalar cell" << std::endl << scalar << std::endl;
  scalar.UpdateVolume();
  std::cout << "scalar_cell.GetVolume(): " << scalar.GetVolume() << std::endl;
  std::vector<Neurite> neurites_scalar_cell;
  neurites_scalar_cell.push_back(Neurite(123));
  neurites_scalar_cell.push_back(Neurite(456));
  scalar.SetNeurites(neurites_scalar_cell);
  scalar.UpdateNeurites();
  auto& scalar_neurites = cell.GetNeurites();
  std::cout << "final scalar cell" << std::endl << scalar << std::endl;

  // scalar reference into vector cell
  std::cout << std::endl << "-----------------------------------" << std::endl;
  std::cout << "scalar reference" << std::endl;
  std::cout << "initial VcBackend cell" << std::endl <<  cell << std::endl;
  auto&& scalar_ref = cell.GetScalar(1);
  scalar_ref.SetDiameter(98);
  scalar_ref.SetVolume(76);
  // scalar_ref.ChangeVolume(3);
  // std::cout << "scalar_ref cell: " << std::endl << scalar_ref << std::endl;
  scalar_ref.UpdateVolume();
  // std::cout << "scalar_ref cell: " << std::endl << scalar_ref << std::endl;
  // std::cout << "scalar_ref.GetVolume(): " << scalar_ref.GetVolume() << std::endl;
  // TODO interact with neurites
  // std::cout << "scalar reference " << std::endl << scalar_ref << std::endl;
  std::cout << " final VcBackend cell after modifications with scalar ref" << std::endl;
  std::cout << cell << std::endl;

  // create soa container add and retrieve elements
  if( argc == 4) {
    std::cout << std::endl;
    std::cout << "-------------------------------------------------" << std::endl;
    size_t cells;
    size_t iterations;
    size_t threads;
    std::istringstream(std::string(argv[1])) >> cells;
    std::istringstream(std::string(argv[2])) >> iterations;
    std::istringstream(std::string(argv[3])) >> threads;
    omp_set_num_threads(threads);
    TimingAggregator statistic;
    benchmarkPlainSoa(cells, iterations, &statistic);
    benchmarkSoaCell(cells, iterations, &statistic);
    benchmarkPlainAosoa(cells, iterations, &statistic);
    benchmarkAosoaCell(cells, iterations, &statistic);
    std::cout << statistic << std::endl;
  }
}

void benchmarkSoaCell(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic) {
  const size_t N = num_cells / Vc::double_v::Size;
  Cell<VcSoaBackend> cells;

// #pragma omp parallel for
  for (size_t i = 0; i < N; i++) {
    Cell<VcBackend> cell;

    cells.push_back(cell);
  }

  auto&& cells_ref = cells.GetSoaRef();

  {
    Timing timing("soaCell", statistic);
// #pragma omp parallel for default(none) shared(cells) firstprivate(iterations)
    for (size_t i = 0; i < N; i++) {
        auto& cell = cells_ref[i];
        auto ifresult = cell.GetDiameter() <= 40;
        Vc::double_v dv(300);
        dv.setZeroInverted(ifresult);
        cell.ChangeVolume(dv);
    }
  }
}


void benchmarkPlainSoa(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic) {
  const size_t N = num_cells / Vc::double_v::Size;
  class SoaCell {
  public:
    using real_v = Vc::double_v;

    SoaCell(const size_t elements) {
      // diameter_ = new Vc::double_v[elements];
      // volume_ = new Vc::double_v[elements];
      diameter_.resize(elements);
      volume_.resize(elements);
      neurites_.resize(elements);
      for (size_t i = 0; i < elements; i++) {
        diameter_[i] = real_v(20);
        volume_[i] = real_v(0);
      }
    }

    virtual ~SoaCell() {
      // delete[] diameter_;
      // delete[] volume_;
    }

    const real_v& GetDiameter(const size_t idx) const {
      return diameter_[idx];
    }

    void ChangeVolume(const size_t idx, const real_v& speed) {
      volume_[idx] += speed  * 0.01;
      volume_[idx] = Vc::iif(volume_[idx] < 5.2359877E-7,
                       Vc::double_v(5.2359877E-7), volume_[idx]);

       // UpdateDiameter();
       // for (size_t j = 0; j < Vc::double_v::Size; j++) {
       //   vc_diameter[idx][j] = std::cbrt(vc_volume[idx][j] * 6 / 3.14);
       // }
    }

    void SetIndex(const size_t idx) {
      idx_ = idx;
    }

    const real_v& GetDiameter() const {
      return diameter_[idx_];
    }

    void ChangeVolume(const real_v& speed) {
      volume_[idx_] += speed  * 0.01;
      volume_[idx_] = Vc::iif(volume_[idx_] < 5.2359877E-7,
                       Vc::double_v(5.2359877E-7), volume_[idx_]);

       // UpdateDiameter();
       // for (size_t j = 0; j < Vc::double_v::Size; j++) {
       //   vc_diameter[idx][j] = std::cbrt(vc_volume[idx][j] * 6 / 3.14);
       // }
    }

  private:
    size_t idx_ = 0;
    // real_v* diameter_;
    // real_v* volume_;
    std::vector<real_v> diameter_;
    std::vector<real_v> volume_;
    std::vector<std::array<Neurite, real_v::Size> > neurites_;
  };

  SoaCell cells(N);

// #pragma omp parallel for
  // for (size_t i = 0; i < N; i++) {
  //   cells.diameter[i] = Vc::double_v(30);
  //   cells.volume[i] = Vc::double_v(1);
  // }

  {
    Timing timing("vcSoa", statistic);
// #pragma omp parallel for default(none) shared(vc_diameter, vc_volume) firstprivate(iterations)
    for (size_t i = 0; i < N; i++) {
      cells.SetIndex(i);
      auto ifresult = cells.GetDiameter() <= 40;
      Vc::double_v dv(300);
      dv.setZeroInverted(ifresult);
      cells.ChangeVolume(dv);

      // auto ifresult = cells.GetDiameter(i) <= 40;
      // Vc::double_v dv(300);
      // dv.setZeroInverted(ifresult);
      // cells.ChangeVolume(i, dv);
    }
  }
}

void benchmarkAosoaCell(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic) {
  const size_t N = num_cells / Vc::double_v::Size;
  std::vector<Cell<VcBackend>, Vc::Allocator<Cell<VcBackend> > > cells;

// #pragma omp parallel for
  for (size_t i = 0; i < N; i++) {
    Cell<VcBackend> cell;

    cells.push_back(cell);
  }

  {
    Timing timing("aosoaCell", statistic);
// #pragma omp parallel for default(none) shared(cells) firstprivate(iterations)
    for (size_t i = 0; i < N; i++) {
        auto& cell = cells[i];
        auto ifresult = cell.GetDiameter() <= 40;
        Vc::double_v dv(300);
        dv.setZeroInverted(ifresult);
        cell.ChangeVolume(dv);
    }
  }
}

void benchmarkPlainAosoa(const size_t num_cells, const size_t iterations,
           TimingAggregator* statistic) {
  const size_t N = num_cells / Vc::double_v::Size;
  class AosoaCell {
  public:
    using real_v = Vc::double_v;

    const real_v& GetDiameter() const {
      return diameter_;
    }

    void ChangeVolume(const real_v& speed) {
      volume_ += speed  * 0.01;
      volume_ = Vc::iif(volume_ < 5.2359877E-7,
                       Vc::double_v(5.2359877E-7), volume_);

       // UpdateDiameter();
       // for (size_t j = 0; j < Vc::double_v::Size; j++) {
       //   vc_diameter[idx][j] = std::cbrt(vc_volume[idx][j] * 6 / 3.14);
       // }
    }

  private:
    real_v diameter_;
    real_v volume_;
    std::array<std::vector<Neurite>, real_v::Size> neurites_;
  };

  std::vector<AosoaCell, Vc::Allocator<AosoaCell> > cells(N);

// #pragma omp parallel for
  // for (size_t i = 0; i < N; i++) {
  //   cells.diameter[i] = Vc::double_v(30);
  //   cells.volume[i] = Vc::double_v(1);
  // }

  {
    Timing timing("vcAosoa", statistic);
// #pragma omp parallel for default(none) shared(vc_diameter, vc_volume) firstprivate(iterations)
    for (size_t i = 0; i < N; i++) {
      auto& cell = cells[i];
      auto ifresult = cell.GetDiameter() <= 40;
      Vc::double_v dv(300);
      dv.setZeroInverted(ifresult);
      cell.ChangeVolume(dv);

      // auto ifresult = cells.GetDiameter(i) <= 40;
      // Vc::double_v dv(300);
      // dv.setZeroInverted(ifresult);
      // cells.ChangeVolume(i, dv);
    }
  }
}