#!/bin/bash

RED='\033[0;31m'
NC='\033[0m' # No Color

GS=$(git status --porcelain=v1 2>/dev/null)
if [ $? -ne 128 ]; then
  function _count_git_pattern() {
    echo "$(grep "^$1" <<< $GS | wc -l)" 
  }                                           

  echo "There are $(_count_git_pattern "??") untracked files."                                 

  printf "${RED}Please stash or commit them.\n ${NC}"

fi

mapfile -t files < <(git -C "$(git rev-parse --show-toplevel)" ls-files | grep -E '\.(c|cpp|h|hpp)$')

for file in "${files[@]}"; do
  echo "Processing: $file"



done
