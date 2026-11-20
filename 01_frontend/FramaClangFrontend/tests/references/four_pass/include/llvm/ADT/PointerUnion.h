#ifndef FC_LLVM_ADT_POINTERUNION_H
#define FC_LLVM_ADT_POINTERUNION_H

namespace llvm {

/* 官方 PointerUnion 是变参模板；模型只用到 OpFoldResult 的
 * `PointerUnion<Attribute, Value>` 两元形式，按"只定义用到的"收窄到两个类型。 */
template <typename PT0, typename PT1> class PointerUnion {
public:
  PointerUnion();
  PointerUnion(PT0 V);
  PointerUnion(PT1 V);

private:
  void *Ptr;
  int Tag;
};

} // namespace llvm
#endif
