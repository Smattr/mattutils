if exists("did_load_filetypes")
    finish
endif
augroup filetypedetect
    au BufRead,BufNewFile *.camkes setfiletype camkes
    au BufRead,BufNewFile *.idl4 setfiletype idl4
    au BufRead,BufNewFile *.tmux.conf setfiletype tmux
augroup END
