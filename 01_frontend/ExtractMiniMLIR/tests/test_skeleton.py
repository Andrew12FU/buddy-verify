#!/usr/bin/env python3
"""End-to-end independent generation tests. No existing mini headers are read."""
import argparse
import json
import os
import sys
from pathlib import Path
import subprocess
import tempfile
import unittest

p=argparse.ArgumentParser()
p.add_argument('--tool',required=True)
p.add_argument('--clang',required=True)
p.add_argument('--frama-c')
p.add_argument('--converter')
args=p.parse_args()
smoke_env=os.environ.copy()
if args.frama_c:
    sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'FramaClangFrontend'))
    from experiment.toolchain import compiler_environment
    smoke_env=compiler_environment(Path(args.clang).resolve().parents[1],Path(args.frama_c))

class SkeletonTest(unittest.TestCase):
    def test_incremental_types_members_and_implicit_operations(self):
        """A later member use upgrades pointer-only records; inherited methods flatten.

        Verify name collisions, exact overload IDs, enum/alias kinds, conversions,
        accessed fields and const signatures independently of compilation success.
        """
        with tempfile.TemporaryDirectory() as tmp:
            r=Path(tmp); real=r/'real';real.mkdir();out=r/'out'
            (real/'api.h').write_text('''#pragma once
namespace other { struct Handle { int unused; }; }
namespace api {
struct Shape { long size() const; };
struct Base { Shape getShape() const; int unused(); };
struct Target {};
struct Tensor : Base { Tensor(); explicit operator bool() const;
 operator Target() const; int index; void select(int); void select(double); };
struct Handle { int key() const; };
enum class Mode : unsigned { A=1, B=2 };
using Count = long;
void consume(Target); void opaque(other::Handle*);
}
''')
            source=r/'input.cpp'
            source.write_text('''#include "api.h"
static_assert(true);
void run(api::Tensor &tensor, api::Handle *p, other::Handle *q) {
  api::Count n = tensor.getShape().size();
  if (tensor) tensor.index = p->key();
  tensor.select(1); tensor.select(1.0);
  api::consume(tensor); api::opaque(q);
  api::Mode m = api::Mode::B;
  api::Tensor local;
}
''')
            def run(cmd):
                result=subprocess.run(list(map(str,cmd)),capture_output=True,text=True,env=smoke_env)
                self.assertEqual(result.returncode,0,result.stdout+'\n'+result.stderr)
            run([args.tool,source,'--output-dir',out,'--','-std=c++17','-I'+str(real)])
            facts=json.loads((out/'records.json').read_text())
            header=(out/'include/skeleton.h').read_text()
            self.assertNotIn('unused',header)
            self.assertIn('getShape',header)
            self.assertIn('operator bool() const',header)
            self.assertIn('enum class Mode',header)
            selects=[x['id'] for x in facts['interfaces'] if x['name']=='api::Tensor::select']
            self.assertEqual(len(set(selects)),2)
            flattened=[x for x in facts['interfaces'] if x.get('abstraction')=='flatten inherited member to receiver']
            self.assertTrue(flattened)
            self.assertTrue(any(t['name']=='other::Handle' and t['state']=='forward' for t in facts['types']))
            self.assertTrue(any(x.get('operation')=='construction/copy/move' for x in facts['interfaces']))
            self.assertTrue(any(x['kind']=='UserDefinedConversion' for x in facts['relations']))
            (out/'include/api.h').write_text('#include "skeleton.h"\n')
            run([args.clang,'-std=c++17','-fsyntax-only','-I'+str(out/'include'),source])
            if args.frama_c:
                import shlex
                flags=['-std=c++17','-I'+str(out/'include')]
                run([args.frama_c,source,'-cxx-nostdinc','-machdep','gcc_x86_64',
                     '-cxx-clang-command='+args.converter,
                     '-fclang-cpp-extra-args='+shlex.join(flags),
                     '-cxx-parseable-output','-print','-ocode',out/'converted.c'])
                run([args.frama_c,'-machdep','gcc_x86_64',out/'converted.c',
                     '-print','-ocode','/dev/null'])

    def test_concrete_template_specialization(self):
        """Observed finite class specializations retain real template identity."""
        with tempfile.TemporaryDirectory() as tmp:
            r=Path(tmp);real=r/'real';real.mkdir()
            (real/'api.h').write_text('namespace api { template<class T> struct Box { T get() const; }; }')
            source=r/'input.cpp';source.write_text('#include "api.h"\nint f(api::Box<int> &b) { return b.get(); }')
            subprocess.run([args.tool,str(source),'--output-dir',str(r/'out'),'--','-std=c++17','-I'+str(real)],check=True,capture_output=True)
            inc=r/'out/include';(inc/'api.h').write_text('#include "skeleton.h"\n')
            header=(inc/'skeleton.h').read_text()
            self.assertIn('template <> struct Box<int>',header)
            subprocess.run([args.clang,'-std=c++17','-fsyntax-only','-I'+str(inc),str(source)],check=True,capture_output=True)

    def test_walk_records_empty_callback_boundary(self):
        """Only the explicitly supported MLIR walk shape receives an empty body."""
        with tempfile.TemporaryDirectory() as tmp:
            r=Path(tmp);real=r/'real';real.mkdir()
            (real/'api.h').write_text('namespace mlir { struct Walker { template<class Callback> void walk(Callback &&c) const & noexcept { c(1); } }; }')
            source=r/'input.cpp';original='#include "api.h"\nvoid f(mlir::Walker &w, int &n) { w.walk([&](int x) { n += x * 7; }); }'
            source.write_text(original)
            subprocess.run([args.tool,str(source),'--output-dir',str(r/'out'),'--','-std=c++17','-I'+str(real)],check=True,capture_output=True)
            inc=r/'out/include';(inc/'api.h').write_text('#include "skeleton.h"\n')
            facts=json.loads((r/'out/records.json').read_text())
            self.assertTrue(any(x['kind']=='callback_not_executed' for x in facts['relations']))
            self.assertEqual(source.read_text(),original)
            self.assertIn('callback is not executed',(inc/'skeleton.h').read_text())
            subprocess.run([args.clang,'-std=c++17','-fsyntax-only','-I'+str(inc),str(source)],check=True,capture_output=True)

    def test_template_is_reported_not_fabricated(self):
        """Unsupported templates must not silently become ordinary classes/functions."""
        with tempfile.TemporaryDirectory() as tmp:
            r=Path(tmp);(r/'api.h').write_text('template<class T, int N> struct Box { T get(); };')
            (r/'input.cpp').write_text('#include "api.h"\nint f(Box<int, 3> &b) { return b.get(); }')
            subprocess.run([args.tool,str(r/'input.cpp'),'--output-dir',str(r/'out'),'--','-std=c++17','-I'+str(r)],check=True,capture_output=True)
            facts=json.loads((r/'out/records.json').read_text())
            self.assertTrue(any(x['category']=='template_definition' for x in facts['unsupported']))
            self.assertNotIn('struct Box {',(r/'out/include/skeleton.h').read_text())

if __name__=='__main__': unittest.main(argv=['test_skeleton'])
