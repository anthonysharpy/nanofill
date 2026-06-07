#!/usr/bin/env bash

#
# I MADE THIS SCRIPT WITH AI BECAUSE IT WAS A LOT FASTER THAN DOING IT MYSELF
#
# PLEASE FORGIVE ME
#

# ─────────────────────────────────────────────
#  bench.sh  –  continuous latency benchmarker
#  Compares ./nanofillmain (baseline) vs ./nanofill (candidate)
# ─────────────────────────────────────────────

set -euo pipefail

# ── Colour palette ────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; DIM='\033[2m'; RESET='\033[0m'
WHITE='\033[1;37m'; MAGENTA='\033[0;35m'

# ── Stat labels (order must match parse_output) ──
STAT_KEYS=("avg" "p0" "p50" "p75" "p90" "p95" "p99" "p99_9" "p100")
STAT_LABELS=("Average" "P0" "P50" "P75" "P90" "P95" "P99" "P99.9" "P100")

# ── Accumulators: associative arrays keyed by "<binary>_<stat>" ──
declare -A sum_val count_val min_val max_val last_val

# ── Parse output from one binary run ──────────
# Prints: avg p0 p50 p75 p90 p95 p99 p99.9 p100  (one per line, numeric only)
parse_output() {
    local raw="$1"
    # Average event time
    echo "$raw" | grep -oP 'Average event time:\s*\K[0-9]+\.?[0-9]*'
    # Percentile lines – extract value after the colon
    for p in P0 P50 P75 P90 P95 P99 "P99\.9" P100; do
        echo "$raw" | grep -oP "${p}:\s*\K[0-9]+\.?[0-9]*"
    done
}

# ── Run one binary and accumulate stats ───────
run_binary() {
    local bin="$1"   # e.g. "nanofill"
    local raw
    raw=$("./${bin}" 2>&1) || { echo "ERROR: ./${bin} failed" >&2; return 1; }

    local values
    mapfile -t values < <(parse_output "$raw")

    if [[ ${#values[@]} -ne ${#STAT_KEYS[@]} ]]; then
        echo "ERROR: unexpected output from ./${bin} (got ${#values[@]} values)" >&2
        return 1
    fi

    for i in "${!STAT_KEYS[@]}"; do
        local key="${bin}_${STAT_KEYS[$i]}"
        local v="${values[$i]}"

        last_val[$key]="$v"

        if [[ -z "${sum_val[$key]+x}" ]]; then
            sum_val[$key]="$v"
            count_val[$key]=1
            min_val[$key]="$v"
            max_val[$key]="$v"
        else
            # Accumulate
            sum_val[$key]=$(awk "BEGIN{printf \"%.4f\", ${sum_val[$key]}+${v}}")
            count_val[$key]=$(( count_val[$key] + 1 ))
            # Min
            min_val[$key]=$(awk "BEGIN{print (${v} < ${min_val[$key]}) ? ${v} : ${min_val[$key]}}")
            # Max
            max_val[$key]=$(awk "BEGIN{print (${v} > ${max_val[$key]}) ? ${v} : ${max_val[$key]}}")
        fi
    done
}

# ── Format a nanosecond value nicely ──────────
fmt_ns() {
    printf "%.2f ns" "$1"
}

# ── Compute derived value (avg/min/max/last) ──
get_stat() {
    local bin="$1" key="$2" mode="$3"
    local k="${bin}_${key}"
    case "$mode" in
        avg)  awk "BEGIN{printf \"%.4f\", ${sum_val[$k]}/${count_val[$k]}}" ;;
        min)  echo "${min_val[$k]}" ;;
        max)  echo "${max_val[$k]}" ;;
        last) echo "${last_val[$k]}" ;;
    esac
}

