#!/usr/bin/env python3
"""Separate rewrite, real-AST extraction, candidate check and conversion stages.

No existing MiniMLIR files are opened. Only frontend toolchain parameter utilities
are reused; generation is performed exclusively by buddy-extract-minimlir.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import sys

ROOT=Path(__file__).resolve().parent
FRONTEND=ROOT.parent/'FramaClangFrontend'
sys.path.insert(0,str(FRONTEND))
from experiment.paths import FRONTEND_LLVM, executable_path
from experiment.toolchain import compiler_arguments, compiler_environment, mini_arguments, run_command
from python.diagnostics import diagnose


def main():
    """Run one current candidate. Exit 0 only if every validation stage passes.

    Args are explicit source/build-environment paths; --input bypasses rewriting.
    Overwrite is allowed only for an output previously owned by this workflow.
    No pass-specific declaration or type handling occurs in Python.
    """
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True,help='Original source matching compile_commands.json')
    p.add_argument('--input',type=Path,help='Already rewritten.cpp; copied byte-for-byte')
    p.add_argument('--rules',default='namespace,guard')
    p.add_argument('--profile',default='matmul-scalar',help='Rewrite frontend profile only')
    p.add_argument('--compile-commands',type=Path,required=True)
    p.add_argument('--llvm-prefix',type=Path,default=FRONTEND_LLVM)
    p.add_argument('--output-dir',type=Path,default=ROOT/'current/tosa-matmul')
    p.add_argument('--tool',type=Path,default=FRONTEND/'build-llvm18/ExtractMiniMLIR/buddy-extract-minimlir')
    p.add_argument('--frama-c',type=Path,default=executable_path('FRAMA_C','frama-c'))
    p.add_argument('--converter',type=Path,default=executable_path('FRAMA_CIRGEN','framaCIRGen'))
    a=p.parse_args()
    out=a.output_dir.resolve()
    # Do not erase any arbitrary caller directory; own exactly one current result.
    protected=[ROOT,ROOT.parent,FRONTEND,ROOT.parent/'MiniMLIR',a.source.resolve().parent]
    if any(out==v or out in v.parents for v in protected):p.error('Unsafe output directory')
    if a.input and (out==a.input.resolve() or out in a.input.resolve().parents):p.error('Input is inside output')
    if out.exists() and any(out.iterdir()):
        if not (out/'.extract-owned').is_file():p.error('Nonempty output lacks ownership marker')
        shutil.rmtree(out)
    out.mkdir(parents=True,exist_ok=True);(out/'.extract-owned').write_text('ExtractMiniMLIR current result\n')
    (out/'logs').mkdir()
    flags,cwd,_=compiler_arguments(a.compile_commands,a.source,a.llvm_prefix)
    env=compiler_environment(a.llvm_prefix)
    summary={'stages':{},'generation_source':'real Clang AST only',
             'semantic_equivalence_proved':False,'callback_execution_model':False}
    def save(): (out/'summary.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2)+'\n')
    def stage(name,cmd,conversion=False):
        item=run_command(cmd,out/'logs'/f'{name}.log',cwd=cwd,
                         env=compiler_environment(a.llvm_prefix,a.frama_c) if conversion else env)
        summary['stages'][name]=item;save();return item['exit_code']==0
    source=out/'rewritten.cpp'
    if a.input:
        shutil.copyfile(a.input,source)
        summary['stages']['rewrite']={'status':'existing input copied unchanged'}
    else:
        if not stage('rewrite',[FRONTEND/'build-llvm18/buddy-framac-rewrite',a.source,
              '--output',source,'--report',out/'rewrite.json','--profile',a.profile,'--rules',a.rules,'--',*flags]):return 1
    summary['rewritten_sha256']=hashlib.sha256(source.read_bytes()).hexdigest();save()
    if not stage('real_clang',[a.llvm_prefix/'bin/clang++',*flags,'-fsyntax-only',source]):return 1
    if not stage('extract',[a.tool,source,'--output-dir',out,'--',*flags]):return 1
    include=out/'include'
    # Only redirect literal include paths, never copy macro/header source contents.
    # Macro includes are not guessed. Missing redirects appear as check diagnostics.
    includes=re.findall(r'^\s*#\s*include\s*[<"]([^>"\n]+)[>"]',source.read_text(),re.M)
    for name in includes:
        rel=Path(name)
        if rel.is_absolute() or '..' in rel.parts:raise ValueError('Unsafe include redirect: '+name)
        dest=include/rel;dest.parent.mkdir(parents=True,exist_ok=True)
        dest.write_text('#pragma once\n#include "skeleton.h"\n')
    summary['include_redirects']=includes
    generated_flags=mini_arguments(flags,include)
    check=stage('candidate_clang',[a.llvm_prefix/'bin/clang++',*generated_flags,'-fsyntax-only','-ferror-limit=0',source])
    records=json.loads((out/'records.json').read_text())
    summary['unsupported_count']=len(records['unsupported'])
    (out/'unsupported.json').write_text(json.dumps(diagnose(records,(out/'logs/candidate_clang.log').read_text()),ensure_ascii=False,indent=2)+'\n')
    # Try the converter even on an incomplete candidate, retaining its own result.
    converted=stage('frama_clang',[a.frama_c,source,'-cxx-nostdinc','-machdep','gcc_x86_64',
              '-cxx-clang-command='+str(a.converter),'-fclang-cpp-extra-args='+shlex.join(generated_flags),
              '-cxx-parseable-output','-print','-ocode',out/'converted.c'],True)
    if converted and (out/'converted.c').is_file():
        readback=stage('c_readback',[a.frama_c,'-machdep','gcc_x86_64',out/'converted.c','-print','-ocode','/dev/null'],True)
    else:
        readback=False;summary['stages']['c_readback']={'status':'not run: no successfully converted C'}
    summary['all_validation_stages_pass']=check and converted and readback
    save()
    (out/'commands.json').write_text(json.dumps(summary['stages'],ensure_ascii=False,indent=2)+'\n')
    print(json.dumps({k:v.get('exit_code',v.get('status')) for k,v in summary['stages'].items()},ensure_ascii=False,indent=2))
    return 0 if summary['all_validation_stages_pass'] else 1

if __name__=='__main__':sys.exit(main())
