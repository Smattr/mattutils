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

" Map Up and Down the way you would expect them to work with wrapped lines.
imap <silent> <Down> <C-o>gj
imap <silent> <Up> <C-o>gk
nmap <silent> <Down> gj
nmap <silent> <Up> gk
imap <silent> <End> <C-o>g<End>
imap <silent> <Home> <C-o>g<Home>
nmap <silent> <End> g<End>
nmap <silent> <Home> g<Home>

" Assume, if we're creating a new main.c, that we want some vaguely sane
" boilerplate.
autocmd! BufNewFile main.c silent! 0r ~/.vim/templates/main.%:e

" Always have a status bar; even with only one file open.
set laststatus=2

" Search incrementally while doing an i-search.
set incsearch

" Bodhi's options for spell checking
"set spell spelllang=en_au
"setlocal spell spelllang=en_au
nm <F7> :setlocal spell! spelllang=en_au<CR>
imap <F7> <C-o>:setlocal spell! spelllang=en_au<CR>
highlight clear SpellBad
highlight SpellBad term=standout ctermfg=1 term=underline cterm=underline
highlight clear SpellCap
highlight SpellCap term=underline cterm=underline
highlight clear SpellRare
highlight SpellRare term=underline cterm=underline
highlight clear SpellLocal
highlight SpellLocal term=underline cterm=underline
let g:spellfile_URL = 'http://ftp.vim.org/vim/runtime/spell'

" Toggle conceallevel setting. Useful for Isabelle.
function! ToggleConceal()
  if &conceallevel == 0
    set conceallevel=2
  else
    set conceallevel=0
  endif
endfunction
nm <F6> :call ToggleConceal()<CR>
imap <F6> <C-o>:call ToggleConceal()<CR>

function! ToggleIsabelleTex()
  if exists('g:isabelle_tex')
    let g:isabelle_tex = !g:isabelle_tex
  else
    let g:isabelle_tex=1
  endif
  syntax off
  syntax on
endfunction
nm <F8> :call ToggleIsabelleTex()<CR>
imap <F8> <C-o>:call ToggleIsabelleTex()<CR>

" Enable our friend, the mouse. Though ttymouse supposedly supports pterm (my
" weapon of choice), xterm2 seems to work where pterm does not. Cheers,
" kristopolous@HN, for the tip.
set ttymouse=xterm2
" Only visual and normal modes so that we get pane resizing, text selection and
" middle button paste.
set mouse=vn

" Standard trick for saving edits to a file that you forgot to open with sudo.
cmap w!! w !sudo tee >/dev/null %

" Enable Pathogen
execute pathogen#infect()

" YouCompleteMe tweaks
let g:ycm_autoclose_preview_window_after_completion = 1
let g:ycm_register_as_syntastic_checker = 0
let g:ycm_semantic_triggers = {
  \  'mkd':['@'],
  \ }
let g:ycm_filetype_blacklist = {}
