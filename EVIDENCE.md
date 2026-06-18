# EVIDENCE

A factual record compiled **2026-06-18** documenting the LOKLiK iCraft firmware,
its GPLv3 lineage, and the public availability of its source. Verifiable claims
are separated from reasonable inferences so readers can judge for themselves.

---

## 1. The machine runs GPL'd software (FACT)

The LOKLiK iCraft's controller is an **ESP32-D0WD-V3** (8 MB flash). On a clean
serial connection it announces itself:

```
[MSG:Grbl_ESP32 Ver 1.3a Date 20211103]
[MSG:Using machine:LOKLiK_iCraft]
[MSG:Axis count 3]
[MSG:Ready]
```

`Grbl_ESP32` is the [bdring/Grbl_Esp32](https://github.com/bdring/Grbl_Esp32)
project, which is licensed **GPLv3**. The iCraft ships a modified build of it.

## 2. The source builds, and matches the firmware on the device (FACT)

The repository at `github.com/HKsjtech/LoklikIdeaStudio-Firmware` was cloned and
compiled with PlatformIO (`pio run -e release`, platform `espressif32@3.0.0`):

| Check | Result |
|---|---|
| Build | **SUCCESS** — 0 errors, `firmware.bin` = 900,048 bytes (Flash 45.8% / RAM 14.8%) |
| Version string in source (`Grbl.h`) | `GRBL_VERSION = "1.3a"`, `GRBL_VERSION_BUILD = "20211103"` |
| Version on the device (boot banner) | `Ver 1.3a Date 20211103` — **exact match** |
| Grbl strings (built binary vs. FlashTool dumps) | **byte-identical**, including the internal path string `Grbl_Esp32/src/I2SOut.cpp` |

The machine definition is custom to LOKLiK: `Grbl_Esp32/src/Device/SJMachine.h`
(`SJ` = the GitHub owner **HKsjtech**), `MACHINE_NAME "LOKLIK_CRAFTER_1"`, with a
Chinese comment `戎图测试机` ("Rongtu test machine"). The shipped binaries instead
carry the string `iCraft`, and have Bluetooth/Wi-Fi compiled in (the source ships
with `ENABLE_BLUETOOTH` / `ENABLE_WIFI` commented out). These are configuration
differences, not withheld code.

> Note on names: LOKLiK uses at least two internal identifiers for this machine —
> `LOKLIK_CRAFTER_1` (in the source) and `LOKLIK_CRAFTER_2` (in the FlashTool
> download path `com.testingtool.app/resources/esp32/LOKLIK_CRAFTER_2/v01..v08/`).
> Both are sold to customers as the **iCraft**.

## 3. The source was on GitHub since 2023 — but disclosed to no one (FACT)

GitHub's own API (`https://api.github.com/repos/HKsjtech/LoklikIdeaStudio-Firmware`,
captured 2026-06-18, raw JSON in [`evidence/`](evidence/)):

| Field | Value |
|---|---|
| `created_at` | `2023-09-05T03:12:32Z` |
| `updated_at` | `2023-09-05T08:40:38Z` |
| `pushed_at`  | **`2026-06-18T03:05:20Z` (the day this was compiled)** |
| `stargazers_count` | `0` |
| `watchers` | `0` |
| commits | 4, all dated `2023-09-05`, all "Add files via upload" (web-UI uploads) |
| owner `HKsjtech` | account created `2023-08-18`, 15 public repos |

Internet Archive (Wayback Machine):
- `http://archive.org/wayback/available?url=github.com/...` → `{"archived_snapshots":{}}`
  (no snapshots).
- A snapshot taken 2026-06-18 04:22:03 UTC was reported by the Archive as the
  **"First Archive"** of this URL:
  `https://web.archive.org/web/20260618042203/https://github.com/HKsjtech/LoklikIdeaStudio-Firmware`

## 4. What is proven vs. inferred

**Proven:** The firmware files existed *on GitHub's servers* since 2023-09-05
(GitHub sets `created_at`; it is not user-editable). The repo was **pushed to on
2026-06-18** — an action only the owner can perform. It has never been starred,
watched, or archived. The source was **not linked or referenced** in the LOKLiK
IdeaStudio app, the FlashTool, the product page, or the printed materials, and it
did not surface in a GitHub search for the product before 2026-06-18.

**Inferred (reasonable, not provable from outside):** The combination of a push on
the same day the customer escalated, zero stars/watchers after ~2.75 years, no
prior archive, and the repository being undiscoverable is consistent with the
repository having been **private (or otherwise non-public) until 2026-06-18**.
GitHub's public API does not expose visibility history, so this cannot be proven
from the outside — it is an inference from the surrounding facts.

**The customer's experience (FACT):** The source was not provided or pointed to
until a formal GPLv3 §6 source request was made and the issue was raised publicly.
The vendor then replied directing the customer to the GitHub repository.

---

*Raw evidence files are in [`evidence/`](evidence/). Firmware images pulled from
the device and from LOKLiK's FlashTool are in [`firmware-dumps/`](firmware-dumps/).*
