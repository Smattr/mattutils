" Vim syntax file
" Language:       Symbolic Analysis Library (SAL)
"                 http://sal.csl.sri.com/
" Maintainer:     Brandon Borkholder
" Filenames:      *.sal
" Last Change:    08 March 2007

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" By convention, keywords are uppercase, but SAL seems to allow both cases
syn case match

" SAL keywords
syn keyword SALbasicType                  NATURAL REAL BOOLEAN NZINTEGER INTEGER NZREAL
syn keyword SALtypeKeyword                TYPE DATATYPE END STATE
syn keyword SALarrayExpr                  ARRAY OF 
syn keyword SALconditional                IF THEN ELSIF ELSE ENDIF
syn keyword SALquantifier                 FORALL EXISTS
syn keyword SALupdateExpr                 WITH
syn keyword SALlambdaExpr                 LAMBDA
syn keyword SALletExpr                    LET
syn keyword SALinExpr                     IN
syn keyword SALlogicalOp                  AND OR NOT XOR
syn keyword SALarithmeticOp               DIV MOD
syn keyword SALbaseModule                 MODULE BEGIN END
syn keyword SALmoduleDecl                 INPUT OUTPUT GLOBAL LOCAL DEFINITION INITIALIZATION TRANSITION
syn keyword SALmoduleOp                   RENAME OBSERVE
syn keyword SALcontext                    CONTEXT
syn keyword SALassertion                  OBLIGATION CLAIM LEMMA THEOREM
syn keyword SALconstant                   TRUE FALSE
syn keyword SALtemporalOp                 AX AG AF EX EG EF X G F

syn cluster SALtype                       add=SALbasicType,SALrangeType,SALrecordType,SALsetType,SALfunctionType,SALconstant,SALnumber
syn cluster SALexpression                 add=SALlogicalOp,SALarithmeticOp,SALarithmetic,@SALtype,SALrelation,SALarrayExpr,SALquantifier
syn cluster SALrange                      add=SALrangeType,SALrangeDelimiter

" A TODO setting is always in every syntax file
syn keyword SALTodo                       contained TODO FIXME XXX

" SAL comments
syn region SALcomment                     start=/%/               end=/$/        contains=SALTodo

" SAL structures and types
syn region SALrecordType                  start=/\[#/             end=/#\]/      contains=@SALtype
syn region SALrangeType                   start=/\[[^#].*\.\./    end=/[^#]\]/   contains=@SALexpression
syn region SALfunctionType                start=/\[[^#].*->/      end=/[^#]\]/   contains=@SALtype
syn region SALsetType                     start=/{/               end=/}/        contains=@SALexpression
syn region SALrecordLiteral               start=/(#/              end=/#)/

" SAL operators and relations
syn match SALrelation                     /\(=\|\/=\|=>\|<\|[^-]>\|<=\|>=\)/
syn match SALmoduleCompos                 /||\|\[\]/
syn match SALtransition                   /\(:=\|-->\)/

" Numbers
syn match SALnumber                       /\<\d*\>/

" Arithmetic operations
syn match SALarithmetic                   /\(*\|+\|\/[^=]\|-[^->]\)/

" next' variables
syn match SALnextVariable                 /\<\w\+\>'/

" Theorem declarations
syn match SALtheoremDeclaration           /\w\+ *: *\(OBLIGATION\|CLAIM\|THEOREM\|LEMMA\)/ contains=theoremName,assertion
syn match SALtheoremName                  /\w\+/ contained nextgroup=assertion

" Define new highlight groups
hi          SALRelationSyntax             guifg=Red ctermfg=Red

" Now set the colors
hi link     SALbasicType                  Type
hi link     SALarrayExpr                  Type
hi link     SALtypeKeyword                Define
hi link     SALconditional                Conditional
hi link     SALquantifier                 Keyword
hi link     SALupdateExpr                 Keyword
hi link     SALlambdaExpr                 Macro
hi link     SALletExpr                    Keyword
hi link     SALinExpr                     Keyword
hi link     SALlogicalOp                  Operator
hi link     SALarithmeticOp               Operator
hi link     SALbaseModule                 Define
hi link     SALmoduleDecl                 Define
hi link     SALmoduleOp                   Operator
hi link     SALcontext                    Define
hi link     SALassertion                  Underlined
hi link     SALconstant                   Boolean
hi link     SALtemporalOp                 Operator
hi link     SALcomment                    Comment
hi link     SALrecordType                 Structure
hi link     SALrangeType                  Constant
hi link     SALfunctionType               Function
hi link     SALsetType                    Constant
hi link     SALrecordLiteral              Constant
hi link     SALrelation                   Statement
hi link     SALmoduleCompos               Operator
hi link     SALtransition                 SALRelationSyntax
hi link     SALnumber                     Number
hi link     SALnextVariable               Identifier
hi link     SALtheoremDeclaration         Underlined
hi link     SALtheoremName                Underlined

" Set the current syntax
let b:current_syntax = "sal"

