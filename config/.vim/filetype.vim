if exists("did_load_filetypes")
    finish
endif
augroup filetypedetect
    au BufRead,BufNewFile *.camkes setfiletype camkes
    au BufRead,BufNewFile *.cdl setfiletype capdl
    au BufRead,BufNewFile *.idl4 setfiletype idl4
    au BufRead,BufNewFile *.tmux.conf setfiletype tmux
    au BufRead,BufNewFile *.scala setfiletype scala
    au BufRead,BufNewFile *.thy setfiletype isabelle
    au BufRead,BufNewFile *.go set filetype=go
    au BufRead,BufNewFile *.less setfiletype less
    au BufRead,BufNewFile *.md setfiletype mkd
    au BufRead,BufNewFile *.pml setfiletype promela
    au BufRead,BufNewFile *.promela setfiletype promela
    au BufRead,BufNewFile ROOT setfiletype root
    au BufRead,BufNewFile CMakeLists setfiletype cmake
    au BufRead,BufNewFile *.ml setfiletype sml
    au BufRead,BufNewFile *.ML setfiletype sml
    au BufRead,BufNewFile *.bf setfiletype bf
    au BufRead,BufNewFile *.pbf setfiletype bf
    au BufRead,BufNewFile *.sal setfiletype sal
augroup END
