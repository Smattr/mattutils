" Vim syntax highlighting for THY files.
"
" Licensing: The original version of this file was released on the Isabelle
" user's list by Jens-Wolfhard Schicke <drahflow@gmx.de> in 2008 and
" subsequently extended by Timothy Bourke and Matthew Fernandez. It's unclear
" to me (Matt) what licence Jens intended and I disclaim any right to it
" myself, but note that this is not NICTA_CORE. Feel free to use it locally if
" it helps you.
"

syn clear
syn sync fromstart
syn case match

syn keyword IsabelleCommand term typ prop prf full_prf pr value
syn keyword IsabelleCommand abbreviation
syn keyword IsabelleCommand theory
syn keyword IsabelleCommand theorem schematic_theorem corollary
    \ schematic_corollary
syn keyword IsabelleCommand lemma
syn keyword IsabelleCommand lemmas
syn keyword IsabelleCommand schematic_lemma
syn keyword IsabelleCommand primrec
syn keyword IsabelleCommand datatype
syn keyword IsabelleCommand declare declaration syntax_declaration
syn keyword IsabelleCommand definition
syn keyword IsabelleCommand method_setup
syn keyword IsabelleCommand fun
syn keyword IsabelleCommand function termination
syn keyword IsabelleCommand typedecl
syn keyword IsabelleCommand types
syn keyword IsabelleCommand typedef
syn keyword IsabelleCommand type_synonym
syn keyword IsabelleCommand consts
syn keyword IsabelleCommand constdefs
syn keyword IsabelleCommand inductive
syn keyword IsabelleCommand inductive_set
syn keyword IsabelleCommand inductive_cases
syn keyword IsabelleCommand record
syn keyword IsabelleCommand defs
syn keyword IsabelleCommand axclass
syn keyword IsabelleCommand instance
syn keyword IsabelleCommand axioms
syn keyword IsabelleCommand axiomatization
syn keyword IsabelleCommand locale
syn keyword IsabelleCommand sublocale
syn keyword IsabelleCommand theorems
syn keyword IsabelleCommand class subclass
syn keyword IsabelleCommand interpretation interpret
syn keyword IsabelleCommand instantiation
syn keyword IsabelleCommand context
syn keyword IsabelleCommand rep_datatype
syn keyword IsabelleCommand export_code
syn keyword IsabelleCommand code_const
syn keyword IsabelleCommand ML_file
syn keyword IsabelleCommand setup
syn keyword IsabelleCommand thm
syn keyword IsabelleCommand print_theorems print_locale print_locales
    \ print_dependencies print_interps print_classes class_deps print_abbrevs
    \ print_statement print_trans_rules print_cases print_induct_rules
syn keyword IsabelleCommand notepad

" Do some juggling to give us ML highlighting inside an Isabelle/ML block.
if exists('b:current_syntax')
  let s:current_syntax=b:current_syntax
  unlet b:current_syntax
endif
syntax include @SML syntax/sml.vim
if exists('s:current_syntax')
  let b:current_syntax=s:current_syntax
else
  unlet b:current_syntax
endif
syntax region IsabelleCommand matchgroup=SpecialComment start="ML[ ]*{\*" end="\*}" contains=@SML


syn keyword IsabelleCommandPart and is
syn keyword IsabelleCommandPart assumes constrains defines shows fixes notes
    \ obtains
syn keyword IsabelleCommandPart where for
syn keyword IsabelleCommandPart begin end
syn keyword IsabelleCommandPart imports
syn keyword IsabelleCommandPart keywords uses
syn keyword IsabelleCommandPart monos overloaded
syn keyword IsabelleCommandMod code simp iff rule_format cong
syn match IsabelleCommandMod /\<intro\>!\?/
syn match IsabelleCommandMod /\<dest\>!\?/
syn keyword IsabelleCommandProofProve proof
syn keyword IsabelleCommandProofProve apply
syn keyword IsabelleCommandProofProve prefer defer back
syn keyword IsabelleCommandProofDone done by qed apply_end
syn keyword IsabelleCommandProofFail sorry oops
syn keyword IsabelleCommandProofIsar assume show have from then thus hence
    \ presume def
