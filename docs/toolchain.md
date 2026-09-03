# Toolchain setup
Everything the `make` targets need, and how to install it. None of this is needed to build in
Microchip Studio, which the [README](../README.md) covers; this is the terminal route, where the
firmware is compiled, flashed, tested and formatted by the scripts in `ci/`.

Two setups are described: **Git Bash on Windows**, the shell that comes with Git for Windows, and
**Linux or WSL**, where a package manager does nearly all of it. The `make` targets are identical
either way.

---

## What each target needs

| Target                             | Needs                                                                   |
| ---------------------------------- | ----------------------------------------------------------------------- |
| `make build`                       | `avr-gcc`, `avr-objcopy`, `avr-size`, and the AVR-Dx device family pack  |
| `make flash`                       | the above, plus `avrdude` and a serial UPDI adapter                     |
| `make test`                        | `g++`, and the `libs/test` submodule checked out                        |
| `make format`, `make format-check` | `clang-format`                                                          |

Every target also needs `make` itself and a `bash` to run the scripts in `ci/` with. Git Bash
provides the shell; Linux has both already.

The scripts say which tool is missing rather than failing obscurely, so running a target is a
reasonable way to find out what is still needed:

```
error: avr-gcc not found. Install it, e.g. 'sudo apt -y install gcc-avr'.
```

---

## Windows, in Git Bash
Run everything below from the **Git Bash (MINGW64)** shell, not from `cmd` or PowerShell — the
`ci/` scripts are bash, and `make clean` uses `rm`. The `winget` commands are the exception; those
are PowerShell.

### Adding something to PATH
Each step below ends with a directory that has to be findable from the shell, and they all follow
the same pattern:

