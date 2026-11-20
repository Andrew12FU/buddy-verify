"""Executable semantic checks for range desugaring, not MLIR implementation mocks."""
import unittest
from rewrite_support import RewriteCase, rest

RANGE = r'''
struct View { int *p; int n; int *begin() const{return p;} int *end() const{return p+n;} };
'''

ENUMERATE = r'''
using size_t = unsigned long;
namespace llvm {
template<typename T> struct ArrayRef {
 T *p; size_t n; size_t size() const{return n;} T &operator[](size_t i)const{return p[i];}
};
template<typename T> struct Entry { size_t index; T &element; };
template<typename T> struct EnumIt { ArrayRef<T> a; size_t i;
 Entry<T> operator*()const{return {i,a[i]};} EnumIt &operator++(){++i;return *this;}
 bool operator!=(EnumIt other)const{return i!=other.i;} };
template<typename T> struct Enum { ArrayRef<T> a;
 EnumIt<T> begin(){return {a,0};} EnumIt<T> end(){return {a,a.size()};} };
template<typename T> Enum<T> enumerate(ArrayRef<T> a){return {a};}
}
'''

class RangeForTests(RewriteCase):
    def test_nested_unbraced_continue_break_and_single_evaluation(self):
        code = RANGE + r'''
int calls=0; int a[]={1,2,3}; View view(){++calls;return {a,3};}
int main(){int sum=0;for(int x:view()) for(int y:view()) {
 if(y==2)continue; if(x==3)break; sum+=x*y;
} return !(sum==12 && calls==4);}
'''
        output, report = self.rewrite(code, 'range-for', profile='assume-tight')
        self.assertEqual(len(report['edits']), 1)
        self.compare_behavior(output)

    def test_reference_binding_and_temporary_lifetime(self):
        code = RANGE + r'''
int live=0; struct Owned { int a[2]={3,4}; Owned(){++live;} ~Owned(){--live;}
 int *begin(){return a;} int *end(){return a+2;} };
int main(){int sum=0;for(int &x:Owned()){if(live!=1)return 1;x+=2;sum+=x;}
 return !(sum==11 && live==0);}
'''
        output, _ = self.rewrite(code, 'range-for', profile='assume-tight')
        self.compare_behavior(output)

    def test_early_increment_adapter_survives_body_invalidation(self):
        code = r'''
struct Node { Node *next; int value; };
struct It { Node *p; Node &operator*()const{return *p;}
 It &operator++(){p=p->next;return *this;} bool operator!=(It x)const{return p!=x.p;} };
struct Uses { Node *p; It begin(){return {p};} It end(){return {nullptr};} };
struct EarlyIt { mutable It it; mutable bool advanced=false;
 Node &operator*()const{Node &n=*it;++it;advanced=true;return n;}
 EarlyIt &operator++(){advanced=false;return *this;}
 bool operator!=(EarlyIt x)const{return it!=x.it;} };
struct Early { Uses uses; EarlyIt begin(){return {uses.begin()};} EarlyIt end(){return {uses.end()};} };
namespace llvm { Early make_early_inc_range(Uses u){return {u};} }
int main(){Node b={nullptr,2};Node a={&b,1};int sum=0;
 for(Node &n:llvm::make_early_inc_range(Uses{&a})){sum+=n.value;n.next=nullptr;}
 return sum!=3;}
'''
        output, _ = self.rewrite(code, 'range-for', profile='gpu-memcpy')
        self.assertIn('llvm::make_early_inc_range', output.read_text())
        self.compare_behavior(output)

    def test_enumerate_preserves_element_reference_and_original_view(self):
        code = ENUMERATE + r'''
int main(){int a[]={3,4};int other[]={100};llvm::ArrayRef<int> v{a,2};int sum=0;
 for(auto [idx,element]:llvm::enumerate(v)) {
   if(idx==0)v={other,1}; element+=2;sum+=element+idx;
 }
 return !(sum==12 && a[0]==5 && a[1]==6);}
'''
        output, _ = self.rewrite(code, 'range-for', profile='assume-tight')
        self.compare_behavior(output)

    def test_mutating_enumerate_proxy_index_is_rejected(self):
        code = ENUMERATE + 'int main(){int a[1]={1};llvm::ArrayRef<int> v{a,1};for(auto [idx,x]:llvm::enumerate(v))++idx;}'
        _, report = self.rewrite(code, 'range-for', expected=1, profile='assume-tight')
        self.assertIn('read-only index', report['rejections'][0])

    def test_macro_range_is_rejected(self):
        code = RANGE + '#define INPUT v\nint main(){int a[1]={1};View v{a,1};for(int x:INPUT){} }'
        _, report = self.rewrite(code, 'range-for', expected=1, profile='assume-tight')
        self.assertTrue(report['rejections'])

if __name__ == '__main__':
    unittest.main(argv=[__file__, *rest])
