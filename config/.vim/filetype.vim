if exists("did_load_filetypes")
    finish
endif
augroup filetypedetect
    au BufRead,BufNewFile *.camkes setfiletype camkes
    au BufRead,BufNewFile *.idl4 setfiletype idl4
    au BufRead,BufNewFile *.tmux.conf setfiletype tmux
    au BufRead,BufNewFile *.scala setfiletype scala
    au BufRead,BufNewFile *.thy setfiletype isabelle
    au BufRead,BufNewFile *.go set filetype=go
    au BufRead,BufNewFile *.md setfiletype mkd
augroup END
