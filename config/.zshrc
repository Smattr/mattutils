
#
# .zshrc
#
# 2010-2012 Matthew Fernandez
#
# 2004-2007 David Greenaway
#
# based on 2002 Bernard Blackham
# based on ?
#

# Enable zsh colour function.
autoload -U colors && colors

# Bail out immediately if the system is overloaded, on the assumption that the
# user just wants a shell to run some recovery commands.
if [ $? -eq 0 ]; then
  OVERLOAD=$(($(free -m | grep --color=none '^-/+' | sed 's/^[^0-9]*[0-9]\+[^0-9]\+\([0-9]\+\).*$/\1/g') < 512))
else
  OVERLOAD=0
fi
if [ ${OVERLOAD} -ne 0 ]; then
  export PROMPT="%{${fg_bold[red]}%}[overloaded]%{${fg_no_bold[default]}%} "
else

#
# General Options
#

# Disable core dumps
limit coredumpsize 0

# Enable directory colours
eval `dircolors -b ~/.dircolors`

# Ensure HOST == HOSTNAME and both are set. Some finicky scripts expect one or
# the other.
if [ -z "${HOST}" ]; then
    export HOST=${HOSTNAME}
fi
if [ -z "${HOSTNAME}" ]; then
    export HOSTNAME=${HOST}
fi

#
# Environment Variables
#

LOCAL_PROMPT="[%n@%m %{${fg_bold[red]}%}%(?..%? )%{${fg_no_bold[default]}%}%~] "
REMOTE_PROMPT="[%n@%m %{${fg_bold[red]}%}%(?..%?)%{${fg_no_bold[default]}%}%~ %{${fg_bold[green]}%}#%{${fg_no_bold[default]}%}] "
export PATH="$HOME/bin:$PATH"
export EDITOR="vim"
export LESS="-i -R -n -S -FRX"
if [ -e "${HOME}/.python" ]; then
  export PYTHONSTARTUP="$HOME/.python"
fi

#
# Determine if we are a remote login
#
LOGIN_IP=`echo ${SSH_CLIENT} | cut -d ' ' -f 1`
echo $LOGIN_IP | egrep -q '^(10|192.168)\.'
if [[ $? == 0 ]]; then
	# Local IP address
	LOGIN_IP=""
fi

#
# Setup prompt
#
if [ $LOGIN_IP ] ; then
	export PROMPT=$REMOTE_PROMPT
else
	export PROMPT=$LOCAL_PROMPT
fi
export LOGIN_IP

#
# Aliases
#

alias ls="ls --color=auto"
alias grep="grep --color=always"
alias l="ls"
alias cp="cp -i"
alias mv="mv -i"
alias ll="ls -l"
alias la="ls -A"
alias vi="vim"
alias hist='fc -RI' # Import History
alias :q=exit
alias :Q=exit
alias tmp='pushd `mktemp -d`'
alias cd..="cd .."
alias ..='cd ..'
alias evince="dbus-launch /usr/bin/evince" # Mask evince problems in newer Ubuntu.
alias ffs='sudo $(fc -ln -1)'
alias vcat='pygmentize -g -O bg=dark'

# ZSH Options

#setopt SHARE_HISTORY
setopt INC_APPEND_HISTORY
unsetopt LIST_AMBIGUOUS
setopt AUTO_LIST
setopt AUTO_MENU
setopt AUTO_REMOVE_SLASH
setopt HIST_IGNORE_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_NO_STORE
setopt EXTENDED_HISTORY
setopt INTERACTIVE_COMMENTS
setopt LIST_TYPES
setopt LONG_LIST_JOBS
setopt NO_HUP
setopt RC_QUOTES
export HISTFILE="${HOME}/.zsh_history"
export HISTSIZE=100000000
export SAVEHIST=100000000

autoload history-search-end
zle -N history-beginning-search-backward-end history-search-end
zle -N history-beginning-search-forward-end history-search-end

# Mouse Support

if [ -e ~/.zsh/mouse.zsh ]; then
	. ~/.zsh/mouse.zsh
	bindkey -M emacs '\em' zle-toggle-mouse
fi

# Key Bindings

