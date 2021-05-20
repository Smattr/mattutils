set autoindent
set smartindent
set tabstop=2
set shiftwidth=2
set expandtab
set hlsearch
set number
set ignorecase

" Fix typos. Ugh, there must be a better way.
cnoreabbrev W w
cnoreabbrev Q q
cnoreabbrev Wq wq
cnoreabbrev WQ wq
cnoreabbrev WQA wqa
cnoreabbrev WQa wqa
cnoreabbrev WqA wqa
cnoreabbrev Wqa wqa
cnoreabbrev wQA wqa
cnoreabbrev wQa wqa
cnoreabbrev wqA wqa

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

" Always have a status bar; even with only one file open.
set laststatus=2

" Search incrementally while doing an i-search.
set incsearch

" Bodhi's options for spell checking
nm <F7> :setlocal spell! spelllang=en_au<CR>
imap <F7> <C-o>:setlocal spell! spelllang=en_au<CR>
nm <F8> :setlocal spell! spelllang=en_us<CR>
imap <F8> <C-o>:setlocal spell! spelllang=en_us<CR>
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

function! ToggleSyntax()
  if exists('b:syntax_off')
    set syntax=on
    unlet b:syntax_off
  else
    set syntax=off
    let b:syntax_off = 1
  endif
endfunction
nm <F5> :call ToggleSyntax()<CR>
imap <F5> <C-o>:call ToggleSyntax()<CR>

" Enable our friend, the mouse. Though ttymouse supposedly supports pterm (my
" weapon of choice), xterm2 seems to work where pterm does not. Cheers,
" kristopolous@HN, for the tip.
set ttymouse=xterm2
" Only visual and normal modes so that we get pane resizing, text selection and
" middle button paste.
set mouse=vn

" Standard trick for saving edits to a file that you forgot to open with sudo.
cmap w!! w !sudo tee >/dev/null %

set foldmethod=syntax
set foldlevelstart=20

" XXX: I don't know why, but the combination of Tmux setting
" TERM=screen-256color, Vim autodetecting t_Co=256 and Pterm displaying things
" incorrectly means we need to set the following or visual highlighting is
" invisible.
hi Visual cterm=reverse

" Some less sane environments have syntax highlighting off by default, so force
" it on.
syntax on

" Force ruler on for some environments where it defaults to off.
set ruler

" Enable some nicer highlighting for Haskell.
let hs_highlight_delimiters=1
let hs_highlight_boolean=1
let hs_highlight_types=1
let hs_highlight_more_types=1
let hs_highlight_debug=1

" Enable C++ highlighting for Flex/Bison
let g:yacc_uses_cpp=1
let g:lex_uses_cpp=1

" Enable some extra languages for highlighting within RST code blocks.
let g:rst_syntax_code_list = ['vim', 'java', 'cpp', 'lisp', 'php', 'python',
  \ 'perl', 'sh']

" Support pasting without messing up indentation, clagged from Stack Overflow
let &t_ti = &t_ti . "\e[?2004h"
let &t_te = "\e[?2004l" . &t_te
function! XTermPasteBegin(ret)
set pastetoggle=<Esc>[201~
set paste
return a:ret
endfunction
map <expr> <Esc>[200~ XTermPasteBegin("i")
imap <expr> <Esc>[200~ XTermPasteBegin("")
vmap <expr> <Esc>[200~ XTermPasteBegin("c")
cmap <Esc>[200~ <nop>
cmap <Esc>[201~ <nop>

" disable RHEL “jump to the last cursor position”
if has("autocmd")
  if exists("#redhat#BufReadPost")
    au! redhat BufReadPost
  endif
endif

" Disable modelines (https://github.com/numirias/security/blob/master/doc/2019-06-04_ace-vim-neovim.md).
set nomodeline

" allow backspace to delete across everything
set bs=indent,eol,start
