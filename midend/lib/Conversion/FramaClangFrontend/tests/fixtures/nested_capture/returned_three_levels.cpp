// Each returned closure owns a pointer value, never a reference to a pointer local.
struct Box {
  int value;
  void add(int n) { value += n; }
};
int main() {
  Box box{1};
  auto middle = [&]() {
    return [&]() {
      return [&]() { box.add(8); };
    };
  }();
  auto leaf = middle();
  leaf();
  return box.value != 9;
}
