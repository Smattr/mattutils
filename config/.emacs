(custom-set-variables
  ;; custom-set-variables was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 '(cua-mode t nil (cua-base))
 '(ido-mode (quote both) nil (ido))
 '(indent-tabs-mode nil)
 '(inhibit-startup-screen t)
 '(isar-proof:Sledgehammer:\ Provers "vampire spass e z3 cvc3")
 '(proof-splash-enable nil)
 '(proof-splash-time 1))
(custom-set-faces
  ;; custom-set-faces was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 )

(load-library "~/l4.verified/misc/site-lisp/proof-indent.el")
(global-set-key (kbd "C-c i") 'auto-proof-indent)
(global-set-key (kbd "C-c o") 'auto-proof-cancel-indent)
(set-fontset-font "fontset-default" 'unicode "Cambria Math 11")
