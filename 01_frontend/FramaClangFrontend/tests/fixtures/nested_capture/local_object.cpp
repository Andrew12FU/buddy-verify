// The inner pointer is declared inside the outer body, after its original local.
struct Box {
  int value;
  void add(int n) { value += n; }
};
template <class F> void now(F f) { f(); }
int main() {
  Box result{0};
  auto outer = [&]() {
    Box local{3};
    auto inner = [&]() { result.add(local.value); local.add(4); };
    inner();
    result.add(local.value);
  };
  outer();
  return result.value != 10;
}
