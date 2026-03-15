#!/usr/bin/env bash
#
# Aggressive memory / fork / error-path stress test for harsh shell
# Designed to provoke still-reachable, possibly-lost, invalid reads/writes

set -u

SHELL_BIN="./harsh"
VALGRIND_LOG="/tmp/harsh_vg_$$.log"
SUPP="readline.supp"           # assume you have one for readline noise

PASS=0
FAIL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

run_test() {
    local name="$1"
    local input="$2"

    echo -n "→ $name ... "

    # We pipe commands + final 'exit' to make sure shell terminates cleanly
    {
        printf '%s\n' "$input"
        echo "exit"
    } | valgrind \
        --trace-children=no \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --suppressions="$SUPP" \
        --log-file="$VALGRIND_LOG" \
        --error-exitcode=42 \
        "$SHELL_BIN" > /dev/null 2>&1

    local vg_rc=$?

    # Extract numbers (handling "no leaks are possible" case too)
    definitely=$(awk '/definitely lost:/ {gsub(/,/, ""); s+=$4} END {print s+0}' "$VALGRIND_LOG")
    indirectly=$(awk '/indirectly lost:/ {gsub(/,/, ""); s+=$4} END {print s+0}' "$VALGRIND_LOG")
    possibly=$(awk   '/possibly lost:/   {gsub(/,/, ""); s+=$4} END {print s+0}' "$VALGRIND_LOG")
    reachable=$(awk  '/still reachable:/  {gsub(/,/, ""); s+=$4} END {print s+0}' "$VALGRIND_LOG")

    if [ "$vg_rc" -eq 42 ] || [ "$definitely" -gt 0 ] || [ "$indirectly" -gt 0 ] || [ "$possibly" -gt 0 ] || [ "$reachable" -gt 0 ]; then
        echo -e "${RED}FAIL${NC}"
        echo "       def=$definitely  ind=$indirectly  pos=$possibly  reach=$reachable  vg_rc=$vg_rc"
        echo -e "${YELLOW}--- relevant leak lines ---${NC}"
        grep -A 12 -E "definitely lost|indirectly lost|possibly lost|still reachable|Invalid (read|write|free)" "$VALGRIND_LOG" \
            | grep -v "^==" | head -60
        echo -e "${YELLOW}----------------------------${NC}"
        FAIL=$((FAIL+1))
    else
        echo -e "${GREEN}PASS${NC}  (reach=$reachable)"
        PASS=$((PASS+1))
    fi

    # Keep log for failed cases
    [ "$vg_rc" -ne 0 ] || [ "$definitely" -gt 0 ] || [ "$indirectly" -gt 0 ] || [ "$possibly" -gt 0 ] || rm -f "$VALGRIND_LOG"
}

echo "============================================================="
echo "  harsh — CRAZY MEMORY / FORK / ERROR-PATH STRESS SUITE"
echo "============================================================="
echo ""

# ──────────────────────────────────────────────────────────────────────────────
#  Normal behavior tests (should be clean)
# ──────────────────────────────────────────────────────────────────────────────

run_test "very long command line" \
    "echo $(printf 'x%.0s' {1..4096})"

run_test "deep pipeline" \
    "echo start | cat | cat | cat | cat | cat | cat | cat | cat | wc -c"

run_test "many background jobs" \
    "sleep 1 & sleep 1 & sleep 1 & sleep 1 & sleep 1 & wait"

run_test "alias recursion attempt (should stop)" \
    "alias a='a' ; a"

run_test "history bomb" \
    "echo 1\n!!\n!!\n!!\n!!\n!!\n!!\n!!\n!!\n!!\n!!"

# ──────────────────────────────────────────────────────────────────────────────
#  Crazy / malicious input zone — try to break memory management
# ──────────────────────────────────────────────────────────────────────────────

run_test "chaos — many failing forks + redirects" \
    "nonexist1 > f1 2>f2 ; nonexist2 >> f3 ; nonexist3 2>&1 | nonexist4 ; echo ok || echo fail"

run_test "background + failing pipeline" \
    "(false | false | false | cat) & wait ; echo survived"

run_test "redirect to invalid fd" \
    "echo hi 999>/dev/null ; echo still alive"

run_test "very many globs that fail" \
    "ls /nonexistent/a* /nonexistent/b* /nonexistent/c* /nonexistent/d* /nonexistent/e* || echo globfail"

run_test "alias explosion" \
    "alias x='echo x'\n x x x x x x x x x x x x x x x x x x x x"

run_test "deep nested quoting & glob" \
    "echo \"'\\\"hello*world\\\"'\" '*.sh' ~/*nonexist*"

run_test "background job bomb + bad commands" \
    "false & true & sleep 0.1 & nonexist1 & nonexist2 & wait ; echo done"

run_test "and-or + background mix madness" \
    "false && true || (echo a | cat > /dev/null &) && echo b || echo c & wait"

run_test "redirect bomb + non-existing files" \
    "echo 1>nofile 2>nofile 3>nofile 4>nofile 5>nofile 6>nofile 7>nofile"

run_test "history + alias + bad command chain" \
    "alias z='nonexistcommand'\nz\n!!\nz\n!!\nunalias z\necho clean?"

run_test "maximum pain — everything at once" \
    "alias bomb='nonexist || echo fail | cat > /dev/null &'\nbomb\n!!\nbomb && bomb || bomb ; bomb & wait ; echo '~/.nonexist*' *.nonexist*"

run_test "cd to invalid + popd abuse" \
    "pushd /impossiblepath123\npopd\npopd\npopd\npopd\npopd\npopd"

run_test "very long alias chain attempt" \
    "alias a='b' ; alias b='c' ; alias c='d' ; alias d='echo deep'\na"

# ──────────────────────────────────────────────────────────────────────────────
# Summary
# ──────────────────────────────────────────────────────────────────────────────

echo ""
echo "============================================================="
echo -e "  Final: ${GREEN}${PASS}${NC} passed   ${RED}${FAIL}${NC} failed / provoked issues"
echo "============================================================="

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}→ All memory-sensitive tests passed cleanly${NC}"
else
    echo -e "${YELLOW}→ Some tests triggered memory errors — check logs above${NC}"
fi

# Cleanup leftover files from tests
rm -f f[1-9] nofile /tmp/harsh_* 2>/dev/null

exit $((FAIL > 0 ? 1 : 0))