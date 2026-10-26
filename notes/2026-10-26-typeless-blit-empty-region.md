# Decision: skip typeless blits whose region covers no texel (Vulkan)

**Date:** 2026-10-26 · **Status:** applied, verified in-game (see Verification) · **Revert:** `git revert` the commit that added this file (fix + note travel together)

## Problem

RSX thread dies with `VK_ERROR_INITIALIZATION_FAILED` in `vk::mem_allocator_vma::alloc` during area
transitions (runs 1 and 3). The diagnostic build logged a zero-width scratch image
(`format=97 (R16G16B16A16_SFLOAT), extent=0x256x1`, memory requirement size 0). VMA rejects size-0
requests with INITIALIZATION_FAILED. This is not VRAM exhaustion (heap 1 at 448 MB of ~4.9 GB).

Call chain: `render_target::memory_barrier` → `blitter::scale_image` → `get_typeless_helper` → `vk::image` ctor → alloc.

## Mechanism (from code reading)

- `memory_barrier` sets `src_scaling_hint = src_bpp / dst_bpp` (e.g. 4/8 = 0.5 into an FP16 RGBA target).
- `scale_image` normalizes the typeless source: `x2 = (int)(width * hint)`, which truncates. A 1-texel 4-byte
  source becomes 0 texels.
- The width was ≥1 before scaling: `deferred_clipped_region::src_rect()` does `ensure(width)`.
- `get_typeless_helper` aligns 0 up to 0 and creates a zero-width image. That only happens when no scratch
  image of that format/class exists yet, which is why it fails on the first occurrence in a run.
- scale_image already rejects empty regions ("Blit request consists of an empty region descriptor!"),
  but only *after* the helper allocation and the first `copy_image_typeless`.
- GL does not crash: it sizes its typeless texture from the whole surface, not the region.

Values not logged: the actual src area, hint and source format. The new warning records them.

## Decision: Option A

In `vk::blitter::scale_image` (`rpcs3/Emu/RSX/VK/VKTexture.cpp`), after normalizing the typeless src or dst
area, if the width or height is ≤0, log a warning and return before allocating. The existing late check stays.

Why skipping is correct, not just safe:
- **Texel-centre rule.** A texel belongs to a region only if its centre lies inside. The crashing region spans
  [0, 0.5) destination texels and contains no centre, so it covers zero texels. (Same rule as the coverage
  sampling in Visual_Project `fill.rs`.) D3D11 `CopySubresourceRegion` also defines an empty box as a no-op.
- **Check before allocate.** The same order as Visual_Project `view.rs`: compute bounds, reject empty, then
  create the resource.

## Rejected

- **B, clamp the width to ≥1 in `get_typeless_helper`:** no crash, but it still records zero-extent copies, which is
  invalid Vulkan usage, and hides the bug.
- **C, round up:** `copy_image_typeless` stages through a buffer: the source writes 4 bytes, the destination reads 8,
  so half the texel is stale scratch data. Outward rounding is only safe when a later stage masks the extra
  area (as coverage does in Visual_Project); this pipeline has no such stage.
- **D, integer bpp math:** the hints are exact powers of two, so float precision is not the issue; the
  rounding policy is. A large change for no gain.
- **E, fix in the surface cache now:** shared code (GL plus every game) and no logs yet of which surfaces
  overlap. Deferred, see below.

## Hardware check (Steam Deck, Zen 2 / RDNA2)

Disassembly of our `-march=native` build shows `vmulss` + `vcvttss2si` for each hint product: no add means no
FMA contraction, and `cvtt*` truncates regardless of MXCSR, as C++ requires. The `+0.5f` in `dst_rect()` may be
fused, but products of power-of-two ratios are exact, so the result is identical. The AppImage behaves the
same. The PPU/SPU JIT does not run this code. RADV adds only the spec rule (zero extent invalid).

## Follow-up (not in this commit)

The typeless width is computed in four places with two rules: `dst_rect()` rounds (+0.5), the
scale_image pass-through check and normalization truncate, and GL truncates on the whole surface. The crash is where
the destination (1) and source (0) disagree. The root fix is one shared helper with one documented rule
(Visual_Project ADR-0025 "single geometry authority" pattern). Do it only after the new warning shows how often
this happens in-game; consider an upstream report.

## How to verify

Play through many area transitions with the locked config. Grep the log for `Typeless blit skipped` and
confirm no `^·F` lines.

## Verification (2026-10-26, runs/2026-10-26-multi-boot)

The crash is tied to one place: an in-area transition inside D_21_1 (PhysX group rebuilt without a map load),
reached via Field_18 → Field_11 → D_21_1. Runs 1 and 3 hit the RSX fatal 0.652 s after that transition.
With the fix, the same transition logged the warning 0.604 s after it and the game continued. A second, similar
transition 0.578 s after a PhysX rebuild also fired the warning. The game ran on for more than 2 minutes, with no `^·F` lines.

Logged values (both firings): `src=1x1 -> 0x1, dst=1x1, hint=0.5, format=0x2c->0x61`
(B8G8R8A8_UNORM, 4 bpp, into R16G16B16A16_SFLOAT, 8 bpp). The source was 1 texel wide before scaling, so truncation
produced the 0, as predicted. Both sides are 1x1, which suggests a 1x1 post-process target (unverified).
