// A self-contained input for the standalone --input workflow.
namespace {
int adjust(int input) {
  if (input < 0 || input == 0)
    return 1;
  return input + 2;
}
}
int main() { return adjust(1) != 3; }
