# esp_hid (patched copy)

Copy of the ESP-IDF 5.5.5 `esp_hid` component (Apache-2.0, Espressif Systems), used instead of
the one in ESP-IDF: a project component with the same name overrides it.

The only change is `src/nimble_hidh.c`, with the upstream fix
[espressif/esp-idf@4c82b25c84](https://github.com/espressif/esp-idf/commit/4c82b25c84)
("fix(nimble): Fix nimble HID service discovery"). In 5.5.5 the HID service characteristics are
never processed: the report map is not read and no input report is registered, so no BLE
keyboard, mouse or gamepad works. This directory can be removed with an ESP-IDF release that
includes the fix.
