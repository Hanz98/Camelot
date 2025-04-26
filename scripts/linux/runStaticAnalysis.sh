#!/bin/bash

scriptDir="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
tmp="$(dirname "$scriptDir")"
rootDir="$(dirname "$tmp")"

cd $rootDir

RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

FAILED_ANALYZERS=()

noconfirm=false
for arg in "$@"; do
    if [[ "$arg" == "--noconfirm" ]]; then
        noconfirm=true
        break
    fi
done

GS=$(git status --porcelain=v1 2>/dev/null)
if [ $? -ne 128 ]; then
  function _count_git_pattern() {
    echo "$(grep "^$1" <<< $GS | wc -l)"
  }

  EXIT_CODE=0

  if [ $(_count_git_pattern "??") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern "??") untracked files.\n"
    EXIT_CODE=1
  fi
  if [ $(_count_git_pattern " M") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern " M") unstaged, modified files.\n"
    EXIT_CODE=1
  fi
  if [ $(_count_git_pattern "M ") -ne 0 ]; then
    printf "${RED}There are $(_count_git_pattern "M ") staged, modified files.\n"
    EXIT_CODE=1
  fi


  printf   "${NC}"
  if [ $EXIT_CODE -eq 1 ]; then
    if ! $noconfirm; then
      read -p "Are you sure you want to proceed? (yes/no): " response
      if [[ "$response" != "yes" && "$response" != "y" ]]; then
          echo "Operation canceled."
          exit 1
      fi
    fi

    echo "Proceeding..."
  fi


fi


mapfile -t files < <(
  git -C "$(git rev-parse --show-toplevel)" ls-files \
    | grep -E '\.(c|cpp|h|hpp)$' \
    | grep -v '/ext/'
)

# Run clang-tidy
printf "${CYAN}clang-tidy\n${NC}"
clang_tidy_failed=0

for file in "${files[@]}"; do
  clang-tidy \
    -p build/release/compile_commands.json \
    --warnings-as-errors='*' \
    "$file" \
  || clang_tidy_failed=1
done

if (( clang_tidy_failed )); then
  FAILED_ANALYZERS+=( "clang-tidy" )
fi

# Run cppcheck
printf "${CYAN}cppcheck\n${NC}"
for file in "${files[@]}"; do
  cppcheck \
    --enable=all \
    --inconclusive \
    --language=c++ \
    --std=c++20 \
    --suppress=missingIncludeSystem \
    --error-exitcode=1 \
    "$file" \
  || cppcheck_failed=1
done

if (( cppcheck_failed )); then
  FAILED_ANALYZERS+=( "cppcheck" )
fi

# Run include-what-you-use
# printf "${CYAN}include-what-you-use\n${NC}"
#include-what-you-use $files

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
