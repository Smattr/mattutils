" @LICENSE(NICTA_CORE)

" Vim syntax highlighting configuration for Isabelle ROOT files. Based on
" sections 3.1, 3.2 of the Isabelle System Manual.

" From section 3.1:
syn keyword RootKeyword description files in options session theories

" From ~~/etc/options
syn keyword RootOption browser_info document document_output document_variants
    \ document_graph show_question_marks names_long names_short names_unique
    \ pretty_margin thy_output_display thy_output_break thy_output_quotes
    \ thy_output_indent thy_output_source print_mode threads threads_trace
    \ parallel_proofs parallel_proofs_threshold proofs quick_and_dirty
    \ skip_proofs condition timing timeout editor_load_delay editor_input_delay
    \ editor_output_delay editor_update_delay editor_reparse_limit
    \ editor_tracing_messages editor_chart_delay auto_load_theories

syn region RootComment start="(\*" end="\*)"
syn region RootDescription start="{\*" end="\*}"
syn region RootString start='"' end='"'
syn match RootNumber "\m[0-9]\+"
syn match RootBool "\(true\|false\)"
syn keyword RootFormat pdf

hi def link RootKeyword Statement
hi def link RootOption Type
hi def link RootComment Comment
hi def link RootDescription Comment
hi def link RootString Constant
hi def link RootNumber Constant
hi def link RootBool Constant
hi def link RootFormat Constant