syn keyword IsabelleCommandProofIsar with next using note
syn keyword IsabelleCommandProofIsar let
syn keyword IsabelleCommandProofIsar moreover ultimately also finally
syn keyword IsabelleCommandProofIsar fix obtain where case guess
syn keyword IsabelleCommandMethod assumption fact this
syn keyword IsabelleCommandMethod rule erule drule frule
syn keyword IsabelleCommandMethod elim
syn match IsabelleCommandMethod /\<intro\>/
syn keyword IsabelleCommandMethod intro_classes intro_locales
syn keyword IsabelleCommandMethod rule_tac erule_tac drule_tac frule_tac
syn keyword IsabelleCommandMethod insert subst hypsubst
syn keyword IsabelleCommandMethod rename_tac rotate_tac
syn keyword IsabelleCommandMethod induct_tac ind_cases induct
syn keyword IsabelleCommandMethod coinduct
syn keyword IsabelleCommandMethod induct_scheme lexicographic_order relation
syn keyword IsabelleCommandMethod case_tac cases split
syn keyword IsabelleCommandMethod subgoal_tac
syn keyword IsabelleCommandMethod eval evaluation
syn keyword IsabelleCommandMethod fail succeed
syn keyword IsabelleCommandMethod atomize atomize_elim
syn keyword IsabelleCommandMethod neg_clausify finish_clausify
syn keyword IsabelleCommandMethod contradiction
syn keyword IsabelleCommandMethod cut_tac
syn keyword IsabelleCommandMethod fold unfold unfold_locales unfolding
syn keyword IsabelleCommandMethod normalization sring_norm
syn match IsabelleCommandMethodMod /\<add!\?:/
syn match IsabelleCommandMethodMod /\<del!\?:/
syn match IsabelleCommandMethodMod /\<only!\?:/
syn match IsabelleCommandMethodMod /\<dest!\?:/
syn match IsabelleCommandMethodMod /\<intro!\?:/
syn match IsabelleCommandMethodMod /\<split!\?:/
syn match IsabelleCommandMethodMod /\<cong!\?:/
syn match IsabelleCommandMethodMod /\<arbitrary!\?:/
syn keyword IsabelleCommandMethodMod in no_asm_use
syn keyword IsabelleCommandMethodMod thin_tac
syn keyword IsabelleCommandBigMethod simp simp_all
syn keyword IsabelleCommandBigMethod blast force auto fast best slow deepen fastforce
syn keyword IsabelleCommandBigMethod unat_arith arith algebra
syn keyword IsabelleCommandBigMethod bestsimp fastsimp simplesubst slowsimp
syn keyword IsabelleCommandBigMethod clarify safe clarsimp default
syn keyword IsabelleCommandBigMethod eprover eproverF eproverH
syn keyword IsabelleCommandBigMethod iprover
syn keyword IsabelleCommandBigMethod metis metisF metisH
syn keyword IsabelleCommandBigMethod meson order pat_completeness
syn keyword IsabelleCommandBigMethod presburger
syn keyword IsabelleCommandBigMethod rtrancl rtranclp trancl tranclp
syn keyword IsabelleCommandBigMethod sat satx
syn keyword IsabelleCommandBigMethod spass spassF spassH
syn keyword IsabelleCommandBigMethod tactic raw_tactic
syn keyword IsabelleCommandBigMethod vampire vampireF vampireH
syn keyword IsabelleCommandRule conjI conjE conjunct1 conjunct2
syn keyword IsabelleCommandRule disjI1 disjI2 disjE disjCI
syn keyword IsabelleCommandRule notI notE
syn keyword IsabelleCommandRule impI
syn keyword IsabelleCommandRule mp
syn keyword IsabelleCommandRule ssubst subst
syn keyword IsabelleCommandRule contrapos_np contrapos_nn
syn keyword IsabelleCommandRule contrapos_pp contrapos_pn
syn keyword IsabelleCommandRule allI allE spec
syn keyword IsabelleCommandRule exI exE
syn keyword IsabelleCommandRule the_equality
syn keyword IsabelleCommandRule some_equality someI someI2 someI_ex
syn keyword IsabelleCommandRule order_antisym
syn keyword IsabelleCommandRule sym
syn keyword IsabelleCommandRule iffD1 iffD2
syn keyword IsabelleCommandRule arg_cong
syn keyword IsabelleCommandRule mult_le_mono1
syn keyword IsabelleCommandRule mod_Suc
syn keyword IsabelleCommandRule mod_div_equality
syn keyword IsabelleCommandRule dvd_mod_imp_dvd dvd_mod dvd_trans
syn keyword IsabelleCommandRule IntI IntD1 IntD2
syn keyword IsabelleCommandRule Compl_iff Compl_Un Compl_partition
syn keyword IsabelleCommandRule Diff_disjoint
syn keyword IsabelleCommandRule subsetI subsetD
syn keyword IsabelleCommandRule Un_subset_iff
syn keyword IsabelleCommandRule set_ext
syn keyword IsabelleCommandRule equalityI equalityE
syn keyword IsabelleCommandRule insert_is_Un
syn keyword IsabelleCommandRule mem_Collect_eq Collect_mem_eq
syn keyword IsabelleCommandRule ballI bspec bexI bexE
syn keyword IsabelleCommandRule UN_iff UN_I UN_E Union_iff
syn keyword IsabelleCommandRule INT_iff Inter_iff
syn keyword IsabelleCommandRule card_Un_Int card_Pow
syn keyword IsabelleCommandRule n_subsets
syn keyword IsabelleCommandRule ext id_def o_def o_assoc
syn keyword IsabelleCommandRule fun_upd_apply fun_upd_upd
syn keyword IsabelleCommandRule inj_on_def surj_def bij_def
syn keyword IsabelleCommandRule inv_f_f surj_f_inv_f inv_inv_eq
syn keyword IsabelleCommandRule expand_fun_eq
syn keyword IsabelleCommandRule image_def image_compose image_Un image_Int
syn keyword IsabelleCommandRule vimage_def vimage_Compl
syn keyword IsabelleCommandRule Id_def rel_comp_def R_O_Id rel_comp_mono
syn keyword IsabelleCommandRule converse_iff Image_iff Domain_iff Range_iff
syn keyword IsabelleCommandRule rtrancl_unfold rtrancl_refl
syn keyword IsabelleCommandRule r_into_rtrancl rtrancl_trans
syn keyword IsabelleCommandRule rtrancl_induct rtrancl_idemp
syn keyword IsabelleCommandRule converse_rtrancl_induct
syn keyword IsabelleCommandRule trancl_trans trancl_converse
syn keyword IsabelleCommandRule less_than_iff wf_less_than
syn keyword IsabelleCommandRule inv_image_def wf_inv_image
syn keyword IsabelleCommandRule measure_def wf_measure
syn keyword IsabelleCommandRule lex_prod_def wf_lex_prod
syn keyword IsabelleCommandRule wf_induct
syn keyword IsabelleCommandRule mono_def monoI monoD
syn keyword IsabelleCommandRule lfp_unfold lfp_induct lfp_induct_set
syn keyword IsabelleCommandRule lfp_lowerbound
syn keyword IsabelleCommandRule gfp_unfold coinduct
syn keyword IsabelleCommandRuleMod of OF THEN simplified where symmetric standard
syn match IsabelleCommandRule /\<[a-zA-Z]\+_def\>/
syn match IsabelleCommandPart /|/

