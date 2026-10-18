# RX 7800 XT (gfx1101) on Windows: an experiment branch of Strata

This branch is my personal experiment log on top of Strata **v0.1.44.1**. It is **not** an official Strata release and
not a pull request. It exists so that the measurements I reported upstream can be traced to code.

Hardware: AMD Radeon RX 7800 XT 16 GB (gfx1101), Ryzen 5 9600X, 64 GB DDR5, NVMe, Windows 11, Windows HIP (built with the ROCm SDK 10.2.0a20260930; runtime `amdhip64_7.dll` 10.0.3686.0, hipBLASLt 1.5.0). Model: Qwen3.8-Flash-Next GSQ-RCO IQ3_XXS (and Swift 1.5 IQ3_XXS for some runs).

## Credits

- **Strata** by Niko1221 and the Strata contributors, MIT License. Everything not listed below is their code.
- **Layer-major prefill** (`STRATA_PREFILL_LAYER_MAJOR`, the layer-major expert kernels, the stager wait and the
  prompt-path direct reads) is by **Tomas Vizdal (@vizdatom)**, from his `layer-major-0.1.43.1` branch
  (github.com/vizdatom/Strata, commits `2e69b6db`, `f0173382`, `149b8995`, `7013dad4`, `bd900165`, `7753c2e2`).
  I ported it to this tree; see upstream issue #1670.
- **Prompt-attention HIP patches** `STRATA_ATTN_GATE` and `STRATA_QPREP` are by **libratechw**, taken from their
  patches of 2026-10-15.
- My own changes: the commit list below says which commit is whose.

## Commits on this branch

Each commit is the source snapshot of one optimization round, applied in order on top of `v0.1.44.1`.

| Commit | What it adds | Author of the idea / code |
| --- | --- | --- |
| round 2 | compact MMQ scratch buffer (`STRATA_MMQ_COMPACT`) and related prefill tweaks | me |
| round 3 | port of layer-major prefill + the two attention patches | @vizdatom, libratechw (ported by me) |
| round 3 arena | layer-major staging borrowed from the expert-cache arena (`STRATA_PREFILL_LM_CACHE_ARENA`, `STRATA_PREFILL_LM_CACHE_GUARD`) | me |
| round 4 | local experiments: `STRATA_CPU_IO_OVERLAP` | me |
| round 5 | local experiments: `STRATA_R5_FETCH_TEAM` | me |
| round 6 | Windows file tier: cached `ReadFile` after load (`STRATA_R6_FILE_READ_CACHE`) | me |

All new switches are opt-in environment variables; with none of them set the engine follows v0.1.44.1's paths, but I
have not verified byte-identical output against the release for every path.

## What was measured (summary)

Fresh 60,047-token prompt, 64K context, medians of 3. Details, per-run data and limitations are in the upstream reports
linked below.

| Step | Comparison | Prompt tok/s | Decode tok/s | TTFT s |
| --- | --- | --- | --- | --- |
| baseline | v0.1.41 as released, FP16 KV, RAM 24 GiB | 437.0 | 29.9 | 137.5 |
| gfx1101 hipBLASLt table | v0.1.44.1 release, Swift 24K, table off → on | 404.6 → 695.2 | 29.9 → 29.2 | 59.5 → 34.7 |
| layer-major (round 3) | same build lineage, same day, off → on, RAM 20 GiB | 896.6 → 1,177.4 | 26.0 → 27.0 | 67.1 → 51.1 |
| file cache (round 6) | same binary, same day, off → on, RAM 19 GiB | 1,399.8 → 1,476.4 | 28.5 → 36.0 | 43.1 → 40.7 |

The rows are separate comparisons with different settings; they do not multiply.

Known open problems: MTP GPU fence waits still happen in long coding runs, and a long-context system-prompt
precedence test (ALPHA → ORBIT) fails 0/3 (it also fails on v0.1.41 as released with FP16 KV at a 23 GiB budget).

## Upstream links

- Community benchmark (RX 7800 XT, Windows): PR https://github.com/Niko1221/Strata/pull/3054
- gfx1101 hipBLASLt table: PR https://github.com/Niko1221/Strata/pull/3055
- Windows cached ReadFile proposal: issue (submission pending)
- Layer-major on HIP/Windows: comment on #1670 (submission pending)

## Building

Same as Strata v0.1.44.1 (`docs/AMD_HIP.md`, `tools/hip/build_windows.bat`). I built only the HIP backend on Windows;
CUDA and SYCL builds of this branch are untested.
