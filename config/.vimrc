set autoindent
set smartindent
set tabstop=4
set shiftwidth=4
set expandtab
set hlsearch
set number
set ignorecase

cnoreabbrev W w
cnoreabbrev Q q
cnoreabbrev Wq wq
cnoreabbrev WQ wq
set display+=lastline
set bg=dark

" Isabelle syntax.
au BufRead,BufNewFile *.thy set filetype=isabelle
au! Syntax isabelle source $HOME/.vim/syntax/isabelle.vim
