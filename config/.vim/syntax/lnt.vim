" Quick and dirty syntax for Flint configs.
"
" FIXME:
"  * highlights -deprecate(...) as a pre-processor directive
"  * doesn't highlight -D"foo=bar baz"
"  * doesn't handle nested brackets

syn match LntComment1 "\/\*\_.\{-}\*\/"
syn match LntComment2 "\/\/.*$"

syn match LntDef "-[dD][a-zA-Z_0-9]\+\(=[a-zA-Z_0-9]*\)\?"
syn match LntFlag "\(-format\)\@![-+][_a-ce-zA-CE-Z][_a-zA-Z0-9]*"
syn region LntOption start="(" end=")"
syn match LntFormat "-format="
syn match LntFormatValue "\(-format=\)\@<=[^ \t]*"

hi def link LntComment1 Comment
hi def link LntComment2 Comment
hi def link LntFlag Type
hi def link LntDef PreProc
hi def link LntFormat Type
hi def link LntFormatValue Constant
hi def link LntOption Statement
