set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

IMAGE_NAME="${IMAGE_NAME:-localtaskmanager-dev}"
DOCKERFILE="${DOCKERFILE:-$ROOT/Dockerfile}"

BUILD_DIR="${BUILD_DIR:-build}"
CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-Ninja}"

VENV_DIR="${VENV_DIR:-$ROOT/.venv}"
REQ_FILE="${REQ_FILE:-$ROOT/requirements.txt}"

APP_CONFIG="${APP_CONFIG:-}"
APP_DB="${APP_DB:-}" 

JOBS="${JOBS:-}"

if [[ -z "${JOBS}" ]]; then
  if command -v nproc >/dev/null 2>&1; then JOBS="$(nproc)"; else JOBS="4"; fi
fi

die() { echo "ERROR: $*" >&2; exit 1; }
is_podman() {
  docker --version 2>/dev/null | grep -qi podman
}

volume_suffix() {
  if command -v getenforce >/dev/null 2>&1; then
    local st
    st="$(getenforce 2>/dev/null || true)"
    if [[ "${st}" == "Enforcing" ]]; then
      echo ":Z"
      return
    fi
  fi
  echo ""
}

docker_run() {
  local vol="${ROOT}:/app$(volume_suffix)"

  local args=(--rm -v "${vol}" -w /app)

  if is_podman; then
    args+=(--userns=keep-id --user "$(id -u)":"$(id -g)")
  else
    args+=(--user "$(id -u)":"$(id -g)")
  fi

  docker run "${args[@]}" "${IMAGE_NAME}" "$@"
}

docker_rebuild() {
  echo "[1/6] docker build -> ${IMAGE_NAME}"
  docker build -t "${IMAGE_NAME}" -f "${DOCKERFILE}" "${ROOT}"
}

docker_cmake_reconfigure() {
  echo "[2/6] cmake reconfigure (clean) in docker -> ${BUILD_DIR}"
  rm -rf "${ROOT:?}/${BUILD_DIR}"
  mkdir -p "${ROOT}/${BUILD_DIR}"

  docker_run cmake -S . -B "${BUILD_DIR}" -G "${CMAKE_GENERATOR}" -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
}

docker_cmake_build() {
  echo "[3/6] cmake build in docker"
  docker_run cmake --build "${BUILD_DIR}" --parallel "${JOBS}"
}

run_host() {
  echo "[4/6] run on host"
  local bin="${ROOT}/${BUILD_DIR}/task_manager"
  [[ -x "${bin}" ]] || die "Binary not found/executable: ${bin}"

  local args=()
  if [[ -n "${APP_CONFIG}" ]]; then args+=(--config "${APP_CONFIG}"); fi
  if [[ -n "${APP_DB}" ]]; then args+=(--db "${APP_DB}"); fi

  echo "Running: ${bin} ${args[*]}"
  "${bin}" "${args[@]}"
}

venv_setup() {
  echo "[5/6] venv setup + install requirements"
  command -v python3 >/dev/null 2>&1 || die "python3 not found"
  if [[ ! -d "${VENV_DIR}" ]]; then
    python3 -m venv "${VENV_DIR}"
  fi

  "${VENV_DIR}/bin/python" -m pip install --upgrade pip setuptools wheel

  if [[ -f "${REQ_FILE}" ]]; then
    "${VENV_DIR}/bin/pip" install -r "${REQ_FILE}"
  else
    die "Requirements file not found: ${REQ_FILE} (set REQ_FILE env var if другой путь)"
  fi
}

buildrun_1_4() {
  docker_rebuild
  docker_cmake_reconfigure
  docker_cmake_build
  run_host
}

all_1_5() {
  docker_rebuild
  docker_cmake_reconfigure
  docker_cmake_build
  venv_setup
  run_host
}

usage() {
  cat <<EOF
Usage: ./scripts/dev.sh <command>

Commands:
  docker:rebuild          (1) rebuild container
  cmake:reconfigure       (2) rm -rf build + cmake configure in docker
  cmake:build             (3) build in docker
  run                     (4) run on host (binary: build/task_manager)

  venv:setup              (5) create venv + pip install -r requirements.txt

  buildrun                run steps 1-4 in one command
  all                     run steps 1-5 (runs app at the end)

Environment knobs:
  IMAGE_NAME, BUILD_DIR, CMAKE_BUILD_TYPE, CMAKE_GENERATOR, JOBS
  VENV_DIR, REQ_FILE
  APP_CONFIG, APP_DB

EOF
}

cmd="${1:-}"
case "${cmd}" in
  docker:rebuild) docker_rebuild ;;
  cmake:reconfigure) docker_cmake_reconfigure ;;
  cmake:build) docker_cmake_build ;;
  run) run_host ;;

  venv:setup) venv_setup ;;

  buildrun) buildrun_1_4 ;;
  all) all_1_5 ;;

  ""|-h|--help|help) usage ;;
  *) usage; die "Unknown command: ${cmd}" ;;
esac