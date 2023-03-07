
#define RECORD_SIZE (60 * 12 * 2)
// 2 samples per minutes = 12 hr

template <typename T>
class TimeSeries {
 public:
  TimeSeries() : _data() {}
  void add(T value) {
    _data[_index] = value;
    _index = (_index + 1) % RECORD_SIZE;
  }
  T get(int index) const { return _data[index]; }
  int size() const { return RECORD_SIZE; }
  int index() const { return _index; }

 private:
  T _data[RECORD_SIZE];
}
