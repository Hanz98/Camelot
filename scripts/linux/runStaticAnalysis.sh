#!/bin/bash

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

tmp="$(dirname "$scriptDir")"

rootDir="$(dirname "$tmp")"

cd $rootDir

RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

GS=$(git status --porcelain=v1 2>/dev/null)
if [ $? -ne 128 ]; then
  function _count_git_pattern() {
    echo "$(grep "^$1" <<< $GS | wc -l)" 
  }                                           
  
  if [ $(_count_git_pattern "??") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern "??") untracked files.\n"                                 
  fi
  if [ $(_count_git_pattern " M") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern " M") unstaged, modified files.\n"
  fi
  if [ $(_count_git_pattern "M ") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern "M ")   staged, modified files.\n"        
  fi
  printf "${RED}Please stash or commit them.\n${NC}"
#  exit 1
  
fi

mapfile -t files < <(git -C "$(git rev-parse --show-toplevel)" ls-files | grep -E '\.(c|cpp|h|hpp)$')

printf "${CYAN}clang-format\n${NC}"
clang-format -i --style=Google $files > /dev/null

printf "${CYAN}clang-tidy\n${NC}"
clang-tidy $files -p build/compile_commands.json

printf "${CYAN}cppcheck\n${NC}"
cppcheck --enable=all --inconclusive -v $files

printf "${CYAN}include-what-you-use\n${NC}"
include-what-you-use $files

printf "${CYAN}uncrustify\n${NC}"
for file in "${files[@]}"; do
  uncrustify -c scripts/linux/cfg/uncrustify.cfg -f $file -o $file --no-backup
done

echo Done