```bash
echo 'export PATH="/c/some/directory/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

Three details are worth getting right. Use **single quotes** around the `echo` argument, so that
`$PATH` is written into the file literally and expanded when the shell starts, rather than being
frozen to today's value. Point at the **directory** holding the executable, never at the
executable itself. And let `cygpath` convert a Windows path rather than typing the Unix spelling
by hand, since it handles the drive letter and the spaces:

```bash
cygpath -u "C:\Program Files\LLVM\bin"
```

If an edit to `~/.bashrc` seems to do nothing, check `~/.bash_profile`. Git Bash starts as a login
shell, which reads `~/.bash_profile` and not `~/.bashrc`, so an existing profile that does not
source it silently ignores everything added above:

```bash
cat ~/.bash_profile   # Should contain: [ -f ~/.bashrc ] && . ~/.bashrc
```

Git Bash writes that profile itself the first time it finds a `~/.bashrc` without one, and warns
while doing so. The warning is harmless and does not come back.

### make
Git Bash has no package manager and ships no `make`, so install one:

```powershell
winget install ezwinports.make
```

WinGet unpacks it rather than putting it on PATH, so find the binary and add its directory:

```bash
find "$(cygpath -u "$LOCALAPPDATA")/Microsoft/WinGet/Packages" -name make.exe
```

```bash
echo 'export PATH="$HOME/AppData/Local/Microsoft/WinGet/Packages/ezwinports.make_Microsoft.Winget.Source_8wekyb3d8bbwe/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
make --version
```

Chocolatey (`choco install make`) and Scoop (`scoop install make`) both package it too, and put it
on PATH by themselves. The package directory above contains a hash and is replaced wholesale when
WinGet upgrades the package, so if `make` disappears one day, run the `find` again.

### avr-gcc
Look before installing anything: Microchip Studio and MPLAB XC8 both ship a complete AVR
toolchain, and one of them is often already on the machine.

```bash
find "/c/Program Files (x86)/Atmel" "/c/Program Files/Microchip" -name avr-gcc.exe 2>/dev/null
```

Prefer the **Studio** toolchain over the XC8 one. It is the plain GNU toolchain, while XC8 in its
free mode restricts the optimization levels. Its directory holds `avr-objcopy` and `avr-size` as
well, which the build also uses, so one PATH entry covers all three:

```bash
echo 'export PATH="/c/Program Files (x86)/Atmel/Studio/7.0/toolchain/avr8/avr8-gnu-toolchain/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
avr-gcc --version
```

The path contains spaces, which is why the inner double quotes are there.

With no Studio installed, download a prebuilt toolchain instead. Microchip publishes the official
`avr8-gnu-toolchain-*-win32.any.x86_64.zip`, and Zak Kemble publishes newer Windows builds whose
`avr-libc` already knows the AVR-Dx parts. Either works, since the device family pack below
supplies the device support in any case. Unpack it somewhere **without spaces**, such as
`C:\avr\avr-gcc`, and add its `bin` directory as above.

### The device family pack
`avr-libc` ships neither the device specs nor `<avr/io.h>` for this part; the AVR-Dx device family
pack (DFP) supplies both. `ci/build.sh` looks for it in `dfp/` inside the repository first, and
then in a local Microchip Studio installation, which it finds by itself. **With Studio installed
there is nothing to do here.**

Without Studio, unpack a copy into `dfp/`. An `.atpack` file is a zip archive, and Git Bash ships
both `curl` and `unzip`, so no extra tool is needed:

```bash
curl -LO http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack
unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack
```

An older Git Bash without `unzip` can use Windows' own `tar` instead, which reads zip archives
where the `tar` in Git Bash does not: `/c/Windows/system32/tar.exe -xf <pack> -C dfp`.

`DFP_DIR` points the build at a pack kept somewhere else:

```bash
make build DFP_DIR=/c/avr/packs/AVR-Dx_DFP
```

Keep that path free of spaces. AVR tooling and `make` both handle them badly, which is a good
reason to copy a pack out of `C:\Program Files (x86)\...` rather than pointing at it in place.

### avrdude
Needed only for `make flash`, and only with a serial UPDI adapter, i.e. a USB-to-serial cable
wired to the UPDI pin. Microchip Studio does not include it; its own programmers are driven from
Studio instead, as the README describes.

Download the Windows zip from the [avrdude releases](https://github.com/avrdudes/avrdude/releases),
unpack it somewhere without spaces such as `C:\avr\avrdude`, and add that directory to PATH.
Chocolatey and Scoop both package it as `avrdude` if you already use one of them.

The port defaults to `COM3`; Device Manager lists which one the adapter actually took:

```bash
make flash PORT=COM4
```

### clang-format
Look for an existing copy first. Both the VS Code C/C++ extension and Visual Studio bundle one:

```bash
ls "$HOME/.vscode/extensions/ms-vscode.cpptools-"*/LLVM/bin/clang-format.exe
ls "/c/Program Files/Microsoft Visual Studio"/*/*/VC/Tools/Llvm/bin/clang-format.exe
```

Either is fine to add to PATH. Otherwise install one of the two below.

**Through pip**, which is the smaller download and the one that can be pinned to a version:

```bash
pip install clang-format==18.1.8
```

It installs a prebuilt binary, so nothing is compiled. The command itself lands in Python's
scripts directory, which recent Python versions put under `AppData\Local` rather than
`AppData\Roaming`; ask Python where it is rather than guessing:

```bash
python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
```

Add that directory to PATH as above. If pip finds no matching wheel for a very new Python and
starts building from source, stop it and use LLVM instead.

**Through LLVM**, which is a much larger install but also brings `clang-tidy` and `clangd`:

```powershell
winget install LLVM.LLVM
```

```bash
echo 'export PATH="/c/Program Files/LLVM/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
clang-format --version
```

The version matters more than it looks. `clang-format` changes its output between major versions,
so code formatted by one version can fail `make format-check` under another without anyone having
touched it. The CI pipeline installs the version its Ubuntu runner packages, 18 at the time of
writing, so a local 18 keeps the check honest.

---

## Linux and WSL
One package manager line covers the whole toolchain:

```bash
sudo apt -y update
sudo apt -y install gcc-avr binutils-avr avr-libc avrdude g++ make clang-format
```

The device family pack is downloaded and unpacked once, into `dfp/` where the build looks for it:

```bash
wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.1.10.114.atpack
unzip -q -d dfp Atmel.AVR-Dx_DFP.1.10.114.atpack
```

On WSL the build also finds a pack from a Windows-side Microchip Studio installation, since it
tries the `/mnt/c/...` spelling of that path as well.

Flashing from WSL needs the USB serial adapter passed through to the Linux side, which is more
trouble than it is worth for a one-line command. Running `make flash` from Git Bash on the Windows
side, against the same `build/main.hex`, is the easier route.

---

## The test framework
`make test` builds against yrgo::test, which is a git submodule rather than part of this
repository. A checkout that skipped it fails with
`error: test framework not found in libs/test`:

```bash
git submodule update --init
```

---

## Checking it worked
Open a **new** Git Bash window first, so that what is tested is what a fresh shell sees, and not
what `source ~/.bashrc` left behind in this one:

```bash
type -a make avr-gcc avrdude clang-format
avr-gcc --version
```

Then run the targets themselves, from the repository root:

```bash
make build         # Ends with the size of main.elf.
make test          # Ends with the test results; needs no board.
make format-check  # Silent when everything is already formatted.
```

`make build` printing `Building avr32db28 firmware with DFP ...` means the pack was found, and the
path it prints is the one it settled on.

---

## Troubleshooting

| Symptom | Cause | Fix |
| ------- | ----- | --- |
| `command not found` right after installing something | The PATH change has not reached this shell | Open a new Git Bash, or `source ~/.bashrc` |
| Edits to `~/.bashrc` change nothing | A `~/.bash_profile` exists that does not source it | Add `[ -f ~/.bashrc ] && . ~/.bashrc` to `~/.bash_profile` |
| `error: AVR-Dx device family pack not found` | No Studio installed and no `dfp/` in the repository | Unpack a pack into `dfp/`, or set `DFP_DIR` |
| `error: test framework not found in libs/test` | The submodule was never checked out | `git submodule update --init` |
| Recipe lines fail with `cmd`-style errors | `make` was run from PowerShell or `cmd` | Run it from Git Bash |
| A Unix path reaches the compiler as `C:\...` | MSYS rewrites arguments that look like paths | `export MSYS_NO_PATHCONV=1` before running `make` |
| Stray `\r` errors in recipes or scripts | The repository was checked out with CRLF line endings | `git config core.autocrlf false`, then check the files out again |
| `avrdude: ser_open(): can't open device` | The adapter is on another COM port | `make flash PORT=COM4`, the port Device Manager lists |
| `make` vanishes after `winget upgrade` | WinGet replaced the whole package directory | Run the `find` again and update `~/.bashrc` |
| `make format-check` fails on code nobody touched | A different `clang-format` major version than the one that formatted it | Install the version the pipeline uses |

---