# ── Draw the results table ─────────────────────
draw_table() {
    local run_count="$1"

    # Column widths
    local C0=10   # stat label
    local C1=14   # baseline avg
    local C2=14   # baseline min
    local C3=14   # baseline max
    local C4=14   # baseline last
    local C5=14   # candidate avg
    local C6=14   # candidate min
    local C7=14   # candidate max
    local C8=14   # candidate last
    local C9=14   # abs diff (avg)
    local C10=12  # % diff (avg)

    local TOTAL=$(( C0+C1+C2+C3+C4+C5+C6+C7+C8+C9+C10 + 12 ))

    # ── helpers ──
    hline() { printf '%*s\n' "$TOTAL" '' | tr ' ' '─'; }
    thick() { printf '%*s\n' "$TOTAL" '' | tr ' ' '═'; }

    cell()  { printf "│ %-${1}s " "$2"; }   # left-aligned
    rcell() { printf "│ %${1}s "  "$2"; }   # right-aligned

    clear

    echo
    printf "${BOLD}${CYAN}  ⚡  nanofill latency benchmark${RESET}   "
    printf "${DIM}runs completed: ${WHITE}%d${RESET}\n\n" "$run_count"

    # Each cell occupies: 1(│) + 1(space) + WIDTH + 1(space) = WIDTH+3 chars
    # Span width for N equal columns of width W = N*(W+2) + (N-1) inner separators
    # but we print it as a single cell so we just need the inner content width:
    # content_span = N*(W+3) - 2  (subtract the outer │ and trailing space handled by printf)
    local BM_SPAN=$(( 4*(C1+3) - 2 ))   # 4 baseline cols
    local CD_SPAN=$(( 4*(C5+3) - 2 ))   # 4 candidate cols

    thick
    printf "│ %-${C0}s " "STAT"
    printf "│ ${MAGENTA}%-${BM_SPAN}s${RESET}" "  ./nanofillmain  (baseline)"
    printf "│ ${CYAN}%-${CD_SPAN}s${RESET}" "  ./nanofill  (candidate)"
    printf "│ %-${C9}s " "DELTA (avg)"
    printf "│ %-${C10}s │\n" "% CHANGE"
    printf "│ %-${C0}s " ""
    for lbl in "AVG" "MIN" "MAX" "LAST"; do
        printf "│ %${C1}s " "${lbl} (ns)"
    done
    for lbl in "AVG" "MIN" "MAX" "LAST"; do
        printf "│ %${C5}s " "${lbl} (ns)"
    done
    printf "│ %${C9}s " "(ns)"
    printf "│ %${C10}s │\n" ""
    thick

    for i in "${!STAT_KEYS[@]}"; do
        local k="${STAT_KEYS[$i]}"
        local label="${STAT_LABELS[$i]}"

        local bm_avg bm_min bm_max bm_last
        local cd_avg cd_min cd_max cd_last

        bm_avg=$(get_stat "nanofillmain" "$k" avg)
        bm_min=$(get_stat "nanofillmain" "$k" min)
        bm_max=$(get_stat "nanofillmain" "$k" max)
        bm_last=$(get_stat "nanofillmain" "$k" last)

        cd_avg=$(get_stat "nanofill" "$k" avg)
        cd_min=$(get_stat "nanofill" "$k" min)
        cd_max=$(get_stat "nanofill" "$k" max)
        cd_last=$(get_stat "nanofill" "$k" last)

        local delta pct colour sign
        delta=$(awk "BEGIN{printf \"%.4f\", ${cd_avg}-${bm_avg}}")
        pct=$(awk  "BEGIN{printf \"%.2f\",  (${bm_avg}!=0) ? (${cd_avg}-${bm_avg})/${bm_avg}*100 : 0}")

        # Colour: green = candidate is faster (negative delta), red = slower
        if awk "BEGIN{exit !(${delta} < 0)}"; then
            colour="${GREEN}"; sign=""
        elif awk "BEGIN{exit !(${delta} > 0)}"; then
            colour="${RED}"; sign="+"
        else
            colour="${WHITE}"; sign=""
        fi

        printf "│ ${BOLD}%-${C0}s${RESET} " "$label"
        printf "│ %${C1}s " "$(printf '%.2f' $bm_avg)"
        printf "│ %${C2}s " "$(printf '%.2f' $bm_min)"
        printf "│ %${C3}s " "$(printf '%.2f' $bm_max)"
        printf "│ %${C4}s " "$(printf '%.2f' $bm_last)"
        printf "│ %${C5}s " "$(printf '%.2f' $cd_avg)"
        printf "│ %${C6}s " "$(printf '%.2f' $cd_min)"
        printf "│ %${C7}s " "$(printf '%.2f' $cd_max)"
        printf "│ %${C8}s " "$(printf '%.2f' $cd_last)"
        printf "│ ${colour}%${C9}s${RESET} " "${sign}$(printf '%.2f' $delta)"
        printf "│ ${colour}%${C10}s${RESET} │\n" "${sign}${pct}%"

        [[ $i -eq 0 ]] && hline   # separator after Average row
    done

    thick
    printf "${DIM}  All values in nanoseconds.  "
    printf "Green = candidate faster · Red = candidate slower.  "
    printf "Press Ctrl-C to stop.${RESET}\n\n"
}

# ── Cleanup on exit ───────────────────────────
trap 'echo -e "\n${DIM}Benchmark stopped.${RESET}"; exit 0' INT TERM

# ── Verify binaries exist ─────────────────────
for b in nanofill nanofillmain; do
    [[ -x "./${b}" ]] || { echo "ERROR: ./${b} not found or not executable." >&2; exit 1; }
done

# ── Main loop ─────────────────────────────────
run_count=0

while true; do
    if (( run_count % 2 == 0 )); then
        run_binary "nanofillmain"
        run_binary "nanofill"
    else
        run_binary "nanofill"
        run_binary "nanofillmain"
    fi

    run_count=$(( run_count + 1 ))
    draw_table "$run_count"

    sleep 4
done