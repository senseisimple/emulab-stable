; Prototype .emacs file.

(setq inhibit-startup-message t)        ; I've seen it...

;; Set the debug option when there is trouble...
;; (setq debug-on-error t)

; Add my own emacs directory to the load path.
(setq load-path (cons (condition-case () 
                          (expand-file-name "~/emacs") 
                        (error nil)) 
                      load-path))

; Fix the minibuffer completion routines so that SPACE expands as
; much as possible, instead of tab.
(define-key minibuffer-local-completion-map " " 'minibuffer-complete)
(define-key minibuffer-local-completion-map "\t" 'minibuffer-complete-word)
(define-key minibuffer-local-must-match-map " " 'minibuffer-complete)
(define-key minibuffer-local-must-match-map "\t" 'minibuffer-complete-word)

; ================================================================
; Appearance.

(setq default-fill-column 86)

(if (fboundp 'resize-minibuffer-mode)
    (resize-minibuffer-mode))

(if (fboundp 'line-number-mode)
    (line-number-mode t))

(if (fboundp 'column-number-mode)
    (column-number-mode t))

; Use CUA-style cut-and-paste editing (^C,^V,^X,^Z) with transient-mark mode.
;  See http://www.emacswiki.org/cgi-bin/wiki.pl?CopyAndPaste for comparison, 
;  http://www.emacswiki.org/cgi-bin/wiki.pl/CuaMode for Cua installation,
;  http://www.cua.dk/ to download cua.el, put in your own emacs dir or under site-lisp.
(require 'cua)
(CUA-mode t)

; Use the mouse wheel if we have support for it.
(if (fboundp 'mouse-wheel-mode)
     (mouse-wheel-mode 1))

(if (string= window-system "w32")       ; Windows!
    (progn

      ; Make the Left-ctl and Right-Alt combination function as Ctl-Alt.
      ; This is particularly nice if caps-lock is turned into a Left-Ctl key.
      (setq w32-recognize-altgr nil)

      ; To make Caps Lock into a Control key, leaving the original control keys alone:
      ; . See "How do I swap CapsLock and Control?" here:
      ;      http://www.gnu.org/software/emacs/windows/faq3.html
      ; . Get the following file and double-click on it to enter the registry
      ;   settings in regedit, then reboot Windows.
      ;     http://www.gnu.org/software/emacs/windows/ntemacs/contrib/caps-as-ctrl.reg
      
    )
)

;================================================================

;; Electric help, avoids mucking up the window layout with the *Help* buffer.
(require 'ehelp)
(global-set-key "\C-h" 'ehelp-command)
(global-set-key [help] 'ehelp-command)
(global-set-key [f1] 'ehelp-command)

;; Electric-command-history pops up a command history window for redo.
(autoload 'electric-command-history "echistory" nil t)
(global-set-key "\^C\e" 'electric-command-history)      ; ^C-Escape.

;; Electric-buffer-list pops up window to select/manipulate buffers.
(global-set-key "\^X\^B" 'electric-buffer-list)         ; ^X-^B.
(setq electric-buffer-menu-mode-hook  ; Add key bindings like searches.
      '(lambda ()
         (local-set-key "e" 'Buffer-menu-execute)
         (local-set-key "\^s" 'isearch-forward)
         (local-set-key "\^r" 'isearch-backward)
         (local-set-key "\^a" 'beginning-of-line)
         (local-set-key "\^e" 'end-of-line)
         (local-set-key "\M-f" 'forward-word)
         (local-set-key "\M-b" 'backward-word)
         (local-set-key "\M-w" 'kill-ring-save)))

;================================================================

(defun revert-buffer-no-confirm ()
  "Like revert-buffer, but doesn't ask for confirmation every time."
  (interactive)
  (revert-buffer nil t)
)
(global-set-key "\^XR" 'revert-buffer-no-confirm)

(global-set-key "\^Ca" 'apropos)                ;The system-level apropos fn.

(defun toggle-truncate-lines ()
  "Toggle the truncate-lines variable."
  (interactive)
  (setq truncate-lines (not truncate-lines))
  (recenter)                            ; Redisplay.
  (message "truncate-lines is now %s in this buffer." 
           (if truncate-lines "on" "off"))
)
(global-set-key "\^C\^L" 'toggle-truncate-lines)
; Individually control the truncation in vertically-split windows.
(setq truncate-partial-width-windows nil)

(defun toggle-stack-trace-on-error ()
  "Toggle the stack-trace-on-error variable."
  (interactive)
  (setq stack-trace-on-error (not stack-trace-on-error))
  (message "stack-trace-on-error is now %s." 
           (if stack-trace-on-error "on" "off"))
)
(global-set-key "\^C\^S" 'toggle-stack-trace-on-error)

