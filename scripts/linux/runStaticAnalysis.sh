#!/bin/bash

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp="$(dirname "$scriptDir")"
rootDir="$(dirname "$tmp")"

cd $rootDir

RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

FAILED_ANALYZERS=()

# Check Git status
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
    printf "${RED}There are $(_count_git_pattern "M ") staged, modified files.\n"
  fi
  printf "${RED}Please stash or commit them.\n${NC}"
#  exit 1
fi

mapfile -t files < <(git -C "$(git rev-parse --show-toplevel)" ls-files | grep -E '\.(c|cpp|h|hpp)$')

# Run clang-format
printf "${CYAN}clang-format\n${NC}"
clang-format -i --style=Google $files > /dev/null
if [ $? -ne 0 ]; then
  FAILED_ANALYZERS+=("clang-format")
fi

# Run clang-tidy
printf "${CYAN}clang-tidy\n${NC}"
clang-tidy $files -p build/compile_commands.json -warnings-as-errors=*
if [ $? -ne 0 ]; then
  FAILED_ANALYZERS+=("clang-tidy")
fi

# Run cppcheck
printf "${CYAN}cppcheck\n${NC}"
cppcheck --enable=all --inconclusive -v $files --error-exitcode=1
if [ $? -ne 0 ]; then
  FAILED_ANALYZERS+=("cppcheck")
fi

# Run include-what-you-use
printf "${CYAN}include-what-you-use\n${NC}"
include-what-you-use $files

# Run uncrustify
printf "${CYAN}uncrustify\n${NC}"
for file in "${files[@]}"; do
  uncrustify -c scripts/linux/cfg/uncrustify.cfg -f $file -o $file --no-backup
  if [ $? -ne 0 ]; then
    FAILED_ANALYZERS+=("uncrustify")
  fi
done

# Summary
if [ ${#FAILED_ANALYZERS[@]} -ne 0 ]; then
  printf "${RED}The following analyzers failed:\n"
  for analyzer in "${FAILED_ANALYZERS[@]}"; do
    printf "  - $analyzer\n"
  done
  printf "${NC}"
else
  printf "${CYAN}All analyzers passed successfully.\n${NC}"
fi

echo Done
