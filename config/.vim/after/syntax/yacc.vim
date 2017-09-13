" Extension to Yacc syntax for Bison support
" Language: Yacc
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

" Bison accepts %precedence in addition to %left, %right, etc
syn match yaccKey "^\s*%precedence\>" contained

" Extra code blocks that Bison supports
" TODO: Java support
syn	region	yaccHeader2	matchgroup=yaccSep	start="^\s*\zs%\(code\(\s\+\(requires\|provides\|top\)\)\?\|destructor\|initial-action\|printer\)\s*{"	end="^\s*}"		contains=@yaccCode	nextgroup=yaccInit	skipwhite skipempty contained
syn	region	yaccHeader	matchgroup=yaccSep	start="^\s*\zs%\(code\(\s\+\(requires\|provides\|top\)\)\?\|destructor\|initial-action\|printer\)\s*{"	end="^\s*}"		contains=@yaccCode	nextgroup=yaccInit	skipwhite skipempty

" Other miscellanea
syn	match	yaccParseParam	'%param\>'		skipwhite	nextgroup=yaccParseParamStr
syn match yaccParseOption '%\(\(glr-parser\|debug\|no-lines\|token-table\|verbose\|yacc\)\s*$\|defines\>\(\s*"[^"]*"\)\?\|\(file-prefix\|language\|name-prefix\|output\|require\|skeleton\)\s"[^"]*"\|expect\(-rr\)\?\s\+\d\+\)'

" Location marker
" FIXME: This doesn't seem to work
syn match yaccOper "@\$" contained

" XXX: The corresponding line in the shipped yacc.vim appears to incorrectly
" exclude yaccOper from the contains list. Override it to fixup.
syn	region	yaccAction	matchgroup=yaccCurly start="{" end="}" 	contains=@yaccCode,yaccOper,yaccVar		contained
