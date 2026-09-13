# Moonleashed

Flipper Zero firmware based on [Unleashed](https://github.com/DarkFlippers/unleashed-firmware), rearranged so the **BLE Full** radio stack fits.

> [!WARNING]
> Experimental firmware for experimental use. Do not use it for anything illegal.
> This project is independent. It is not affiliated with Flipper Devices, the Unleashed team, or Moon Firmware.

## Why

The BLE Full stack installs at `0x080CE000`, so CPU1 firmware has to end below 843,776 bytes. Stock Unleashed is about 860,000 bytes and does not fit. BLE Full is what enables central role and raw HCI access to the controller, which BLE Light cannot provide.

Moonleashed gets under the limit by moving large, rarely-resident pieces onto the SD card, and by running big apps from flash instead of RAM. It does this without removing firmware exports that SD-card apps rely on.

| | Unleashed | Moonleashed |
|---|---|---|
| CPU1 firmware | ~860,000 B | ~773,000 B |
| Radio stack | BLE Light | BLE Full |
| App API | 88.9 | 88.10 |

## What is different from Unleashed

- **JavaScript engine on the SD card.** MJS is linked into JS Runner and the `js` CLI command instead of firmware. JS modules resolve it through JS Runner.
- **XIP (execute in place), ported from [Moon Firmware](https://github.com/KaraZajac/Moon-Firmware).** An app's read-only code can run from a flash region between the firmware and the radio stack instead of RAM. JS Runner and Sub-GHz use it, which is what gives scripts like `gui.js` enough heap. Plugins fall back to the region when RAM runs short. Apps opt in with the `ForceXIP` manifest flag, bit-compatible with Momentum and Moon.
- **Sub-GHz and Archive run from the SD card.** Archive reopens on the same tab and item after launching something.
- **Level-up animation loads from the SD card.**
- **BLE Full raw HCI mode.** An app can suspend the Bluetooth service (`bt_profile_suspend`) and drive the controller directly (`furi_hal_bt_hci_*`).
- **No post-update slideshow.**

Everything else follows upstream Unleashed. See its README and changelog for features.

## Compatibility

- Apps built for Unleashed or official firmware on API 88.x load as usual. libnfc, liblfrfid and the rest of the firmware API are still exported.
- `mjs_*` is no longer exported by firmware. JS modules are unaffected. A standalone app calling MJS directly would fail to load, but none are known.

## Known limitations

- **XIP is off while Bluetooth is connected.** Erasing flash would drop the link, so XIP apps load into RAM instead. `gui.js` or Sub-GHz's Frequency Analyzer can then run out of memory. Disconnect and try again.
- **One XIP app at a time.** Switching between JS Runner and Sub-GHz rewrites the region, which adds a flash erase to launch.
- **Tight headroom.** The XIP region is 69,632 bytes against a 65,536-byte minimum. If upstream firmware grows by about 5 KB, XIP turns off until something else moves out.

## Install

Download `flipper-z-f7-update-*.tgz` from [Releases](../../releases) and install it with qFlipper (**Install from file**) or the Flipper mobile app. The package includes the BLE Full radio stack.

To go back to Unleashed or official firmware, install one of their full update packages, which carry their own radio stack.

## Build

```sh
./fbt COMPACT=1 DEBUG=0 updater_package \
  COPRO_OB_DATA=scripts/ob_custradio.data \
  COPRO_STACK_BIN=stm32wb5x_BLE_Stack_full_fw.bin \
  COPRO_STACK_TYPE=ble_full
```

Checks worth running after changes:

```sh
./fbt lint_js_api        # JS Runner's MJS table matches the firmware API list
./fbt lint_fap_imports   # every built app and plugin can resolve its imports
```

See [HowToBuild](/documentation/HowToBuild.md) for the toolchain.

## Upstream sync

`main` takes every Unleashed release by merge. A daily GitHub Actions workflow merges the release, builds BLE Full, and publishes the package. See `.github/workflows/sync-upstream-release.yml`.

## Credits

- [Unleashed Firmware](https://github.com/DarkFlippers/unleashed-firmware) by @xMasterX and the Unleashed team and contributors: the base of this firmware.
- [Flipper Devices](https://github.com/flipperdevices/flipperzero-firmware): the official firmware underneath it.
- [Moon Firmware](https://github.com/KaraZajac/Moon-Firmware): the XIP loader.
- [Momentum Firmware](https://github.com/Next-Flip/Momentum-Firmware): the application manifest flags.

## License

GPLv3, as upstream. See [LICENSE](/LICENSE).
