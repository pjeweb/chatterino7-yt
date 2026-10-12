#!/usr/bin/env bash
# Build and test inside a container derived from Chatterino's official Ubuntu
# build image (see scripts/dev/Dockerfile). Every checkout is mounted at /src,
# so worktrees share one ccache.
#
# Usage: scripts/dev/docker-build.sh <mode> [extra cmake args...]
#   app    build the app (RelWithDebInfo)
#   test   build the tests (Debug)
#   check  build the tests and run them (starts httpbox + pubsub test server)
#   asan   ASan + UBSan, RelWithDebInfo at -O1, no test retries
#
# Environment: C7YT_JOBS (default: half the cores), C7YT_CCACHE (cache dir).
set -euo pipefail

mode="${1:-check}"
shift || true

repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
image="c7yt-dev:latest"
jobs="${C7YT_JOBS:-$(($(nproc) / 2))}"
ccache_dir="${C7YT_CCACHE:-$HOME/.cache/c7yt-ccache}"
mkdir -p "$ccache_dir"

if ! docker image inspect "$image" >/dev/null 2>&1; then
    docker build -t "$image" "$repo/scripts/dev"
fi

common=(-DCMAKE_PREFIX_PATH=/usr/local-qt/lib/cmake -DCHATTERINO_STATIC_QT_BUILD=On
    -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
run_tests=0
test_options=(--repeat until-pass:4)
sanitizer_env=()
verify_sanitizers=""
case "$mode" in
    app) dir=build-app; flags=(-DBUILD_APP=ON -DBUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo) ;;
    test) dir=build-test; flags=(-DBUILD_APP=OFF -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug) ;;
    check) dir=build-test; flags=(-DBUILD_APP=OFF -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug); run_tests=1 ;;
    # GCC 15 UBSan cannot constant-evaluate Qt's debug QStringView pointer check
    # for weak enum-name storage (GCC PR71962). Keep both sanitizers, use Qt's
    # release configuration with symbols rather than disabling sanitizer checks.
    asan) dir=build-asan; flags=(-DBUILD_APP=OFF -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
        '-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O1 -g -DNDEBUG'
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCHATTERINO_SANITIZER_SUPPORT=ON
        -DSANITIZE_ADDRESS=ON -DSANITIZE_UNDEFINED=ON); run_tests=1
        # Fail on the first diagnostic; retries can hide intermittent memory bugs.
        test_options=()
        sanitizer_env=(-e ASAN_OPTIONS=halt_on_error=1:detect_leaks=1
            -e UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1)
        verify_sanitizers="python3 scripts/dev/check-sanitizers.py $dir/compile_commands.json && " ;;
    *) echo "unknown mode: $mode" >&2; exit 1 ;;
esac

printf -v script '%q ' cmake -S . -B "$dir" -G Ninja "${common[@]}" "${flags[@]}" "$@"
printf -v build_command '%q ' cmake --build "$dir" -j "$jobs"
script+=" && ${verify_sanitizers}${build_command}"
if [ "$run_tests" = 1 ]; then
    printf -v test_command '%q ' ctest "${test_options[@]}" --output-on-failure -j "$jobs"
    script+=" && (httpbox --port 9051 >/dev/null 2>&1 &) \
        && (cd /opt/pubsub-server && ./server 127.0.0.1:9050 127.0.0.1:9052 >/dev/null 2>&1 &) \
        && sleep 1 && cd $dir && ${test_command}"
fi

exec docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e CCACHE_DIR=/ccache \
    -e CCACHE_MAXSIZE=20G \
    -e QT_QPA_PLATFORM=minimal \
    "${sanitizer_env[@]}" \
    -v "$repo:/src" \
    -v "$ccache_dir:/ccache" \
    -w /src \
    "$image" \
    bash -c "$script"
