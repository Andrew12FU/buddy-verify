// Nested reference captures must continue to modify the original object.
struct Box {
  int value;
  void add(int n) { value += n; }
};
template <class F> void now(F f) { f(); }
int main() {
  Box box{1};
  now([&]() { now([&]() { box.add(6); }); });
  return box.value != 7;
}
