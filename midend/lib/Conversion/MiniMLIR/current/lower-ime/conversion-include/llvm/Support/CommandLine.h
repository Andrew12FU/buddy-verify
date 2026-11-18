// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Bounded external declarations: see docs/FOUR_PASS_INTERFACES.md.
#ifndef BUDDY_MINI_LLVM_SUPPORT_COMMANDLINE_H
#define BUDDY_MINI_LLVM_SUPPORT_COMMANDLINE_H
#include "mlir/Support/LLVM.h"
namespace llvm { namespace cl {
class Option { public: virtual ~Option(); };
template<typename T> class parser;
 
 
 
template<typename DataType,bool ExternalStorage,bool isClass> class opt_storage;
template<typename DataType> class opt_storage<DataType,false,false> { DataType Value; public: operator DataType() const; };
template<typename DataType,bool ExternalStorage=false,typename ParserClass=parser<DataType>>
class opt : public Option,public opt_storage<DataType,ExternalStorage,std::is_class<DataType>::value> {
public: template<typename T> DataType &operator=(const T &value);
 template<typename T> DataType &operator=(T &&value);
};
} }

#endif
