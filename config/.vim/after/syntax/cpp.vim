" Extension to C++ syntax
" Language: C++
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

syn match cppCast "\<\(const\|static\|dynamic\|reinterpret\)_pointer_cast\s*<"me=e-1
syn match cppCast "\<\(const\|static\|dynamic\|reinterpret\)_pointer_cast\s*$"

" LLVM hacking
syn keyword cppCast isa cast dyn_cast cast_or_null dyn_cast_or_null
