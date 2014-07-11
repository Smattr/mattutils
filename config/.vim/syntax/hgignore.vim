syn match HgignoreComment "#.*$"
syn match HgignoreSyntax "syntax:"
syn keyword HgignoreKeyword regexp glob

hi def link HgignoreComment Comment
hi def link HgignoreKeyword Statement
hi def link HgignoreSyntax Statement
