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

" Map Up and Down the way you would expect them to work with wrapped lines.
imap <silent> <Down> <C-o>gj
imap <silent> <Up> <C-o>gk
nmap <silent> <Down> gj
nmap <silent> <Up> gk
imap <silent> <End> <C-o>g<End>
imap <silent> <Home> <C-o>g<Home>
nmap <silent> <End> g<End>
nmap <silent> <Home> g<Home>
