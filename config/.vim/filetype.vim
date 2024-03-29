if exists("did_load_filetypes")
    finish
endif
augroup filetypedetect
    au BufRead,BufNewFile *.c_pp setfiletype c
    au BufRead,BufNewFile *.camkes setfiletype camkes
    au BufRead,BufNewFile *.capnp setfiletype capnp
    au BufRead,BufNewFile *.cdl setfiletype capdl
    au BufRead,BufNewFile *.idl4 setfiletype idl4
    au BufRead,BufNewFile *.tmux.conf setfiletype tmux
    au BufRead,BufNewFile *.scala setfiletype scala
    au BufRead,BufNewFile *.thy set conceallevel=2
    au BufRead,BufNewFile *.go set filetype=go
    au BufRead,BufNewFile *.gvpr setfiletype gvpr
    au BufRead,BufNewFile *.less setfiletype less
    au BufRead,BufNewFile *.md setfiletype mkd
    au BufRead,BufNewFile *.pml setfiletype promela
    au BufRead,BufNewFile *.promela setfiletype promela
    au BufRead,BufNewFile ROOT setfiletype root
    au BufRead,BufNewFile CMakeLists,CMakeLists.txt setfiletype cmake
    au BufRead,BufNewFile *.ml setfiletype sml
    au BufRead,BufNewFile *.ML setfiletype sml
    au BufRead,BufNewFile *.nanorc setfiletype nanorc
    au BufRead,BufNewFile *.bf setfiletype bf
    au BufRead,BufNewFile *.pbf setfiletype bf
    au BufRead,BufNewFile *.rs setfiletype rust
    au BufRead,BufNewFile *.sal setfiletype sal
    au BufRead,BufNewFile *.toml setfiletype toml
    au BufRead,BufNewFile .hgignore setfiletype hgignore
    au BufRead,BufNewFile *.lds setfiletype ld
    au BufRead,BufNewFile *.bsh setfiletype java
    au BufRead,BufNewFile *.conf setfiletype dosini
    au BufRead,BufNewFile SConstruct setfiletype python
    au BufRead,BufNewFile *.lnt setfiletype lnt
    au BufRead,BufNewFile *.grako setfiletype grako
augroup END