syn region IsabelleInner matchgroup=IsabelleInnerMarker start=+"+ end=+"+ contains=@IsabelleInnerStuff

syn match IsabelleSpecial /\\<lambda>\|%/
syn match IsabelleSpecial /\\<circ>\|\<o\>/
syn match IsabelleSpecial /\<O\>/
syn match IsabelleSpecial /\./

syn cluster IsabelleInnerStuff contains=IsabelleSpecial

syn match IsabelleComment "(\*\_.\{-}\*)"
syn match IsabelleComment "--.*$"
syn match IsabelleComment "\(chapter\|text\|txt\|header\|\(sub\)*section\)[ ]*{\*\_.\{-}\*}"
hi def link IsabelleComment Comment

hi IsabelleCommand           ctermfg=3 cterm=bold guifg=yellow gui=bold
hi IsabelleCommandPart       ctermfg=3 cterm=none guifg=yellow
hi IsabelleCommandMod        ctermfg=3 cterm=none guifg=yellow
hi IsabelleInnerMarker       ctermfg=1 cterm=none guifg=red
hi IsabelleSpecial           ctermfg=5 cterm=none guifg=magenta
hi IsabelleCommandProofProve ctermfg=2 cterm=none guifg=green
hi IsabelleCommandProofIsar  ctermfg=2 cterm=none guifg=green
hi IsabelleCommandProofDone  ctermfg=2 cterm=bold guifg=green gui=bold
hi IsabelleCommandProofFail  ctermfg=1 cterm=bold guifg=red   gui=bold
hi IsabelleCommandRule       ctermfg=7 cterm=bold guifg=white gui=bold
hi IsabelleCommandRuleMod    ctermfg=6 cterm=none guifg=cyan
hi IsabelleCommandMethod     ctermfg=6 cterm=none guifg=cyan
hi IsabelleCommandMethodMod  ctermfg=6 cterm=none guifg=cyan
hi IsabelleCommandBigMethod  ctermfg=6 cterm=bold guifg=cyan gui=bold

hi Normal guibg=black guifg=grey
