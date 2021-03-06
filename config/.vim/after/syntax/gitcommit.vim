" switch Git comment character from '#' to ';' (see ../../../.gitconfig)
syn clear gitcommitComment
syn match gitcommitComment "^;.*"

" reset text width to the default (suppress vendor patches that change this)
set tw&
