syn clear
syn sync fromstart
syn case match

syn keyword IsabelleCommand term
syn keyword IsabelleComment abbreviation
syn keyword IsabelleCommand theory
syn keyword IsabelleCommand theorem
syn keyword IsabelleCommand lemma
syn keyword IsabelleCommand lemmas
syn keyword IsabelleCommand primrec
syn keyword IsabelleCommand datatype
syn keyword IsabelleCommand declare
syn keyword IsabelleCommand definition
syn keyword IsabelleCommand fun
syn keyword IsabelleCommand typedecl
syn keyword IsabelleCommand types
syn keyword IsabelleCommand typedef
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
syn keyword IsabelleCommandPart and is
syn keyword IsabelleCommandPart assumes shows fixes
syn keyword IsabelleCommandPart where for
syn keyword IsabelleCommandPart begin end
syn keyword IsabelleCommandPart imports
syn keyword IsabelleCommandPart monos overloaded
syn keyword IsabelleCommandMod simp iff rule_format cong
syn match IsabelleCommandMod /\<intro\>!\?/
syn match IsabelleCommandMod /\<dest\>!\?/
syn keyword IsabelleCommandProofProve proof
syn keyword IsabelleCommandProofProve apply
syn keyword IsabelleCommandProofProve prefer defer
syn keyword IsabelleCommandProofDone done by qed
syn keyword IsabelleCommandProofFail sorry
syn keyword IsabelleCommandProofIsar assume show have from then thus hence
syn keyword IsabelleCommandProofIsar with next using note
syn keyword IsabelleCommandProofIsar let
syn keyword IsabelleCommandProofIsar moreover ultimately also finally
syn keyword IsabelleCommandProofIsar fix obtain where case
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
syn keyword IsabelleCommandMethod fold unfold unfold_locales
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
syn keyword IsabelleCommandBigMethod blast force auto fast best slow deepen
syn keyword IsabelleCommandBigMethod arith algebra
syn keyword IsabelleCommandBigMethod bestsimp fastsimp simplesubst slowsimp
syn keyword IsabelleCommandBigMethod clarify safe clarsimp
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
syn keyword IsabelleCommandRuleMod of OF THEN simplified where
syn match IsabelleCommandRule /\<[a-zA-Z]\+_def\>/
syn match IsabelleCommandPart /|/

syn region IsabelleInner matchgroup=IsabelleInnerMarker start=+"+ end=+"+ contains=@IsabelleInnerStuff

syn match IsabelleSpecial /\\<lambda>\|%/
syn match IsabelleSpecial /\\<circ>\|\<o\>/
syn match IsabelleSpecial /\<O\>/
syn match IsabelleSpecial /\./

syn cluster IsabelleInnerStuff contains=IsabelleSpecial

syn match IsabelleComment "(\*\_.\{-}\*)"
syn match IsabelleComment "text {\*\_.\{-}\*}"
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
