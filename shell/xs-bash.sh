#!/bin/sh

# Run the xs program to get the target directory to be used in the various context
# This is the fundamental core of the bookmarking idea. An alias or substring is
# expected and upon receiving one it either resolves the alias if it exists or opens a
# curses window with the narrowed down options waiting for the user to select one.
# @param  string alias
#
# @access private
# @return 0 on success, >0 on failure
function _xs_get_dir()
{
 local bookmark extrapath
 # if there is one exact match (possibly with extra path info after it),
 # then just use that match without calling xs
 if [ -e "$HOME/.xs" ]; then
  dir=`grep "^$1 " "$HOME/.xs"`
  if [ -z "$dir" ]; then
   bookmark="${1/\/*/}"
   if [ "$bookmark" != "$1" ]; then
    dir=`grep "^$bookmark " "$HOME/.xs"`
    extrapath=`echo "$1" | sed 's#^[^/]*/#/#'`
   fi
  fi
  [ -n "$dir" ] && dir=`echo "$dir" | sed 's/^[^ ]* //'`
 fi

 if [ -z "$dir" -o "$dir" != "${dir/
 /}" ]; then
  # okay, we need xs to resolve this one.
  # note: intentionally retain any extra path to add back to selection.
  dir=
  if xscore --noresolve "${1/\/*/}"; then
   dir=`cat "$HOME/.xsresult"`
   rm -f "$HOME/.xsresult";
  fi
 fi

 if [ -z "$dir" ]; then
  # echo "Aborted: no directory selected" >&2
  return 1
 fi

 [ -n "$extrapath" ] && dir="$dir$extrapath"

 if [ ! -d "$dir" ]; then
  echo "Failed: no such directory '$dir'" >&2
  return 2
 fi
}

# Perform the command (cp or mv) using the xs bookmark alias as the target directory
# @param  string command argument list          #
#                                               #
# @access private                               #
# @return void                                  #
function _xs_exec()
{
 local arg dir i last call_with_browse

 # Get the last option which will be the bookmark alias
 eval last=\${$#};

 # Resolve the bookmark alias. If it cannot resolve, the
 # curses window will come up at which point a directory
 # will need to be choosen.  After selecting a directory,
 # the function will continue and $_xs_dir will be set.
 if [ -e $last ]; then
  last=
 fi

 if _xs_get_dir "$last"; then
  # For each argument save the last, move the file given in
  # the argument to the resolved xs directory
  i=1;
  for arg; do
   if [ $i -lt $# ]; then
    $command "$arg" "$dir";
   fi
   let i=$i+1;
  done
 fi
}

# Change directory to the xs target directory
# @param  string alias
#
# @access public
function xs()
{ 
 local dir

 _xs_get_dir "$1" && cd "$dir" && echo `pwd`;
}

# Bash programming completion for xs. Sets the $COMPREPLY list for complete
# @param  string substring of alias
#
# @access private
function _xs_aliases ()
{
 local cur bookmark dir strip oldIFS
 COMPREPLY=()
 if [ -e "$HOME/.xs" ]; then
  cur=${COMP_WORDS[COMP_CWORD]}
  if [ "$cur" != "${cur/\//}" ]; then # if at least one /
   bookmark="${cur/\/*/}"
   dir=`grep "^$bookmark " "$HOME/.xs" | sed 's#^[^ ]* ##'`
   if [ -n "$dir" -a "$dir" = "${dir/
/}" -a -d "$dir" ]; then
    strip="${dir//?/.}"
    oldIFS="$IFS"
    IFS='
'
    COMPREPLY=( $(
     compgen -d "$dir`echo "$cur" | sed 's#^[^/]*##'`" \
     | sed -e "s/^$strip/$bookmark/" -e "s/\([^\/a-zA-Z0-9#%_+\\\\,.-]\)/\\\\\\1/g" ) )
    IFS="$oldIFS"
   fi
   else
    COMPREPLY=( $( (echo $cur ; cat "$HOME/.xs") | \
     awk 'BEGIN {first=1}
     {if (first) {cur=$0; l=length(cur); first=0}
     else if (substr($1,1,l) == cur) {print $1}}' ) )
  fi
 fi
 
 return 0
}

# Bash programming completion for xs
# Set up completion (put in a function just so `nospace' can be a local variable)
#
# @access private
_xs_complete() {
 local nospace=
 [ "${BASH_VERSINFO[0]}" -ge 3 -o \( "${BASH_VERSINFO[0]}" = 2 -a \( "${BASH_VERSINFO[1]}" = 05a -o "${BASH_VERSINFO[1]}" = 05b \) \) ] && nospace='-o nospace'
 complete $nospace -S / -X '*/' -F _xs_aliases xs
}

_xs_complete
