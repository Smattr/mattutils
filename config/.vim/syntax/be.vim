" Language: Bugs Everywhere comment
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

if exists("b:current_syntax")
  finish
endif

syn region beTrailer start="^== Anything below this line will be ignored$" 
  \ end="\%$" contains=beSubject,beComment
syn match beSubject "\(^Subject: \)\@<=.*$"
syn match beComment "\(^> \)\@<=.*$"

hi def link beTrailer Comment
hi def link beSubject Keyword

let b:current_syntax = "be"
