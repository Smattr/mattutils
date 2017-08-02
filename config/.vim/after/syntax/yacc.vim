
" Extension to Yacc syntax for Bison support
" Language: Yacc
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

" Bison accepts %precedence in addition to %left, %right, etc
syn match yaccKey "^\s*%precedence\>" contained
