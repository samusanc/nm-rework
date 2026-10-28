# ft_nm test image: plain Debian (what the 42 machines and evaluators use),
# pinned so the reference `nm` (GNU binutils) is the same on every machine.
#
#   docker compose build
#   docker compose run --rm nm            # the whole test suite
#   docker compose run --rm nm bash       # a shell with gcc, nm, make, python3
FROM debian:trixie-slim

RUN set -eux; \
	apt-get update; \
	apt-get install -y --no-install-recommends \
		build-essential binutils make python3 git ca-certificates; \
	# 32-bit toolchain, so the x86 (ELF32) code path is exercised too
	if [ "$(dpkg --print-architecture)" = amd64 ]; then \
		apt-get install -y --no-install-recommends gcc-multilib; \
	fi; \
	rm -rf /var/lib/apt/lists/*; \
	# the repo is bind-mounted from the host with a different owner
	git config --system --add safe.directory '*'

WORKDIR /app
CMD ["bash", "tests/run_tests.sh"]