bindkey -e
bindkey '\e[A' history-beginning-search-backward-end
bindkey '\e[B' history-beginning-search-forward-end
bindkey '\e[1~' beginning-of-line
bindkey '\e[2~' overwrite-mode
bindkey '\e[3~' delete-char
bindkey '\e[4~' end-of-line
bindkey '\e[5~' beginning-of-history
bindkey '\e[6~' end-of-history
bindkey '\e[5C' forward-word
bindkey '\e[5D' backward-word
bindkey '\e\e[5C' forward-word
bindkey '\e\e[5D' backward-word

# Allow custom completion

fpath=(~/.zsh $fpath)
autoload -U ~/.zsh/*(:t)

# Version control status.
setopt PROMPT_SUBST
function vcs_prompt {
    if [ -n "${DISABLE_VCS_PROMPT+x}" ]; then
        echo "-%{${fg_bold[magenta]}%}DISABLED%{${fg_no_bold[default]}%}"
        exit 0
    fi
    for i in git hg svn; do
        which $i &>/dev/null
        if [ $? -ne 0 ]; then
            echo "zshrc: $i not found; vcs_prompt bailing out." 1>&2
            exit 1
        fi
    done
    git branch &>/dev/null
    if [ $? -eq 0 ]; then
        echo -n '-±-'
        if [ -z "`git status --short 2>/dev/null`" ]; then
            # Working directory is clean.
            echo -n "%{${fg_bold[green]}%}"
        elif [ -z "`git status --short 2>/dev/null | grep -v '^?'`" ]; then
            # Working directory only contains changes to untracked files.
            echo -n "%{${fg_bold[yellow]}%}"
        else
            # Working directory contains changes to tracked files.
            echo -n "%{${fg_bold[red]}%}"
        fi
        echo -n `git branch 2>/dev/null | grep '^*' | cut -d ' ' -f 2`
        echo -n "%{${fg_no_bold[default]}%}"
    fi
    hg root &>/dev/null
    if [ $? -eq 0 ]; then
        echo -n '-☿-'
        if [ -z "`hg status 2>/dev/null`" ]; then
            # Working directory is clean.
            echo -n "%{${fg_bold[green]}%}"
        elif [ -z "`hg status 2>/dev/null | grep -v '^?'`" ]; then
            # Working directory only contains changes to untracked files.
            echo -n "%{${fg_bold[yellow]}%}"
        else
            # Working directory contains changes to tracked files.
            echo -n "%{${fg_bold[red]}%}"
        fi
        echo -n `hg branch 2>/dev/null`
        echo -n "%{${fg_no_bold[default]}%}"
    fi
    svn list &>/dev/null
    if [ $? -eq 0 ]; then
        echo -n '-‡-'
        if [ -z "`svn status 2>/dev/null`" ]; then
            # Working directory is clean.
            echo -n "%{${fg_bold[green]}%}"
        elif [ -z "`svn status 2>/dev/null | grep -v '^?'`" ]; then
            # Working directory only contains changes to untracked files.
            echo -n "%{${fg_bold[yellow]}%}"
        else
            # Working directory contains changes to tracked files.
            echo -n "%{${fg_bold[red]}%}"
        fi
        echo -n `svn info 2>/dev/null | grep URL | sed 's/.*\/\(.*\)/\1/g'`
        echo -n "%{${fg_no_bold[default]}%}"
    fi
}

function reboot_prompt {
    if [ -e /var/run/reboot-required ]; then
        echo -n "%{${fg[red]}%} ↺%{${fg_no_bold[default]}%}"
    fi
}

export RPROMPT=$'[%{${fg[red]}%}$(jobs | wc -l | grep -v "^0")%{${fg_no_bold[default]}%}%*$(vcs_prompt)$(reboot_prompt)]'

# Reload scripts
r() {
	local f
	f=(~/.zsh/*(.))
	unfunction $f:t 2> /dev/null
	autoload -U $f:t
}

# Completion

#autoload -U compinit
#compinit

zstyle ':completion:*' completer _expand _complete
zstyle ':completion:*' list-colors ${(s.:.)LS_COLORS}
zstyle ':completion:*' insert-unambiguous true
zstyle ':completion:*' list-prompt %SAt %p: Hit TAB for more, or the character to insert%s
zstyle ':completion:*' matcher-list '' 'm:{a-z}={A-Z}' 'm:{a-z}={A-Z} r:|[._-]=* r:|=*'
zstyle ':completion:*' menu select=2

zstyle ':completion:*' verbose yes
zstyle ':completion:*:descriptions' format '%B%d%b'
zstyle ':completion:*:messages' format '%d'
zstyle ':completion:*:warnings' format 'No matches for: %d'
zstyle ':completion:*' group-name ''



#
# X-Term Title
#

case $TERM in
	(xterm*|putty*))
		precmd() {
			echo -n "\033]0;${USER}@${HOST}: zsh (${PWD})\007";
		}
		preexec() {
			COMMAND=`echo -n $1 | cat -v | sed 's/	/ /'`;
			echo -n "\033]0;${USER}@${HOST}: ${COMMAND} (${PWD})\007";
		}
		;;
	screen*)
		precmd() {
			echo -ne "\033]0;${USER}@${HOST}: zsh (${PWD})\007";
			echo -ne "\033k${USER}@${HOST}: zsh (${PWD})\033\\"; 
		}
		preexec() {
			COMMAND=`echo -n $1 | cat -v | sed 's/	/ /'`;
			echo -ne "\033]0;${USER}@${HOST}: ${COMMAND} (${PWD})\007";
			echo -ne "\033k${USER}@${HOST}: ${COMMAND} (${PWD})\033\\";
		}
		;;
	*)
		;;
esac

# Source any computer-local options

if [ -e ~/.zshrc_local ]; then
	source ~/.zshrc_local
fi
if [ -e ~/.zshrc_mattbin ]; then
    source ~/.zshrc_mattbin
fi
if [ -e ~/.zshrc_nictabin ]; then
    source ~/.zshrc_nictabin
fi


# Configure ssh-agent for gnome
export SSH_ASKPASS=gnome-ssh-askpass

# Check cron mail
mail -H &>/dev/null
if [[ $? == 0 ]]; then
    echo "You have unread mail."
fi

# Terminal highlighting.
highlight() { perl -pe "s/$1/\e[1;35;44m$&\e[0m/g"; }

if [[ "${TERM}" != "screen" ]]; then
    echo "You are not in screen/tmux..."
fi

# Ubuntu-style Command not found helper.
if [ -f /etc/zsh_command_not_found ]; then
    . /etc/zsh_command_not_found
fi

# \<forall> helper.
function _forall() {
    TYPE=$1
    shift
    find . -type ${TYPE} \
        ! -name '.' \
        ! -path '*/.hg/*'  ! -name '.hg'  ! -name '.hgkeep'  ! -name '.hgignore' \
        ! -path '*/.git/*' ! -name '.git' ! -name '.gitkeep' ! -name '.gitignore' \
        ! -path '*/.svn/*' ! -name '.svn' ! -name '.svnkeep' \
        ! -path '*/.cvs/*' ! -name '.cvs' ! -name '.cvskeep' \
        ! -name '*.swp' \
        -exec $@ "{}" \;
}
function forall() {
    if [ $# -eq 0 ]; then
        _forall f echo
    else
        _forall f $@
    fi
}
function foralld() {
    if [ $# -eq 0 ]; then
        _forall d echo
    else
        _forall d $@
    fi
}

# Unmap Ctrl-s and Ctrl-q. Thank you http://feedproxy.google.com/~r/catonmat/~3/pbN7TgpMiyg/annoying-keypress-in-linux.
stty stop undef
stty start undef

if [ -n "${PHD}" ]; then
    export PROMPT="%{${fg_bold[blue]}%}[phd]%{${fg_no_bold[default]}%} ${PROMPT}"
fi

# Export of environment variables with a visible reminder.
function exp() {
  if [ $# -eq 0 ]; then
    echo "usage: $0 variable=value" >&2
    return 1
  fi
  export "$1"
  PROMPT="%{${bg_bold[magenta]}%}${1%%=*}%{${bg_no_bold[default]}%} ${PROMPT}"
}

fi # Close overload detection
