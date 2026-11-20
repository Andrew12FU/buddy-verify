"""Imported pass rules: preserve overloads and conditional evaluation effects."""
import unittest
from rewrite_support import RewriteCase, rest

class ImportedPassTests(RewriteCase):
    def test_const_handle_keeps_readonly_overload_and_initializes_once(self):
        code = r'''
namespace mlir { struct Value { int n; }; }
int calls=0; mlir::Value make(){++calls;return {7};}
int read(const mlir::Value &v){return v.n;} int read(mlir::Value &){return -1;}
int main(){const mlir::Value v=make();return !(read(v)==7 && calls==1);}
'''
        output, report = self.rewrite(code, 'const-local', profile='depthwise-conv')
        self.assertEqual(len(report['edits']), 1)
        self.assertIn('const mlir::Value &v', output.read_text())
        self.compare_behavior(output)

    def test_borrowed_callback_preserves_referent_and_per_instance_member(self):
        # Reference parameter, scalar, class handle and this are all borrowed.
        # Two objects detect accidental globalization; callback mutation of the
        # reference parameter verifies that the original referent is retained.
        code = r'''
struct Handle { int value; };
struct Pattern {
 int width;
 int run(Handle &rewriter) const {
  int scalar=3; Handle local{2};
  auto invoke=[](auto callback){return callback();};
  return invoke([&](){++rewriter.value;return width+scalar+local.value;});
 }
};
int main(){Pattern a{10},b{20};Handle writer{0};
 return !(a.run(writer)==15 && b.run(writer)==25 && writer.value==2);}
'''
        # The adapter intentionally accepts direct expression statements only.
        code=code.replace('return invoke([&](){++rewriter.value;return width+scalar+local.value;});',
                          'int result=0; invoke([&](){++rewriter.value;result=width+scalar+local.value;return 0;});return result;')
        output, report=self.rewrite(code, 'capture', profile='builtin-transpose')
        self.assertEqual(sum(e['rule']=='capture' for e in report['edits']), 1)
        self.assertIn('this->width', output.read_text())
        self.compare_behavior(output)

    def test_borrowed_callback_rejects_unknown_this_method(self):
        code=r'''
template<typename F> void invoke(F f){f();}
struct Pattern {int query()const{return 1;} void run()const{invoke([&](){return query();});}};
'''
        _, report=self.rewrite(code, 'capture', expected=1, profile='builtin-transpose')
        self.assertTrue(report['rejections'])

    def test_array_ranges_share_call_scope_and_use_unique_names(self):
        code = r'''
#include <initializer_list>
using size_t=unsigned long;
namespace llvm { template<typename T> struct ArrayRef {
 const T *p; size_t n; ArrayRef(const T *p,size_t n):p(p),n(n){}
}; }
namespace mlir { struct Value { int n; };
struct ValueRange {const Value *p;size_t n;
 ValueRange(std::initializer_list<Value> a):p(a.begin()),n(a.size()){}
 ValueRange(llvm::ArrayRef<Value> a):p(a.p),n(a.n){}
}; }
int sum(mlir::ValueRange a,mlir::ValueRange b){return a.p[0].n+a.p[1].n+b.p[0].n;}
int main(){mlir::Value a{2},b{5};
 int total=sum(mlir::ValueRange{a,b},mlir::ValueRange{b});return total!=12;}
'''
        output, report = self.rewrite(code, 'init-list', profile='depthwise-conv')
        self.assertEqual(sum(e['rule']=='init-list' for e in report['edits']), 2)
        self.assertEqual(sum(e['rule']=='init-list-preamble' for e in report['edits']), 1)
        self.compare_behavior(output)

    def test_nested_metadata_lists_converge_without_wrong_outer_list(self):
        code = r'''
#include <initializer_list>
namespace llvm {template<typename T> struct ArrayRef {
 const T *p; unsigned long n;
 ArrayRef(const T *p,unsigned long n):p(p),n(n){}
 ArrayRef(std::initializer_list<T> a):p(a.begin()),n(a.size()){}
};}
namespace mlir {
 struct NamedAttribute {int n;};
 struct Builder {
 int getI32TensorAttr(llvm::ArrayRef<int> a){return a.p[0]+a.p[1];}
 NamedAttribute getNamedAttr(const char *,int n){return {n};}
 };
}
int sum(llvm::ArrayRef<mlir::NamedAttribute> a){return a.p[0].n+a.p[1].n;}
int main(){mlir::Builder b;
 int n=sum(llvm::ArrayRef<mlir::NamedAttribute>{
 b.getNamedAttr("a",b.getI32TensorAttr({1,2})),
 b.getNamedAttr("b",b.getI32TensorAttr({3,4}))});return n!=10;}
'''
        original=code
        total=0
        for _ in range(5):
            (self.root/"output.cpp").unlink(missing_ok=True)
            (self.root/"report.json").unlink(missing_ok=True)
            output, report=self.rewrite(code, 'init-list', profile='builtin-transpose')
            if not report['edits']:
                break
            total+=sum(e['rule']=='init-list' for e in report['edits'])
            code=output.read_text()
        else:
            self.fail('nested lists did not converge')
        self.assertEqual(total,3)
        (self.root/'source.cpp').write_text(original)
        self.compare_behavior(output)

    def test_list_argument_with_effectful_call_is_rejected(self):
        code = r'''
#include <initializer_list>
namespace mlir {struct Value {int n;};
struct ValueRange {ValueRange(std::initializer_list<Value>);};}
mlir::Value make();void consume(mlir::ValueRange);
void run(){consume(mlir::ValueRange{make()});}
'''
        _, report = self.rewrite(code, 'init-list', expected=1, profile='depthwise-conv')
        self.assertTrue(report['rejections'])

    def test_metadata_list_rejects_mutating_affine_operator(self):
        code=r'''
#include <initializer_list>
namespace llvm {template<typename T>struct ArrayRef {ArrayRef(std::initializer_list<T>);};}
namespace mlir {struct AffineExpr {int n; AffineExpr &operator=(const AffineExpr &);};}
void consume(llvm::ArrayRef<mlir::AffineExpr>);
void run(mlir::AffineExpr a,mlir::AffineExpr b){consume(llvm::ArrayRef<mlir::AffineExpr>{a=b});}
'''
        _, report=self.rewrite(code,'init-list',expected=1,profile='builtin-transpose')
        self.assertTrue(report['rejections'])

    def test_conditionally_evaluated_list_is_not_hoisted(self):
        code = r'''
#include <initializer_list>
namespace mlir {struct Value {int n;};
struct ValueRange {ValueRange(std::initializer_list<Value>);};}
void consume(mlir::ValueRange);
void run(bool flag,mlir::Value v){consume(flag ? mlir::ValueRange{v} : mlir::ValueRange{v});}
'''
        _, report = self.rewrite(code, 'init-list', expected=1, profile='depthwise-conv')
        self.assertTrue(report['rejections'])

    def test_context_constructor_preserves_other_inherited_overloads(self):
        code = r'''
namespace mlir { struct MLIRContext { int value; }; namespace memref { struct CopyOp {}; }
template<typename T> struct OpRewritePattern {
 int value;
 explicit OpRewritePattern(MLIRContext *p):value(p->value){}
 OpRewritePattern(MLIRContext *p,int extra):value(p->value+extra){}
}; }
struct EliminateMemRefCopyPattern : public mlir::OpRewritePattern<mlir::memref::CopyOp> {
 using mlir::OpRewritePattern<mlir::memref::CopyOp>::OpRewritePattern;
};
int main(){mlir::MLIRContext c{4};EliminateMemRefCopyPattern a(&c),b(&c,7);
 return !(a.value==4 && b.value==11);}
'''
        output, report = self.rewrite(code, 'constructor', profile='eliminate-copy')
        self.assertEqual(len(report['edits']), 1)
        self.assertIn('using mlir::OpRewritePattern', output.read_text())
        self.compare_behavior(output)

    def test_unbraced_else_if_does_not_evaluate_later_query(self):
        code = r'''
int calls=0;struct Handle{int v;explicit operator bool()const{return v!=0;}};
Handle query(int v){++calls;return {v};}
int choose(int a,int b){if(auto x=query(a))return x.v;else if(auto y=query(b))return y.v;else return 0;}
int main(){int x=choose(3,4);if(x!=3||calls!=1)return 1;
 calls=0;return !(choose(0,4)==4 && calls==2);}
'''
        output, _ = self.rewrite_to_fixpoint(code, 'if-condition', profile='gpu-memcpy')
        self.compare_behavior(output)

    def test_observable_temporary_cleanup_is_rejected(self):
        code = r'''
struct H{explicit operator bool()const{return true;}};
struct Temp{~Temp(){};operator H()const{return {};}};
int run(){if(H h=Temp())return 1;return 0;}
'''
        _, report = self.rewrite(code, 'if-condition', expected=1, profile='eliminate-copy')
        self.assertTrue(report['rejections'])

if __name__ == '__main__':
    unittest.main(argv=[__file__, *rest])
