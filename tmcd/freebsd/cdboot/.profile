:
# $FreeBSD: src/release/fixit.profile,v 1.8.2.1 2001/11/05 11:22:04 brian Exp $

export BLOCKSIZE=K
export EDITOR=vi
export PAGER=more

alias ls="ls -F"
alias ll="ls -l"
alias m="more -e"

# Make the arrow keys work; everybody will love this.
set -o emacs 2>/dev/null
