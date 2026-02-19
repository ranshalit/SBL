# Copilot instructions for this workspace (Slim Bootloader / SBL)

## Scope and boundaries
- Main editable tree: `slimbootloader/`.
- `slimbootloader_04-Dec-2023_SBL0005/` is a historical snapshot; avoid editing it unless explicitly requested.
- Root `README.md` is project context, but implementation patterns come from `slimbootloader/`.

## Documentation map (`docs/`)
- `docs/conga-TC570.pdf` and `docs/TCTL.pdf` are TC570 SOM/module-focused references.
- `docs/TEVA2.pdf`, `docs/TEVA2GA0_Overview.pdf`, and `docs/TEVA2SA2.pdf` cover conga-TEVAL carrier board variants.
- `docs/slimbootloader.github.io/` is the offline Slim Bootloader documentation mirror.
- `docs/secured_boot/` contains secure boot materials and lab/reference PDFs.
- Treat these docs as hardware/context input when implementing board-specific changes (especially CFGDATA, platform IDs, stitch flows, and secure boot settings).

## Big-picture architecture
- Build orchestration is centralized in `slimbootloader/BuildLoader.py`.
  - `build` discovers `Platform/*BoardPkg/BoardConfig*.py`, instantiates `Board`, then drives image generation.
  - `clean` removes `Build`, `Conf`, and generated artifacts; `build_dsc` supports direct DSC builds.
- Board config is Python composition, not a static manifest:
  - `BoardConfig.py` subclasses `BaseBoard` and sets sizing, feature toggles, keys, payloads, and flash layout.
  - Example: `Platform/TigerlakeBoardPkg/BoardConfig.py` controls `_CFGDATA_EXT_FILE`, stage sizes, FWU, Boot Guard related sizes.
- Core boot flow is stage-based (`Stage1A -> Stage1B -> Stage2 -> Payload`) under `BootloaderCorePkg/Stage1A|Stage1B|Stage2`, reflected by QEMU log checks in `Platform/QemuBoardPkg/Script/TestCases/linux_boot.py`.
- Config data pipeline is first-class:
  - YAML definitions under `Platform/*BoardPkg/CfgData/CfgDataDef.yaml` + delta `.dlt` files.
  - Build stitches CFGDATA via tooling in `BootloaderCorePkg/Tools` and board hooks.
- Final IFWI stitching is platform-script driven (`Platform/*BoardPkg/Script/StitchLoader.py`, `StitchIfwi.py`, `StitchIfwiConfig_*.py`) and often consumes board-specific binary blobs under `Platform/*BoardPkg/Binaries`.

## Critical developer workflows
- Typical build from `slimbootloader/`:
  - `python BuildLoader.py build tgl`
  - release: `python BuildLoader.py build tgl -r`
  - explicit payload: `python BuildLoader.py build tgl -p "OsLoader.efi:LLDR:Lz4;UEFIPAYLOAD_RELEASE.fd:UEFI:Lzma"`
- Clean builds: `python BuildLoader.py clean` (or `clean -d` for distclean).
- Stitch IFWI (Tigerlake example):
  - `python Platform/TigerlakeBoardPkg/Script/StitchLoader.py -i <base_ifwi.bin> -s Outputs/tgl/SlimBootloader.bin -o Build/<output>.bin -p <platform_id>`
- QEMU validation path:
  - build QEMU image, then run `python Platform/QemuBoardPkg/Script/qemu_test.py`.
  - targeted tests: `python Platform/QemuBoardPkg/Script/qemu_test.py linux_boot`.

## Prerequisites and constraints
- Linux dependencies are captured in `slimbootloader/Dockerfile` (e.g., `build-essential`, `nasm`, `uuid-dev`, `gcc-multilib`, `qemu-system-x86`, stitching GUI libs).
- BaseTools must exist; `BuildLoader.py` auto-rebuilds them (`make -C BaseTools`) if binaries are missing.
- Default signing key directory is `../SblKeys` via `SBL_KEY_DIR` if env var is not set.
- Generated outputs land under `Outputs/<board>/` (e.g., `SlimBootloader.bin`, `FlashMap.txt`) and are inputs to stitching / test workflows.

## Conventions for edits
- Prefer board-level changes in `Platform/<BoardPkg>/BoardConfig.py` over hardcoding in shared core code.
- For new board variants, update CFGDATA delta lists (`_CFGDATA_EXT_FILE`) and related `CfgData/*.yaml|*.dlt` consistently.
- Keep signing/keys and hash algorithm choices aligned with existing board fields (`_RSA_SIGN_TYPE`, `_SIGN_HASH`, key IDs) instead of ad-hoc scripts.
- Keep stage/region size edits coherent: BoardConfig has guard checks for redundant/non-redundant region overflow; adjust related sizes together.
- Use QEMU scripts for behavior checks when possible before touching hardware-specific stitch flows.

## Device defaults used by automation
- `target_ip`: `192.168.55.1`
- `target_user`: `ubuntu`
- `target_password`: `ubuntu`
- `target_prompt_regex`: `(?:<username>@<username>:.*[$#]|[$#]) ?$`
- `target_serial_device`: `/dev/ttyUSB0`
- `target_mac`: `3c:6d:66:62:a2:11`
