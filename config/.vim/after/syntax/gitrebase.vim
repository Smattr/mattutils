" Syntax highlighting for inline diffs inserted into Git's rebase picker by rerebase.
" Language: GIT_EDITOR=rerebase git rebase --interactive
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

syn match gitrebaseDiffCommit "\(^\s*# \)\@<=commit .*" containedin=gitrebaseComment
syn match gitrebaseDiffAuthor "\(^\s*# \)\@<=Author: .*" containedin=gitrebaseComment
syn match gitrebaseDiffDate "\(^\s*# \)\@<=Date: .*" containedin=gitrebaseComment
syn match gitrebaseDiffMessage "\(^\s*# \)\@<=    .*" containedin=gitrebaseComment
syn match gitrebaseDiffDiff "\(^\s*# \)\@<=diff .*" containedin=gitrebaseComment
syn match gitrebaseDiffIndex "\(^\s*# \)\@<=index .*" containedin=gitrebaseComment
syn match gitrebaseDiffPlusFile "\(^\s*# \)\@<=+++ .*" containedin=gitrebaseComment
syn match gitrebaseDiffMinusFile "\(^\s*# \)\@<=--- .*" containedin=gitrebaseComment
syn match gitrebaseDiffAt "\(^\s*# \)\@<=@@ .*" containedin=gitrebaseComment
syn match gitrebaseDiffPlus "\(^\s*# \)\@<=+\(++ \)\@!.*" containedin=gitrebaseComment
syn match gitrebaseDiffMinus "\(^\s*# \)\@<=-\(-- \)\@!.*" containedin=gitrebaseComment

" Instead of doing the usual hi def link, directly set colours to match Git's output. Note that we
" deliberately leave some matches unhighlighted (e.g. gitrebaseDiffAuthor).
hi gitrebaseDiffCommit ctermfg=Yellow
hi gitrebaseDiffDiff ctermfg=White
hi gitrebaseDiffIndex ctermfg=White
hi gitrebaseDiffPlusFile ctermfg=White
hi gitrebaseDiffMinusFile ctermfg=White
hi gitrebaseDiffAt ctermfg=Cyan
hi gitrebaseDiffPlus ctermfg=DarkGreen
hi gitrebaseDiffMinus ctermfg=DarkRed
