"""Behavior checks for structural rules across renamed APIs and profiles."""
import unittest
from rewrite_support import RewriteCase, rest

PROFILES = ('reshape', 'matmul-scalar', 'lower-ime', 'silu-fusion',
            'assume-tight', 'eliminate-copy', 'gpu-memcpy',
            'depthwise-conv', 'builtin-transpose')

class GeneralizedRules(RewriteCase):
    def all_profiles(self, code, rule, *, execute=True):
        outputs = []
        for profile in PROFILES:
            (self.root / 'output.cpp').unlink(missing_ok=True)
            (self.root / 'report.json').unlink(missing_ok=True)
            output, report = self.rewrite(code, rule, profile=profile,
                                          options=('--namespace-prefix', 'renamed'))
            self.assertTrue(report['edits'], profile)
            outputs.append(output.read_text())
            if execute:
                self.compare_behavior(output)
        self.assertTrue(all(x == outputs[0] for x in outputs), rule)
        return outputs[0]

    def test_namespace_has_no_pass_or_class_dependency(self):
        self.all_profiles('''
namespace outer { namespace {struct Renamed {int n;};} }
namespace outer { namespace {int read(Renamed v){return v.n;}} }
int main(){outer::Renamed x{7};return outer::read(x)!=7;}
''', 'namespace')

    def test_constructor_renaming_defaults_and_implicit_conversion(self):
        self.all_profiles(r'''
struct Token {int value;};
struct RenamedBase {
 int value;
 RenamedBase():value(9){}
 RenamedBase(Token *p,int extra=2):value(p->value+extra){}
 explicit RenamedBase(int n):value(n){}
};
struct RenamedPattern:RenamedBase {using RenamedBase::RenamedBase;};
int take(RenamedPattern p){return p.value;}
int main(){Token t{4};RenamedPattern a(&t),b(&t,7),c;
 return !(a.value==6 && b.value==11 && c.value==9 && take(&t)==6);}
''', 'constructor')

    def test_const_record_type_and_profile_independence(self):
        self.all_profiles(r'''
namespace user {template<class T> struct Box {T n;};}
int calls=0;user::Box<int> make(){++calls;return {7};}
int read(const user::Box<int>&x){return x.n;}
int read(user::Box<int>&){return -1;}
int main(){const user::Box<int> box=make();return !(read(box)==7&&calls==1);}
''', 'const-local')

    def test_const_record_decltype_observer_rejects(self):
        self.rewrite('struct H{int n;}; int main(){const H h{1};using T=decltype(h);return sizeof(T)==0;}',
                     'const-local', expected=1)

    def test_range_arrays_and_adl_keep_references_and_lookup(self):
        self.all_profiles(r'''
namespace renamed {struct View{int *p;};
int *begin(View &v){return v.p;} int *end(View &v){return v.p+2;}}
// Ordinary lookup must not replace range-for's selected ADL overload.
int *begin(renamed::View &){return nullptr;}
int main(){int a[2]={1,2};for(int &x:a)x+=2;
renamed::View v{a};int sum=0;for(int &x:v){++x;sum+=x;}
return !(sum==9 && a[0]==4 && a[1]==5);}
''', 'range-for')

    def test_list_elements_capacity_and_profiles_are_structural(self):
        self.all_profiles(r'''
#include <initializer_list>
namespace user {struct Item {int n;};}
namespace llvm {template<class T>struct ArrayRef {
const T*p;unsigned long n;
ArrayRef(std::initializer_list<T>x):p(x.begin()),n(x.size()){}
ArrayRef(const T*p,unsigned long n):p(p),n(n){}
};
template<class T,int N>struct SmallVector {
T a[2]; SmallVector(std::initializer_list<T>x){int i=0;for(auto &v:x)a[i++]=v;}
SmallVector(const T*b,const T*e){int i=0;for(;b!=e;++b)a[i++]=*b;}
};}
int read(llvm::ArrayRef<user::Item>x){return x.p[0].n+x.p[1].n;}
int main(){user::Item a{2},b{5};llvm::SmallVector<int,8> v{3,4};
int n=read(llvm::ArrayRef<user::Item>{a,b});return !(n==7&&v.a[0]+v.a[1]==7);}
''', 'init-list')

    def test_list_unknown_element_constructor_effect_rejects(self):
        self.rewrite(r'''
#include <initializer_list>
namespace llvm {template<class T>struct ArrayRef {
ArrayRef(std::initializer_list<T>);ArrayRef(const T*,unsigned long);};}
int calls=0;struct Item {int n;Item(int x):n(x){++calls;}};
void use(llvm::ArrayRef<Item>);
void f(){use(llvm::ArrayRef<Item>{Item(1)});}
''', 'init-list', expected=1)

    def test_value_range_any_callee_slot_count_and_profile(self):
        self.all_profiles(r'''
#include <initializer_list>
namespace mlir {struct Value{int n;};}
namespace llvm {template<class T>struct ArrayRef {const T*p;unsigned long n;
ArrayRef(const T*p,unsigned long n):p(p),n(n){};};}
namespace mlir {struct ValueRange {const Value*p;unsigned long n;
ValueRange(std::initializer_list<Value>a):p(a.begin()),n(a.size()){}
ValueRange(llvm::ArrayRef<Value>a):p(a.p),n(a.n){};};}
int consume(mlir::ValueRange x,int n){return x.p[0].n+n;}
int combine(int n,mlir::ValueRange x,mlir::ValueRange y,mlir::ValueRange z){
return n+x.p[0].n+y.p[0].n+z.p[0].n;}
int main(){mlir::Value a{2},b{3},c{5};
int x=consume({a},1);int y=combine(4,{a},{b},{c});return !(x==3&&y==14);}
''', 'value-range')

    def test_while_is_already_structural_across_profiles(self):
        self.all_profiles(r'''
struct Renamed {int n; explicit operator bool()const{return n<4;}
bool operator!()const{return true;}};
int main(){int calls=0,sum=0;
while(auto h=Renamed{++calls}){if(h.n==2)continue;sum+=h.n;}
return !(calls==4&&sum==4);}
''', 'while')

    def test_constructor_function_pointer_declarator_and_noexcept(self):
        self.all_profiles(r'''
int plus(int n){return n+1;}
struct Parent {int value;explicit Parent(int (*f)(int)) noexcept:value(f(3)){};};
struct Child:Parent{using Parent::Parent;};
static_assert(noexcept(Child(&plus)), "inherited exception specification");
int main(){Child c(&plus);return c.value!=4;}
''', 'constructor')

    def test_constructor_ambiguous_unused_base_overloads_reject(self):
        self.rewrite('struct T{};struct B{B(T*);B(T*,int=0);};struct D:B{using B::B;};',
                     'constructor', expected=1)

    def test_value_range_rejects_effectful_argument_wrappers(self):
        self.rewrite(r'''
#include <initializer_list>
namespace mlir {struct Value{int n;};struct ValueRange {
ValueRange(std::initializer_list<Value>);};}
struct Callback {template<class F>Callback(F f){f();}};
void consume(mlir::ValueRange,Callback);
void run(){mlir::Value v{1};consume({v},[&](){++v.n;});}
''', 'value-range', expected=1)

    def test_constructor_existing_defaulted_tail_overload_is_retained(self):
        code=r'''
struct T{};struct B{B(T*){}};
struct D:B{using B::B;D(T*p,int=0):B(p){}};
int main(){T t;D d(&t);}
'''
        output, report=self.rewrite(code, 'constructor')
        self.assertFalse(report['edits'])
        self.compare_behavior(output)

    def test_constructor_default_reference_temporary_keeps_full_expression(self):
        self.all_profiles(r'''
struct T{};int live=0;
struct Temp {Temp(){++live;}~Temp(){--live;}};
struct Base {Base(T*,const Temp& t={}){}};
struct Derived:Base{using Base::Base;};
int observe(const Derived&){return live;}
int main(){T t;int n=observe(Derived(&t));return !(n==1&&live==0);}
''', 'constructor')

    def test_constructor_private_parameter_type_rejects(self):
        self.rewrite('class B{struct Hidden{};public:B(Hidden*);};struct D:B{using B::B;};',
                     'constructor', expected=1)

    def test_constructor_scalar_signature_has_no_context_pointer_requirement(self):
        self.all_profiles(r'''
struct Parent {int n;explicit Parent(int a,double b=2.5)noexcept:n(a+int(b)){};};
struct Child:Parent{using Parent::Parent;};
static_assert(noexcept(Child(3)), "preserve exception specification");
int main(){Child a(3),b(3,7.0);return !(a.n==5&&b.n==10);}
''', 'constructor')

    def test_constructor_private_alias_rvalue_forwarding_uses_public_type(self):
        self.all_profiles(r'''
class Parent{using HiddenAlias=int;public:int n;
explicit Parent(HiddenAlias&&x):n(x){}};
struct Child:Parent{using Parent::Parent;};
int main(){Child c(7);return c.n!=7;}
''', 'constructor')

    def test_list_uses_ast_types_without_int64_alias_and_prints_function_pointers(self):
        self.all_profiles(r'''
#include <initializer_list>
namespace llvm {template<class T>struct ArrayRef {const T*p;unsigned long n;
ArrayRef(std::initializer_list<T>x):p(x.begin()),n(x.size()){}
ArrayRef(const T*p,unsigned long n):p(p),n(n){};};}
long read(llvm::ArrayRef<long>x){return x.p[0]+x.p[1];}
int plus(int n){return n+1;}
int invoke(llvm::ArrayRef<int(*)(int)>x){return x.p[0](3);}
int main(){long a=2,b=5;int(*f)(int)=plus;
long x=read({a,b});int y=invoke({f});return !(x==7&&y==4);}
''', 'init-list')

    def test_enumerate_does_not_require_a_global_size_t_alias(self):
        from test_range_for import ENUMERATE
        code=ENUMERATE.replace('using size_t = unsigned long;','')
        code=code.replace('namespace llvm {','namespace llvm {using size_t=unsigned long;',1)
        code+='int main(){int a[2]={3,4};llvm::ArrayRef<int> v{a,2};int n=0;for(auto [i,x]:llvm::enumerate(v))n+=i+x;return n!=8;}'
        self.all_profiles(code, 'range-for')

    def test_constructor_record_value_identity_is_not_assumed_from_trivial_copy(self):
        self.rewrite(r'''
struct Token{};struct Identity{Identity*self;Identity():self(this){}};
struct Base{bool same;Base(Token*,Identity value={}):same(value.self==&value){}};
struct Derived:Base{using Base::Base;};
int main(){Token t;Derived d(&t);return !d.same;}
''', 'constructor', expected=1)

    def test_constructor_uninstantiated_nondependent_default(self):
        output, report = self.rewrite("""
template<class T> struct Base { Base(int *p, unsigned count = 1) {} };
struct Derived : Base<int> { using Base<int>::Base; };
int main() { return 0; }
""", "constructor")
        self.assertEqual(len(report["edits"]), 1)
        self.assertIn("= 1", output.read_text())
        self.compare_behavior(output)

    def test_constructor_uninstantiated_dependent_default_rejected(self):
        self.rewrite("""
template<int N> struct Base { Base(int *p, int count = N) {} };
struct Derived : Base<2> { using Base<2>::Base; };
int main() { return 0; }
""", "constructor", expected=1)


if __name__ == '__main__':
    unittest.main(argv=[__file__, *rest])
