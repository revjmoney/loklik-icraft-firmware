# How this repo came alive

This is a mirror of the firmware source for the **LOKLiK iCraft** cutting machine,
together with the firmware images, the machine definitions, and the story of how a
dead, surge-bricked cutter turned into an unlocked open-source CNC — and how its
GPL'd source code finally saw daylight.

## The short version

The LOKLiK iCraft runs **Grbl_ESP32** — open-source software licensed under the
**GPLv3**. LOKLiK ships it behind a login-gated app and never tells you it's there.
The corresponding source was sitting in a GitHub repository that wasn't linked from
the app, the FlashTool, the product page, or the manual — and that, by every
outside indication, wasn't publicly visible until the day a customer pushed the
issue. (See [EVIDENCE.md](EVIDENCE.md) for the receipts.)

## The night (June 15, 2026)

A whole-house power surge took out the iCraft. The LOKLiK IdeaStudio app couldn't
see it; the machine replied with garbage bytes and every query timed out. Cracking
it open revealed an **ESP32-D0WD-V3** — and the chip had survived.

- Pulled a full 8 MB backup off the ESP32 with `esptool`.
- LOKLiK's standalone **FlashTool** downloads firmware to
  `com.testingtool.app/resources/esp32/LOKLIK_CRAFTER_2/v01..v08/firmware.bin`.
  The download folder says `CRAFTER_2`; the binary strings say `LOKLiK_iCraft`.
- Block-comparing the backup against each version showed the installed build was
  **V03**, with ~320 KB (5 of 24 blocks) corrupted by the surge.
- Flashed a clean **V03**, opened a serial terminal, and the machine spoke:

  ```
  [MSG:Grbl_ESP32 Ver 1.3a Date 20211103]
  [MSG:Using machine:LOKLiK_iCraft]
  ```

That's the reveal: the iCraft's "secret" firmware is **grbl_ESP32 — GPLv3 free
software** with LOKLiK's name on it. Sorting the 24 V motor rail brought back full
XYZ motion. A dead machine became an **unlocked, standard GRBL CNC** — drivable by
gSender, Fusion 360, F-Engrave, or anything that speaks G-code. No cloud, no login.

## The GPL angle

grbl_ESP32 is GPLv3. Anyone distributing it must make the corresponding source
available. The iCraft's source was **not** provided with the product, nor pointed
to anywhere a customer would find it. A formal **GPLv3 §6 source request** was sent
to LOKLiK, and the issue was raised publicly.

LOKLiK responded — politely — stating the firmware code "was uploaded to GitHub
back in 2023" and pointing to
[`HKsjtech/LoklikIdeaStudio-Firmware`](https://github.com/HKsjtech/LoklikIdeaStudio-Firmware).

The files are indeed stamped 2023 (GitHub's `created_at` confirms it). But the repo
was **pushed to on the same day** the request was escalated, had **zero stars,
zero watchers** after nearly three years, had **never been archived** by the
Internet Archive (the 2026-06-18 snapshot is its *first*), and was undiscoverable
by search. Whether it was literally private until that day can't be proven from the
outside — GitHub doesn't expose visibility history — but the source was, in
practice, **disclosed to no one** until a customer made noise. See
[EVIDENCE.md](EVIDENCE.md).

## What's in this repo

- A **clean, buildable mirror** of the LOKLiK/HKsjtech Grbl_ESP32 source
  (original commit history preserved as provenance).
- [`firmware-dumps/`](firmware-dumps/) — the 8 FlashTool firmware versions, the
  full 8 MB device backup, the original NVS, and partition tables.
- [`machines/`](machines/) — the iCraft pin map and motion tune extracted from the
  source, plus ready-to-use **FluidNC** and **Marlin** configurations for anyone
  rebuilding or converting one of these machines.
- [`EVIDENCE.md`](EVIDENCE.md) — the verifiable record.

## Credit where it's due

- **grbl** — Sungeon (Sonny) Jeon ([gnea/grbl](https://github.com/gnea/grbl)).
- **Grbl_ESP32 / FluidNC** — Bart Dring and contributors
  ([bdring/Grbl_Esp32](https://github.com/bdring/Grbl_Esp32),
  [bdring/FluidNC](https://github.com/bdring/FluidNC)).
- The LOKLiK/HKsjtech machine-specific layer (`SJMachine`, `SJCutter`, etc.).

This mirror exists so that owners of these machines can exercise the rights the
GPLv3 already grants them: to study, repair, modify, and rebuild the software that
runs their own hardware.
