# LiteOS host dependencies and cross-toolchain

The normal build uses a real `x86_64-elf` cross compiler. `make all`, `make
run`, and `make test` first check/install host build dependencies, then reuse a
valid cross-toolchain already on `PATH` or under `TOOLCHAIN_ROOT`; if none is
available, they build the pinned GNU Binutils/GCC toolchain in
`build/toolchain`. The build never silently substitutes host GCC/binutils.

```sh
# Requires GNU Make already installed (use gmake on macOS).
make all
make test                 # Includes the actual QEMU boot/interaction test.
make toolchain            # Install prerequisites and build/validate the toolchain only.

# Skip automatic system package installation; fails clearly if anything is missing.
AUTO_INSTALL_DEPS=0 make all

# Reuse/build under a chosen prefix.
make TOOLCHAIN_ROOT=/opt/x86_64-elf all

# Host compiler is an explicit development-only opt-in; not cross-build evidence.
make TOOLCHAIN_MODE=host all
```

## Automatic host dependency installation

`scripts/install-build-deps.sh` detects missing tools and installs packages with
one of these supported package managers: `apt-get` (Debian/Ubuntu), `dnf`
(Fedora/RHEL), `pacman` (Arch Linux), or Homebrew (`brew`, macOS). Linux package
installation needs root or usable `sudo`; Homebrew installs as the current user.
`AUTO_INSTALL_DEPS=0` disables installation and reports missing prerequisites.
On systems without a supported package manager, install the dependencies
manually. The prerequisites cover a host C/C++ compiler, GNU Make, GCC build
tools (`bison`, `flex`, `gawk`, `texinfo`/`makeinfo`, `m4`), Python, download
and archive tools, the GNU compiler math libraries, GRUB/ISO and mtools tools,
and QEMU.

GNU Make itself must already be installed to invoke `make`; the Makefile cannot
install the program needed to parse it. macOS's built-in BSD `make` is not
supported: install Homebrew GNU Make and invoke `gmake` (ensure Homebrew's bin
directory is on `PATH`). `make all` then performs dependency detection and
installation for the remaining prerequisites.

## Pinned toolchain build

The bootstrap downloads source archives only from `https://ftp.gnu.org/gnu/`,
checks their SHA-256 values before extraction, and builds a C-only compiler
without a target libc. No executable compiler is downloaded or checked into
the repository.

| Component | Version | Source archive SHA-256 |
| --- | --- | --- |
| GNU Binutils | 2.46.1 | `e127a709cba24c76de8936cb7083dd768f28cd37eb010492e2f19b71eb1294e4` |
| GNU GCC | 14.2.0 | `a7b39bc69cbf9e25826c5a60ab26477001f7c08d85cec04bc0e29cabed6f3cc9` |

The archive hashes provide integrity checks against the pinned values; the
bootstrap does **not** verify detached GNU signatures. The compiler is built
with C language support only and no target headers/libc. GCC's host-side
build requires a working host C compiler and the libraries/tools installed by
the supported package manager. The default install prefix is `build/toolchain`
(ignored by Git); `JOBS` can override parallel build jobs.

A candidate compiler is reused only after all required target executables
(`gcc`, `as`, `ld`, `ar`, `objcopy`, `strip`) are present, GCC reports the
exact `x86_64-elf` target, and a freestanding object can be linked. The normal
build additionally validates all configured tools. `TOOLCHAIN_ROOT` may point
to a custom prefix containing `bin/x86_64-elf-*`; with no custom prefix, a
complete toolchain on `PATH` is preferred, otherwise the default prefix is
bootstrapped.

`make toolchain-check` is side-effect-free and validates without installing or
building. `TOOLCHAIN_MODE=host` is an explicit opt-in for local development
only; it does not certify a cross build. Tool variables `CC`, `AS`, `LD`, `AR`,
`OBJCOPY`, and `STRIP` remain configurable, but cross mode rejects a compiler
whose `-dumpmachine` does not match `TARGET`.
