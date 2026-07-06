# This file is governed by the terms outlined in the LICENSE file located in the root directory of
# this repository. Please refer to the LICENSE file for comprehensive information.
"""
Standalone signing + flashing script — Method 1 from standalone_flow_proposal.drawio.

Self-contained: no 'backend' or 'tpds' package imports.
Dependencies (pip install): requests  cryptography  intelhex  pyyaml

PHASE A — Offline signing on the host  (mirrors the TPDS FoTA flow)
    * Calls GET /trustkeypairs to fetch the signer public key (saved as <key>.pem).
    * Calls POST /sign to obtain the ECDSA signature.
    * Patches the rawSignature (r||s, 64 B) into the input hex at `sign_addr`
      and writes <name>_<ver>.hex/.bin (per component) and combined_component.hex/.bin.
    * Signature verification is NOT done on the host — the bootloader verifies
      each image on-device using the signer public key in ATECC608 slot 14.

PHASE A2 — ATECC608 provisioning (optional, [provision] enable = true)
    * Writes the signer public key into ATECC608 slot 14 via atcab_write_pubkey
      so the bootloader can verify FOTA images (TPDS uses slot 15; this app uses 14).

PHASE B — Flash signed image
    * Calls ipecmd via subprocess to flash combined_component.hex.
    * Host does NOT touch the ATECC608 (no atcab_write_pubkey, no atcab_read_pubkey).

Usage
-----
    # Phase A only (sign + patch + write outputs)
    python standalone_sign_and_flash.py sign --config mchp_non_tpds_fota_config.yaml

    # Phase B only (flash an already-signed hex)
    python standalone_sign_and_flash.py flash --image combined_component.hex

    # Both phases in one shot
    python standalone_sign_and_flash.py all --config mchp_non_tpds_fota_config.yaml

Config YAML schema (mchp_non_tpds_fota_config.yaml)
-------------------------------------
    Fleet_uid:            "<Fleet Profile Public UID>"
    keySTREAM_API_Token:  "<API Key, with or without 'Basic ' prefix>"
    keySTREAM_Signingkey: "<keySTREAM signing key name>"
    comp_1:               "path/to/component1.hex"        # required
    comp_1_info:          "0x<info-struct address>"       # required
    comp_2:               "path/to/component2.hex"        # optional
    comp_2_info:          "0x<info-struct address>"       # required when comp_2 set
    output_dir:           "./signed_out"                  # optional, defaults to CWD
    # Flash settings (used by 'flash' and 'all' subcommands)
    ipecmd:               "C:/Program Files/Microchip/MPLABX/v6.20/mplab_platform/bin/ipecmd.exe"
    mcu:                  "ATSAMD21E18A"
    programmer:           "nEDBG"
"""

from __future__ import annotations

import argparse
import configparser
import os
import struct
import subprocess
import sys
import traceback
from base64 import b64decode
from pathlib import Path

# ---------------------------------------------------------------------------
# Set to True to print full Python tracebacks on errors; False shows only the
# concise error message (default for normal use).
# ---------------------------------------------------------------------------
VERBOSE_ERRORS: bool = False

import requests
import yaml
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from intelhex import IntelHex


# ---------------------------------------------------------------------------
# Inlined KeyStream client — copied from backend/key_stream/key_stream.py
# (no modifications; keeps identical API surface)
# ---------------------------------------------------------------------------

class KeyStream:
    """Kudelski IoT keySTREAM signing client."""

    def __init__(
        self,
        keystream_auth_token: str,
        pub_uid: str,
        keystream_endpoint: str = "https://mss.iot.kudelski.com",
    ):
        self.keystream_endpoint = keystream_endpoint
        self.pub_uid = pub_uid
        self.headers = {
            "accept": "application/json",
            "x-correlation-id": "ISEPDMUI-4d1cff76-7ace-4fda-0811-488fb8d99cbf",
            "Authorization": (
                keystream_auth_token
                if keystream_auth_token.startswith("Basic ")
                else f"Basic {keystream_auth_token}"
            ),
        }
        self.dm_uuid = None
        self._get_dm_uuid()

    def _get_dm_uuid(self):
        url = f"{self.keystream_endpoint}/dm?dpPublicUid={self.pub_uid}"
        resp = requests.get(url=url, headers=self.headers)
        assert resp.status_code == 200, (
            "Connection failed — check Fleet_uid and keySTREAM_API_Token."
        )
        data = resp.json()
        assert data.get("totalRecords") != 0, "Invalid Fleet Profile Public UID."
        self.dm_uuid = data["deviceManagers"][0]["uuid"]

    def sign(self, key_name: str, digest: bytes) -> dict:
        """POST /sign — the only keySTREAM call used by Phase A."""
        url = f"{self.keystream_endpoint}/lp/dm/{self.dm_uuid}/trustkeypairs/{key_name}/sign"
        resp = requests.post(url=url, headers=self.headers, json={"digest": list(bytearray(digest))})
        data = resp.json()
        assert resp.status_code == 200, (
            f"Sign failed — HTTP {resp.status_code}: {data.get('message')}"
        )
        return {
            "rawSignature": b64decode(data["rawSignature"]),
            "asn1Signature": b64decode(data["asn1Signature"]),
        }

    def get_trusted_pub_key(self, key_name: str) -> dict:
        """GET /trustkeypairs — fetch the signer public key (raw 64B + ASN.1)."""
        url = f"{self.keystream_endpoint}/lp/dm/{self.dm_uuid}/trustkeypairs/?name={key_name}"
        resp = requests.get(url=url, headers=self.headers)
        data = resp.json()
        assert resp.status_code == 200, (
            f"Fetch signing key '{key_name}' failed — HTTP {resp.status_code}: "
            f"{data.get('message')}"
        )
        keys = data["trustKeyPairs"][0]
        return {
            "rawPublicKey": b64decode(keys["rawPublicKey"]),      # 64 B raw P-256
            "asn1PublicKey": b64decode(keys["asn1PublicKey"]),   # SubjectPublicKeyInfo
        }

    def caluclate_digest(self, data: bytes, hash_algo=None) -> bytes:
        algo = hash_algo or hashes.SHA256()
        d = hashes.Hash(algo, backend=default_backend())
        d.update(data)
        return d.finalize()


# ----------------------------------------------------------------------------
# Helpers
# ----------------------------------------------------------------------------

INFO_STRUCT_FMT = "<II16s16sIII"   # same layout used by FoTA.generate_resources
INFO_STRUCT_LEN = struct.calcsize(INFO_STRUCT_FMT)   # 52


def _log(msg: str) -> None:
    print(msg, flush=True)


def _load_config(path: str) -> dict:
    with open(path, "r", encoding="utf-8") as f:
        cfg = yaml.safe_load(f) or {}
    for required in ("pub_uid", "keystream_auth_token", "key_name", "comp_1", "comp_1_info"):
        if not cfg.get(required):
            raise ValueError(f"Missing required config field: {required}")
    return cfg


# ---------------------------------------------------------------------------
# Signer public-key helpers (mirror TPDS get_public_pem / verify_signature)
# ---------------------------------------------------------------------------

def _load_public_key_from_asn1(asn1_pubkey_bytes: bytes):
    """Load the SubjectPublicKeyInfo returned by keySTREAM (DER first, then PEM)."""
    try:
        return serialization.load_der_public_key(asn1_pubkey_bytes, backend=default_backend())
    except Exception:
        return serialization.load_pem_public_key(asn1_pubkey_bytes, backend=default_backend())


def _save_pem(public_key, path: Path) -> None:
    """Write the signer public key to a PEM file (matches TPDS get_public_pem)."""
    pem = public_key.public_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PublicFormat.SubjectPublicKeyInfo,
    )
    path.write_bytes(pem)


def _save_pubkey_h(raw_pub_key: bytes, path: Path) -> None:
    """
    Write the 64-byte raw P-256 signer public key as a C header file.

    Boards where the ATECC608 is only accessible from the MCU (e.g. EV89U05A
    with PKoB4) cannot be provisioned via USB HID from the host.  This header
    provides an alternative: include signer_pubkey.h in the bootloader and use
    the SIGNER_PUBLIC_KEY array directly instead of reading it from a slot.
    """
    guard = "SIGNER_PUBKEY_H"
    lines = [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "/* Auto-generated by mchp_non_tpds_fota.py — do not edit manually. */",
        "/* 64-byte raw P-256 signer public key (X || Y, big-endian). */",
        "",
        "#include <stdint.h>",
        "",
        "static const uint8_t SIGNER_PUBLIC_KEY[64] = {",
    ]
    hex_bytes = [f"0x{b:02X}" for b in raw_pub_key]
    for i in range(0, 64, 16):
        chunk = ", ".join(hex_bytes[i:i + 16])
        suffix = "," if i + 16 < 64 else ""
        lines.append(f"    {chunk}{suffix}")
    lines += ["};", "", f"#endif /* {guard} */", ""]
    path.write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# ATECC608 provisioning (mirrors TPDS FoTA.proto_provision atcab_write_pubkey)
# ---------------------------------------------------------------------------

def provision_signer_pubkey(raw_pub_key: bytes, slot: int = 14, i2c_address: int = 0x70) -> None:
    """
    Write the 64-byte raw P-256 signer public key into an ATECC608 slot.

    Mirrors TPDS FoTA.proto_provision() (atcab_write_pubkey), except the target
    slot defaults to 14 (this application's Signing-Key-Services slot) instead
    of TPDS's slot 15.

    Address auto-discovery: tries the configured i2c_address first, then falls
    back through a list of known ATECC608 kit addresses until one responds.
    The kit (CryptoAuth Trust Platform or equivalent) must be connected via USB.
    """
    try:
        import cryptoauthlib as cal
    except ImportError as exc:
        raise RuntimeError(
            "cryptoauthlib is not installed — cannot provision the ATECC608.\n"
            "Install it with:  pip install cryptoauthlib"
        ) from exc

    if len(raw_pub_key) != 64:
        raise ValueError(f"Public key must be 64 raw bytes, got {len(raw_pub_key)}")

    # Try the configured address first, then common ATECC608 kit addresses.
    # 0xC0 = 8-bit write addr for default ATECC608A/B (7-bit 0x60)
    # 0x70 = TPDS kit default
    # 0x6C / 0xB0 / 0x58 = other common variants
    _CANDIDATE_ADDRS = [i2c_address] + [
        a for a in (0xC0, 0x70, 0x6C, 0xB0, 0x58) if a != i2c_address
    ]

    last_status = None
    for addr in _CANDIDATE_ADDRS:
        cfg = cal.cfg_ateccx08a_kithid_default()
        cfg.cfg.atcahid.dev_interface = int(cal.ATCAKitType.ATCA_KIT_I2C_IFACE)
        cfg.cfg.atcahid.dev_identity = addr
        cfg.devtype = int(cal.ATCADeviceType.ATECC608)
        status = cal.atcab_init(cfg)
        if status == cal.Status.ATCA_SUCCESS:
            if addr != i2c_address:
                _log(f"  (ATECC608 found at address 0x{addr:02X} — configured address "
                     f"0x{i2c_address:02X} did not respond)")
            break
        cal.atcab_release()
        last_status = status
    else:
        tried = ", ".join(f"0x{a:02X}" for a in _CANDIDATE_ADDRS)
        raise RuntimeError(
            f"ATECC608 did not respond at any address ({tried}).\n"
            "\n"
            "  This board's ATECC608 is connected to the MCU via I2C only and is\n"
            "  NOT accessible from the host PC over USB HID (PKoB4 does not bridge\n"
            "  the secure-element I2C bus to USB).\n"
            "\n"
            "  To provision slot 14, use one of these alternatives:\n"
            "  A) Connect a CryptoAuth Trust Platform Kit (EV10E69A / ATECC608\n"
            "     USB-HID kit) that has direct USB access to its ATECC608, then\n"
            "     re-run with [provision] enable = true.\n"
            "  B) Use the generated signer_pubkey.h (in signed_out/) — include it\n"
            "     in your bootloader and write the key array to slot 14 from\n"
            "     firmware (atcab_write_pubkey(14, SIGNER_PUBLIC_KEY)).\n"
            "  C) Set [provision] enable = false to skip this step if slot 14\n"
            "     is already provisioned."
        )

    try:
        status = cal.atcab_write_pubkey(slot, raw_pub_key)
        if status != cal.Status.ATCA_SUCCESS:
            raise RuntimeError(
                f"atcab_write_pubkey(slot={slot}) failed: 0x{status:02X}. "
                "Slot must be configured as public-key-capable "
                "(KeyConfig.KeyType=4, PubInfo=1) in the ATECC608 config zone."
            )
        _log(f"Signer public key written into ATECC608 slot {slot}.")
    finally:
        cal.atcab_release()


# ---------------------------------------------------------------------------
# INI-based configuration loader
# ---------------------------------------------------------------------------

def _load_ini_config(ini_path: str) -> dict:
    """
    Load mchp_non_tpds_fota_config.ini and return a cfg dict compatible with phase_a_sign /
    phase_b_flash.  Component hex files are resolved relative to the directory
    that contains the .ini file (.trustplatform\\keystream_fota by default).
    """
    ini = configparser.ConfigParser()
    ini.read(ini_path, encoding="utf-8")

    base_dir = Path(ini_path).resolve().parent

    def _get(section: str, key: str, fallback: str = "") -> str:
        return ini.get(section, key, fallback=fallback).strip()

    pub_uid   = _get("keystream", "Fleet_uid")
    token     = _get("keystream", "keySTREAM_API_Token")
    key_name  = _get("keystream", "keySTREAM_Signingkey")
    ssid      = _get("wifi", "ssid")
    password  = _get("wifi", "password")

    comp_1_file = _get("components", "comp_1_file")
    comp_1_info = _get("components", "comp_1_info")
    comp_2_file = _get("components", "comp_2_file")
    comp_2_info = _get("components", "comp_2_info")

    # Resolve component paths from the script / ini directory
    comp_1 = str(base_dir / comp_1_file) if comp_1_file else ""
    comp_2 = str(base_dir / comp_2_file) if comp_2_file else ""

    output_dir_rel = _get("output", "output_dir", fallback="signed_out")
    output_dir = str(base_dir / output_dir_rel)

    # ATECC608 provisioning (writes the signer public key into a slot). TPDS uses
    # slot 15; this application uses slot 14.
    provision_enable = _get("provision", "enable", fallback="false").lower() in (
        "1", "true", "yes", "on"
    )
    provision_slot_str = _get("provision", "slot", fallback="14") or "14"
    provision_i2c_str = _get("provision", "i2c_address", fallback="0x70") or "0x70"

    for field, value in [
        ("Fleet_uid", pub_uid),
        ("keySTREAM_API_Token", token),
        ("keySTREAM_Signingkey", key_name),
        ("comp_1_file", comp_1_file),
        ("comp_1_info", comp_1_info),
    ]:
        if not value:
            raise ValueError(f"mchp_non_tpds_fota_config.ini: missing required field [{field}]")

    return {
        "pub_uid":              pub_uid,
        "keystream_auth_token": token,
        "key_name":             key_name,
        "ssid":                 ssid,
        "password":             password,
        "comp_1":               comp_1,
        "comp_1_info":          comp_1_info,
        "comp_2":               comp_2,
        "comp_2_info":          comp_2_info,
        "output_dir":           output_dir,
        "provision_enable":     provision_enable,
        "provision_slot":       int(provision_slot_str, 0),
        "provision_i2c":        int(provision_i2c_str, 0),
    }


# ---------------------------------------------------------------------------
# App_Config.h generator
# ---------------------------------------------------------------------------

def _app_config_h_path(base_dir: Path) -> Path:
    """
    Return the primary path of App_Config.h: SG41_X/App_Config.h.

    configuration.h includes '../../../../../App_Config.h' (5 levels up from
    keystream_connect/keystream_fota/src/config/default/), which resolves to
    the workspace root (SG41_X/).  This is the file the XC32 compiler looks for.
    """
    return base_dir / "App_Config.h"


def _write_App_Config_h(out_dir: Path, ssid: str, pwd: str, pub_uid: str) -> None:
    """Write App_Config.h with the supplied WiFi credentials and device UID."""
    out_dir.mkdir(parents=True, exist_ok=True)
    path = out_dir / "App_Config.h"
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n#ifndef _App_Config_H\n")
        f.write("\n#define _App_Config_H\n")
        f.write("\n#ifdef __cplusplus\n")
        f.write('extern "C" {\n')
        f.write("#endif\n\n")
        f.write('/** @brief Wifi SSID for TrustManaged device */\n')
        f.write(f'#define WIFI_SSID\t\t"{ssid}"\n\n')
        f.write('/** @brief Wifi password for TrustManaged device */\n')
        f.write(f'#define WIFI_PWD\t\t"{repr(pwd)[1:-1]}"\n\n')
        f.write('/** @brief TrustManaged Device Public UID */\n')
        f.write(f'#define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID\t"{pub_uid}"\n\n')
        f.write("#ifdef __cplusplus\n}\n")
        f.write("#endif\n")
        f.write("\n#endif // _App_Config_H\n")
    _log(f"Wrote {out_dir}")


def _app_config_h_is_current(path: Path, ssid: str, pwd: str, pub_uid: str) -> bool:
    """Return True if the existing App_Config.h already contains the expected values."""
    if not path.is_file():
        return False
    try:
        content = path.read_text(encoding="utf-8")
        return (
            f'#define WIFI_SSID\t\t"{ssid}"' in content
            and f'#define KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID\t"{pub_uid}"' in content
        )
    except Exception:
        return False


# ---------------------------------------------------------------------------
# Configuration validation
# ---------------------------------------------------------------------------

# Known placeholder / unconfigured values (case-insensitive).
_PLACEHOLDER_VALUES: frozenset[str] = frozenset({
    "", "xxxx", "xyz", "abc",
    "wifi-ssid", "wifi-pswd", "wifi_ssid", "wifi_pswd",
    "ssid", "password", "my_ssid", "my_password",
    "xxxxxxxxxx", "xxxxxxxxxxx",
})


def _is_placeholder(value: str) -> bool:
    """Return True if *value* looks like an unconfigured placeholder."""
    stripped = (value or "").strip()
    if not stripped:
        return True
    if stripped.lower() in _PLACEHOLDER_VALUES:
        return True
    # All identical characters, length ≥ 3 → e.g. "XXXXX", "aaaaa"
    if len(stripped) >= 3 and len(set(stripped.upper())) == 1:
        return True
    return False


def _validate_cfg_values(cfg: dict) -> None:
    """
    Verify that all mandatory configuration fields contain real values.
    Raises ValueError with a clear message when any field is empty or
    still holds a placeholder such as 'XXXX', 'XYZ', 'WiFi-SSID', etc.
    """
    checks = [
        ("[keystream] Fleet_uid",             cfg["pub_uid"]),
        ("[keystream] keySTREAM_API_Token",   cfg["keystream_auth_token"]),
        ("[keystream] keySTREAM_Signingkey",  cfg["key_name"]),
        ("[wifi] ssid",                       cfg.get("ssid", "")),
        ("[wifi] password",                   cfg.get("password", "")),
    ]
    errors = [
        f"  • {label} = '{value}'"
        for label, value in checks
        if _is_placeholder(value)
    ]
    if errors:
        raise ValueError(
            "Invalid or placeholder configuration values — signing aborted.\n"
            "Update mchp_non_tpds_fota_config.ini with valid values for:\n"
            + "\n".join(errors)
        )


# ---------------------------------------------------------------------------
# Recompilation check
# ---------------------------------------------------------------------------

def _check_stale_hexes(app_config_path: Path, comp_1_hex: str, comp_2_hex: str) -> list[str]:
    """
    Return a list of HEX file paths that must be recompiled, i.e. paths where:
      - the file does not exist, OR
      - the file was built before App_Config.h was last modified.

    Returns an empty list when App_Config.h itself is missing (nothing to compare).
    """
    if not app_config_path.is_file():
        return []
    cfg_mtime = app_config_path.stat().st_mtime
    stale: list[str] = []
    for hex_path in filter(None, [comp_1_hex, comp_2_hex]):
        p = Path(hex_path)
        if not p.is_file():
            stale.append(f"{hex_path}  (file not found)")
        elif p.stat().st_mtime < cfg_mtime:
            stale.append(f"{hex_path}  (older than App_Config.h)")
    return stale


# ---------------------------------------------------------------------------
# Signing freshness check
# ---------------------------------------------------------------------------

def _signing_is_current(output_dir: str, comp_1_hex: str, comp_2_hex: str) -> bool:
    """
    Return True when the signed outputs in *output_dir* are already newer than
    all input HEX files — meaning no changes have been made to the binaries
    since the last successful signing run.

    For a 2-component build the presence and mtime of combined_component.hex is
    used as the signing timestamp.  For a 1-component build the newest *.hex
    found in output_dir (excluding combined_component.hex) is used.
    """
    out = Path(output_dir)
    input_hexes = [p for p in filter(None, [comp_1_hex, comp_2_hex]) if Path(p).is_file()]
    if not input_hexes:
        return False  # cannot compare — inputs missing

    latest_input_mtime = max(Path(p).stat().st_mtime for p in input_hexes)

    if comp_2_hex:
        # 2-component build: combined_component.hex is the signing artefact.
        combined = out / "combined_component.hex"
        return combined.is_file() and combined.stat().st_mtime > latest_input_mtime
    else:
        # 1-component build: any signed *.hex newer than the input is sufficient.
        signed = [h for h in out.glob("*.hex") if h.name != "combined_component.hex"]
        if not signed:
            return False
        return max(h.stat().st_mtime for h in signed) > latest_input_mtime


# ---------------------------------------------------------------------------
# Interactive YES / NO flash prompt
# ---------------------------------------------------------------------------

def _prompt_flash_yn() -> bool:
    """Ask the user whether to flash the signed image.  Returns True for YES."""
    print("")
    print("=" * 70)
    print("Proceed with flashing the firmware image?")
    print("  [1] YES — program the board now")
    print("  [2] NO  — exit (signed files are in signed_out/)")
    print("=" * 70)
    while True:
        choice = input("Enter choice (1 / 2): ").strip()
        if choice in ("1", "yes", "y", "YES", "Y"):
            return True
        if choice in ("2", "no", "n", "NO", "N"):
            return False
        print("Please enter 1 (YES) or 2 (NO).")


# ---------------------------------------------------------------------------
# Full ini-driven workflow: validate → App_Config.h → recompile check →
#                           sign → optional flash
# ---------------------------------------------------------------------------

def run_from_ini(ini_path: str) -> None:
    """
    Complete workflow driven by mchp_non_tpds_fota_config.ini:

      Step 1 — Configuration & App_Config.h
        a. Load configuration from the .ini file.
        b. Validate that all mandatory fields hold real (non-placeholder) values.
        c. Generate App_Config.h at keystream_connect/App_Config.h (if new or
           values have changed since the last run).
        d. Abort with clear instructions if either HEX binary is missing or was
           compiled before the latest App_Config.h update (recompilation needed).

      Step 2 — Signing (Phase A)
        Sign both components offline; patches rawSignature into each hex and
        writes <name>_<ver>.hex / .bin plus combined_component.hex / .bin.
        Also fetches the signer public key (saved as <key>.pem). Signature
        verification is performed on-device by the bootloader (slot 14), not
        on the host.

      Step 3 — ATECC608 provisioning (optional, [provision] enable = true)
        Write the signer public key into ATECC608 slot 14 (TPDS uses slot 15)
        so the bootloader can verify FOTA images.

      Step 4 — Flashing (Phase B, optional)
        Ask the user whether to flash the signed combined image.
        YES → program via ipecmd; NO → exit (signed files remain in signed_out/).
    """
    _log(f"Loading configuration from: {ini_path}")
    cfg = _load_ini_config(ini_path)
    base_dir = Path(ini_path).resolve().parent

    # ---- Step 1b: Validate configuration values ----
    _log("=" * 70)
    _log("Validating configuration values ...")
    _validate_cfg_values(cfg)
    _log("Configuration OK.")

    # ---- Step 1c: Generate App_Config.h (only if missing or values changed) ----
    app_config_path = _app_config_h_path(base_dir)
    _log("=" * 70)
    if _app_config_h_is_current(app_config_path, cfg["ssid"], cfg["password"], cfg["pub_uid"]):
        _log(f"App_Config.h is up-to-date: {app_config_path}")
        app_config_updated = False
    else:
        _log("Generating App_Config.h ...")
        # Write to SG41_X/ — satisfies configuration.h (#include "../../../../../App_Config.h",
        # which is 5 levels up from keystream_connect/keystream_fota/src/config/default/).
        _write_App_Config_h(app_config_path.parent, cfg["ssid"], cfg["password"], cfg["pub_uid"])
        # Write to keystream_connect/ — satisfies ktaConfig.h
        # (#include "../../../../../../../../App_Config.h",
        # which is 8 levels up from ...default/library/kta_lib/SOURCE/include/).
        _write_App_Config_h(
            app_config_path.parent / "keystream_connect",
            cfg["ssid"], cfg["password"], cfg["pub_uid"],
        )
        app_config_updated = True

    # ---- Step 1d: Check whether recompilation is required ----
    _log("=" * 70)
    stale_hexes = _check_stale_hexes(app_config_path, cfg.get("comp_1", ""), cfg.get("comp_2", ""))
    if stale_hexes:
        _log("ERROR: The following HEX files are out-of-date with App_Config.h:")
        for entry in stale_hexes:
            _log(f"  • {entry}")
        _log("")
        _log("Recompilation is required before signing.")
        _log("Please rebuild the affected project(s) in MPLAB X, then re-run this script:")
        # Map each hex filename to its MPLAB X project name.
        _HEX_TO_PROJECT = {
            "keystream_boot.X.production.hex": "keystream_boot.X",
            "keystream_fota.X.production.hex": "keystream_Fota.X",
            "keystream_Fota.X.production.hex": "keystream_Fota.X",
        }
        projects_to_rebuild: list[str] = []
        for entry in stale_hexes:
            hex_name = Path(entry.split("  ")[0]).name
            project = _HEX_TO_PROJECT.get(hex_name, hex_name)
            if project not in projects_to_rebuild:
                projects_to_rebuild.append(project)
        for idx, project in enumerate(projects_to_rebuild, start=1):
            _log(f"  {idx}. Open {project} and perform Clean and Build.")
        sys.exit(1)

    if app_config_updated:
        _log("App_Config.h was regenerated — HEX files are confirmed up-to-date.")
    else:
        _log("HEX files are up-to-date — recompilation not required.")

    # ---- Step 2: Phase A — sign (skipped when signed outputs are already current) ----
    _log("=" * 70)
    out_dir = Path(cfg["output_dir"])
    if _signing_is_current(cfg["output_dir"], cfg.get("comp_1", ""), cfg.get("comp_2", "")):
        _log("Signed outputs are up-to-date — skipping re-signing.")
        signed_files = sorted(
            str(h) for h in out_dir.glob("*.hex") if h.name != "combined_component.hex"
        )
        combined_path = out_dir / "combined_component.hex"
        result = {
            "signed_files": signed_files,
            "combined_hex": str(combined_path) if combined_path.is_file() else None,
            "output_dir": str(out_dir),
        }
    else:
        result = phase_a_sign(cfg)
        _log("")
        _log("=" * 70)
        _log("Signing complete.")
        _log("=" * 70)

    # ---- Step 3: Provision ATECC608 slot with signer public key (mirrors TPDS) ----
    if cfg.get("provision_enable"):
        _log("=" * 70)
        _log(f"Provisioning ATECC608 slot {cfg['provision_slot']} with signer public key ...")
        signer_pub_key = result.get("signer_pub_key")
        if not signer_pub_key:
            # Signing was skipped (outputs up-to-date) — fetch the key directly.
            _log("Fetching signer public key from keySTREAM ...")
            ks = KeyStream(cfg["keystream_auth_token"], cfg["pub_uid"])
            signer_pub_key = ks.get_trusted_pub_key(cfg["key_name"])["rawPublicKey"]
        provision_signer_pubkey(signer_pub_key, cfg["provision_slot"], cfg["provision_i2c"])
        _log("Provisioning complete.")

    # ---- Step 4: Prompt — flash? ----
    if _prompt_flash_yn():
        combined = result.get("combined_hex")
        if not combined:
            if len(result["signed_files"]) != 1:
                raise RuntimeError("Cannot determine image to flash — no combined hex produced.")
            combined = result["signed_files"][0]
        phase_b_flash(combined, cfg)
        _log("")
        _log("=" * 70)
        _log("Flashing complete.")
        _log("=" * 70)
    else:
        _log("")
        _log("Flashing skipped.  Signed files are in: " + cfg["output_dir"])


def _parse_addr(value) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 16)


def _read_component_info(ih: IntelHex, info_addr: int):
    """Unpack the 52-byte component-info struct embedded in the hex."""
    raw = ih.gets(info_addr, INFO_STRUCT_LEN)
    _, _, name, version, addr, size, sign_addr = struct.unpack(INFO_STRUCT_FMT, raw)
    name_str = name.decode("utf-8").rstrip("\x00")
    version_str = version.decode("utf-8").rstrip("\x00")
    return name_str, version_str, addr, size, sign_addr


# ----------------------------------------------------------------------------
# PHASE A — Offline signing
# ----------------------------------------------------------------------------

def phase_a_sign(cfg: dict, output_dir: str | None = None) -> dict:
    """
    Implements Phase A of Method 1.

    Steps (per drawio):
        [1] Read input hex locally; data = ih.tobinarray(addr, size); digest = SHA256(data).
        [2] POST /sign {digest:[32 ints]}                  <-- only KS call
        [3] receive rawSignature (64B) + asn1Signature (DER)
            (NO host-side verify, NO GET /trustkeypairs)
        [4] ih[sign_addr : sign_addr+64] = rawSignature
            write <name>_<ver>.hex / .bin
            write combined_component.hex / .bin
    """
    out_dir = Path(output_dir or cfg.get("output_dir") or os.getcwd()).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    _log("=" * 70)
    _log("PHASE A — Offline signing on host (NO pubkey fetch, NO local verify)")
    _log("=" * 70)
    _log(f"Output dir : {out_dir}")

    _log("Connecting to keySTREAM ...")
    ks = KeyStream(cfg["keystream_auth_token"], cfg["pub_uid"])
    _log("Connected.")

    # Fetch the signer public key (mirrors TPDS get_trusted_pub_key) so each
    # component's signature can be verified on the host before it is emitted.
    _log("Fetching signer public key ...")
    pub_resp = ks.get_trusted_pub_key(cfg["key_name"])
    signer_pub_key = pub_resp["rawPublicKey"]              # 64 B raw P-256, for slot provisioning
    public_key = _load_public_key_from_asn1(pub_resp["asn1PublicKey"])
    pem_path = out_dir / f"{cfg['key_name']}.pem"
    _save_pem(public_key, pem_path)
    _log(f"Signer public key saved: {pem_path}")
    h_path = out_dir / "signer_pubkey.h"
    _save_pubkey_h(signer_pub_key, h_path)
    _log(f"Signer public key header: {h_path}")

    components = []
    if cfg.get("comp_1"):
        components.append(("comp_1", cfg["comp_1"], _parse_addr(cfg["comp_1_info"])))
    if cfg.get("comp_2"):
        if not cfg.get("comp_2_info"):
            raise ValueError("comp_2 specified but comp_2_info is missing")
        components.append(("comp_2", cfg["comp_2"], _parse_addr(cfg["comp_2_info"])))

    combined_ih = IntelHex()
    signed_files: list[str] = []

    for label, hex_path, info_addr in components:
        hex_path = os.path.abspath(hex_path)
        if not os.path.exists(hex_path):
            _log(f"FileNotFoundError: No such file or directory: '{hex_path}'")
            _log("Compile the (BootLoader + KTA)Applications to execute further steps")
            _log("-" * 70)
            sys.exit(1)

        _log("-" * 70)
        _log(f"[{label}] Loading hex: {hex_path}")
        ih_comp = IntelHex(source=hex_path)
        ih_comp.padding = 0xFF

        name, version, addr, size, sign_addr = _read_component_info(ih_comp, info_addr)
        _log(f"[{label}] name={name}  version={version}")
        _log(f"[{label}] data_addr=0x{addr:08X}  size=0x{size:X}  sign_addr=0x{sign_addr:08X}")

        # [1] data + digest, computed entirely on host
        data = bytes(ih_comp.tobinarray(addr, size=size))
        digest = ks.caluclate_digest(data)
        _log(f"[{label}] SHA256 digest: {digest.hex().upper()}")

        # [2] POST /sign  (the ONLY keySTREAM call)
        _log(f"[{label}] POST /sign ...")
        sig_resp = ks.sign(cfg["key_name"], digest)
        raw_sig = sig_resp["rawSignature"]              # 64 B (r||s)
        if len(raw_sig) != 64:
            raise ValueError(f"[{label}] Unexpected rawSignature length: {len(raw_sig)}")
        _log(f"[{label}] rawSignature: {raw_sig.hex().upper()}")
        # NOTE: signature verification is performed on-device by the bootloader,
        # using the signer public key provisioned into ATECC608 slot 14.

        # [4] patch + write per-component artifacts
        ih_comp[sign_addr:sign_addr + 64] = list(raw_sig)
        file_stem = f"{name}_{version}"
        out_hex = out_dir / f"{file_stem}.hex"
        out_bin = out_dir / f"{file_stem}.bin"
        ih_comp.write_hex_file(str(out_hex), byte_count=32)
        ih_comp.tobinfile(str(out_bin))
        signed_files.append(str(out_hex))
        _log(f"[{label}] Wrote: {out_hex}")
        _log(f"[{label}] Wrote: {out_bin}")

        combined_ih.merge(ih_comp, overlap="replace")

    combined_hex_path = None
    if len(components) == 2:
        _log("-" * 70)
        _log("Generating combined component (BANK2-shifted) ...")
        combined_hex_path = out_dir / "combined_component.hex"
        combined_bin_path = out_dir / "combined_component.bin"
        # Same offset pattern as FoTA.generate_resources()
        combined_ih.frombytes(bytes=bytes(combined_ih.tobinarray()), offset=0x00080000)
        combined_ih.write_hex_file(str(combined_hex_path), byte_count=32)
        combined_ih.tobinfile(str(combined_bin_path))
        _log(f"Wrote: {combined_hex_path}")
        _log(f"Wrote: {combined_bin_path}")

    _log("Phase A complete.")
    return {
        "signed_files": signed_files,
        "combined_hex": str(combined_hex_path) if combined_hex_path else None,
        "output_dir": str(out_dir),
        "signer_pub_key": signer_pub_key,
    }


# ----------------------------------------------------------------------------
# PHASE B — Flash (no ATECC608 access from host)
# ----------------------------------------------------------------------------

import glob
import shutil

# Roots to search for MPLAB X installations (all versions / both arches).
_MPLABX_ROOTS = [
    r"C:\Program Files\Microchip\MPLABX",
    r"C:\Program Files (x86)\Microchip\MPLABX",
]


def _find_ipecmd() -> str:
    """Locate ipecmd.exe across any installed MPLAB X version (recursive search)."""
    # 1) PATH first
    on_path = shutil.which("ipecmd.exe") or shutil.which("ipecmd")
    if on_path:
        return on_path

    # 2) Recursive glob — handles all known layouts:
    #    v*\mplab_platform\bin\ipecmd.exe
    #    v*\mplab_platform\mplab_ipe\ipecmd.exe        <-- v6.20+
    #    v*\mplab_ipe\ipecmd.exe                       <-- older
    candidates: list[str] = []
    for root in _MPLABX_ROOTS:
        if os.path.isdir(root):
            candidates += glob.glob(os.path.join(root, "**", "ipecmd.exe"), recursive=True)

    if candidates:
        # Sort by version folder name so the newest MPLAB X wins.
        candidates.sort()
        return candidates[-1]

    raise FileNotFoundError(
        "ipecmd.exe not found.\n"
        "Either install MPLAB X (https://www.microchip.com/mplab/mplab-x-ide),\n"
        "add ipecmd to your PATH, or set 'ipecmd' in your config YAML."
    )


def _find_ipecmd_jar_and_java() -> tuple[str | None, str | None]:
    """
    Locate ipecmd.jar + bundled java.exe — same layout TPDS uses
    (see tpds.flash_program.FlashProgram.__flash_micro).

    Returns (jar_path, java_path) or (None, None) if not found.
    """
    jars: list[str] = []
    javas: list[str] = []
    for root in _MPLABX_ROOTS:
        if not os.path.isdir(root):
            continue
        jars  += glob.glob(os.path.join(root, "**", "ipecmd.jar"), recursive=True)
        javas += glob.glob(os.path.join(root, "**", "bin", "java.exe"), recursive=True)
    jar = max(jars,  default=None)   # newest MPLAB X version wins (lexicographic)
    jav = max(javas, default=None)
    return jar, jav


# ---------------------------------------------------------------------------
# Board metadata lookup — mirrors FoTA.load_image() which does:
#     FlashProgram(board, get_details(board)).load_hex_image_with_ipe(image)
# Reads backend/boards/<board>.yaml to pick up mcu_part_number / program_tool
# ---------------------------------------------------------------------------

_BOARDS_DIR_CANDIDATES = [
    Path(__file__).resolve().parent / "backend"   / "boards",
    Path(__file__).resolve().parent / "backend__" / "boards",
]


# Built-in fallback catalog (used when a board has no YAML in backend*/boards/).
# Keys are the board names you'd see on the silkscreen / USB sticker.
BUILTIN_BOARD_CATALOG = {
    # Source: TPDS site-packages keystream_connect/backend/boards/EV10E69A.yaml
    # CryptoAuth Trust Platform (older, nEDBG)
    "EV10E69A": {"mcu": "ATSAMD21E18A",     "programmer": "nEDBG"},
    # CryptoAuth Pro Trust Platform — PIC32CX SG41 + on-board PKoB 4.
    # Source of truth: tpds_extension/keystream_connect/backend/boards/EV10E69A.yaml
    # in the installed TPDS package (`mcu_part_number: 32CX1025SG41100`,
    # `program_tool: PKoB4`).
    "EV89U05A": {"mcu": "32CX1025SG41100",   "programmer": "PKoB4"},
}


# Reverse map: ipecmd -TP tag → most likely Microchip dev kit.
# Used so the UI does not need to expose Board / MCU / Programmer fields —
# the backend resolves them from USB.
PROGRAMMER_TO_DEFAULT_BOARD = {
    "nEDBG": "EV10E69A",
    "PKOB4": "EV89U05A",
}


def _get_board_details(board_name: str) -> dict:
    """Return {mcu, programmer} for the given board.

    Looks up board YAML files first (mirrors FoTA.get_details), then falls
    back to BUILTIN_BOARD_CATALOG.
    """
    if not board_name:
        return {}

    # 1) YAML files under backend*/boards/
    for boards_dir in _BOARDS_DIR_CANDIDATES:
        if not boards_dir.is_dir():
            continue
        for yml in boards_dir.glob("*.yaml"):
            try:
                data = yaml.safe_load(yml.read_text(encoding="utf-8")) or {}
            except Exception:
                continue
            boards = (data.get("boards") or {})
            if board_name in boards:
                b = boards[board_name]
                return {
                    "mcu":        b.get("mcu_part_number"),
                    "programmer": b.get("program_tool"),
                    "board_file": str(yml),
                    "source":     "yaml",
                }

    # 2) Built-in catalog
    if board_name in BUILTIN_BOARD_CATALOG:
        b = BUILTIN_BOARD_CATALOG[board_name]
        return {
            "mcu":        b["mcu"],
            "programmer": b["programmer"],
            "board_file": "(built-in catalog)",
            "source":     "builtin",
        }
    return {}


# ---------------------------------------------------------------------------
# Hardware (USB) detection — no external pip dependencies, uses PowerShell.
# Maps VID/PID → ipecmd "-TP" tool tag. Board name itself is NOT broadcast
# on USB by Microchip kits; only the programmer family is.
# ---------------------------------------------------------------------------
# ipecmd '-TP' tool tags:
#   nEDBG        : nano Embedded Debugger (older SAM Xplained / Trust kits)
#   PKOB4        : MPLAB PKoB 4 (PICkit On-Board 4) — newer kits like EV89U05A
#   PICKIT4      : standalone MPLAB PICkit 4
#   SNAP         : MPLAB Snap
USB_PROGRAMMER_MAP = {
    # (vid, pid)        : (ipecmd_tag, friendly)
    ("03EB", "2110"): ("nEDBG",   "nEDBG (Atmel Embedded Debugger)"),
    ("03EB", "2111"): ("nEDBG",   "nEDBG (CMSIS-DAP)"),
    ("03EB", "2175"): ("nEDBG",   "nEDBG (HID kit)"),
    ("03EB", "2177"): ("nEDBG",   "nEDBG"),
    ("04D8", "810B"): ("PKOB4",   "MPLAB PKoB 4 (PICkit On-Board 4)"),
    ("04D8", "9012"): ("PICKIT4", "MPLAB PICkit 4"),
    ("04D8", "9018"): ("SNAP",    "MPLAB Snap"),
}


def _ps_query_microchip_usb() -> list[dict]:
    """Query Windows for connected Microchip / Atmel USB devices."""
    if sys.platform != "win32":
        return []
    ps = (
        "Get-PnpDevice -PresentOnly | "
        "Where-Object { $_.InstanceId -match 'VID_(03EB|04D8)' } | "
        "ForEach-Object { "
        "  $bus = (Get-PnpDeviceProperty -InstanceId $_.InstanceId "
        "          -KeyName 'DEVPKEY_Device_BusReportedDeviceDesc' "
        "          -ErrorAction SilentlyContinue).Data; "
        "  [PSCustomObject]@{ FriendlyName=$_.FriendlyName; BusDesc=$bus; "
        "                     InstanceId=$_.InstanceId } "
        "} | ConvertTo-Json -Compress"
    )
    try:
        proc = subprocess.run(
            ["powershell", "-NoProfile", "-Command", ps],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=15,
        )
    except Exception:
        return []
    out = (proc.stdout or "").strip()
    if not out:
        return []
    import json, re
    try:
        data = json.loads(out)
    except Exception:
        return []
    if isinstance(data, dict):
        data = [data]

    results: list[dict] = []
    for entry in data:
        inst = (entry.get("InstanceId") or "").upper()
        m = re.search(r"VID_([0-9A-F]{4})&PID_([0-9A-F]{4})", inst)
        if not m:
            continue
        vid, pid = m.group(1), m.group(2)
        results.append({
            "vid": vid,
            "pid": pid,
            "friendly": entry.get("FriendlyName") or "",
            "bus_desc": entry.get("BusDesc") or "",
            "instance": inst,
        })
    return results


def detect_connected_programmer() -> dict:
    """
    Return info about the connected Microchip programmer (if any).

    Returns dict with keys: programmer (ipecmd -TP tag), friendly, vid, pid.
    Empty dict when nothing is detected.
    """
    devices = _ps_query_microchip_usb()
    seen_keys: set[tuple[str, str]] = set()
    for d in devices:
        key = (d["vid"], d["pid"])
        if key in seen_keys:
            continue
        seen_keys.add(key)
        if key in USB_PROGRAMMER_MAP:
            tag, friendly = USB_PROGRAMMER_MAP[key]
            return {
                "programmer": tag,
                "friendly":   friendly,
                "vid":        d["vid"],
                "pid":        d["pid"],
                "bus_desc":   d["bus_desc"],
            }
    # Fallback: just return the first Microchip device we saw, with no -TP tag.
    if devices:
        d = devices[0]
        return {
            "programmer": "",
            "friendly":   d["bus_desc"] or d["friendly"] or "Unknown Microchip device",
            "vid":        d["vid"],
            "pid":        d["pid"],
            "bus_desc":   d["bus_desc"],
        }
    return {}


def check_board_status(cfg: dict) -> dict:
    """
    Standalone analog of `FoTA.check_board_status()` from
    backend__/usecases/proto_provision/helper/helper.py.

    Differences vs the TPDS version:
      * No dependency on tpds.flash_program / tpds.api — pure Python.
      * Uses USB enumeration (Get-PnpDevice) instead of FlashProgram.is_board_connected().
      * Uses YAML-or-built-in catalog instead of get_details(board).
      * Optionally programs a factory hex (cfg['factory_hex']) if provided.

    Returns a fully-resolved dict:
        {
            'board':        <board name>,
            'mcu':          <MCU part>,
            'programmer':   <ipecmd -TP tag>,
            'detected':     <USB info dict or {}>,
            'board_file':   <yaml path or '(built-in)'>,
            'factory_hex':  <abs path or None>,
        }

    Raises with an actionable message when something is missing.
    """
    _log("Checking Board Status ...")

    # 1) physical kit-parser check (USB enumeration of Microchip programmers)
    detected = detect_connected_programmer()
    if detected:
        _log(
            f"Detected USB tool  : {detected['friendly']} "
            f"(VID={detected['vid']} PID={detected['pid']})"
        )
    else:
        _log("Detected USB tool  : <none>")

    assert detected, (
        "Check the Kit parser board connections — no Microchip programmer "
        "(nEDBG / PKoB 4 / PICkit / Snap) is connected via USB."
    )

    # 2) board name: explicit cfg > derived from detected programmer
    board_name = (cfg.get("board") or "").strip()
    if not board_name:
        board_name = PROGRAMMER_TO_DEFAULT_BOARD.get(detected.get("programmer", ""), "")
        if board_name:
            _log(f"Board (auto)       : {board_name}  (from {detected['programmer']})")
    else:
        _log(f"Board              : {board_name}")
    assert board_name, (
        "Could not determine the board automatically.\n"
        f"Detected programmer '{detected.get('programmer')}' has no default board mapping. "
        "Add an entry to PROGRAMMER_TO_DEFAULT_BOARD or set 'board' in the config."
    )

    # 3) board details: YAML first, then built-in catalog
    details = _get_board_details(board_name)
    if details:
        _log(f"Board YAML/catalog : {details.get('board_file')}")
        _log(f"  -> MCU            : {details.get('mcu')}")
        _log(f"  -> Programmer     : {details.get('programmer')}")
    else:
        _log(f"(No board entry for '{board_name}' in YAML or built-in catalog.)")

    # 4) resolve final values: explicit cfg > YAML/catalog > USB-detected
    mcu = (cfg.get("mcu") or details.get("mcu") or "").strip()
    pgm = (cfg.get("programmer") or details.get("programmer") or "").strip()
    if not pgm and detected.get("programmer"):
        pgm = detected["programmer"]

    assert mcu, (
        f"MCU part number is not set for board '{board_name}'.\n"
        "Add the board to BUILTIN_BOARD_CATALOG or backend/boards/<name>.yaml."
    )
    assert pgm, "Programmer (-TP tag) could not be determined."

    # 5) optional factory-hex programming, mirroring FoTA.check_board_status()
    factory_hex = cfg.get("factory_hex") or _find_factory_hex(board_name)
    if factory_hex:
        _log(f"Factory hex        : {factory_hex}")
    else:
        _log("Factory hex        : <not configured — skipping factory program step>")

    _log("Board Status OK")
    return {
        "board":       board_name,
        "mcu":         mcu,
        "programmer":  pgm,
        "detected":    detected,
        "board_file":  details.get("board_file") if details else None,
        "factory_hex": factory_hex,
    }


def _find_factory_hex(board_name: str) -> str | None:
    """Look for backend*/boards/<board>/<board>.hex (matches FoTA layout)."""
    for boards_dir in _BOARDS_DIR_CANDIDATES:
        candidate = boards_dir / board_name / f"{board_name}.hex"
        if candidate.is_file():
            return str(candidate.resolve())
    return None


def _normalize_mcu_for_ipecmd(mcu: str) -> str:
    """ipecmd auto-prefixes 'PIC' for PIC32 parts, so strip it if present.
    AT-prefixed SAM parts are passed through unchanged."""
    m = (mcu or "").strip()
    if m.upper().startswith("PIC"):
        return m[3:]
    return m


# Map of MCU family prefix → DFP pack name used by ipecmd's -PInstallPack option.
# Used to auto-install missing Device Family Packs (exit code 36).
_DFP_BY_MCU_PREFIX = [
    # (mcu prefix lowercase, dfp pack id)
    ("32cx1025sg", "Microchip.PIC32CX-SG_DFP"),  # PIC32CX SG41 family (EV89U05A)
    ("32cx",       "Microchip.PIC32CX-SG_DFP"),
    ("32cm51",     "Microchip.PIC32CM-SG_DFP"),
    ("32cm12",     "Microchip.PIC32CM-LE_DFP"),
    ("32cm",       "Microchip.PIC32CM_DFP"),
    ("32mz",       "Microchip.PIC32MZ-EF_DFP"),
    ("32mx",       "Microchip.PIC32MX_DFP"),
    ("samd21",     "Microchip.SAMD21_DFP"),
    ("atsamd21",   "Microchip.SAMD21_DFP"),
    ("samd51",     "Microchip.SAMD51_DFP"),
    ("atsamd51",   "Microchip.SAMD51_DFP"),
    ("saml10",     "Microchip.SAML10_DFP"),
    ("saml11",     "Microchip.SAML11_DFP"),
    ("atsaml1",    "Microchip.SAML1x_DFP"),
]


def _guess_dfp(mcu_arg: str) -> str | None:
    """Best-effort guess of the DFP pack id needed for the given MCU."""
    m = mcu_arg.lower()
    for prefix, dfp in _DFP_BY_MCU_PREFIX:
        if m.startswith(prefix):
            return dfp
    return None


def _install_dfp(ipecmd: str, dfp: str) -> bool:
    """
    Run `ipecmd -PInstallPack=<dfp>` to download/install the missing pack.
    Returns True on success, False on failure.
    """
    cmd = [ipecmd, f"-PInstallPack={dfp}"]
    cwd = os.path.dirname(ipecmd)
    _log(f"Installing missing pack: {dfp}")
    _log(f"Command    : {' '.join(cmd)}")
    proc = subprocess.run(
        cmd, cwd=cwd,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
        timeout=600,
    )
    if proc.stdout:
        for line in proc.stdout.splitlines():
            _log(f"  ipecmd | {line}")
    if proc.returncode == 0:
        _log(f"Installed pack: {dfp}")
        return True
    _log(f"Pack install returned exit code {proc.returncode}")
    return False


def _build_ipe_cmd(ipecmd: str, mcu: str, pgm: str, hex_path: str) -> tuple[list[str], str]:
    """
    Build the ipecmd invocation list, preferring `java -jar ipecmd.jar` (which
    is what `tpds.flash_program.FlashProgram.__flash_micro` uses), and falling
    back to `ipecmd.exe`.

    Returns (cmd_list, cwd).
    """
    mcu_arg = _normalize_mcu_for_ipecmd(mcu)
    args = [
        f"-P{mcu_arg}",
        f"-TP{pgm}",
        "-OL",
        "-OK",
        "-M",
        f"-F{hex_path}",
    ]

    # Prefer java + ipecmd.jar (matches TPDS exactly, avoids .exe wrapper quirks).
    jar, java = _find_ipecmd_jar_and_java()
    if jar and java:
        return [java, "-jar", jar] + args, os.path.dirname(jar)

    # Fallback: ipecmd.exe
    return [ipecmd] + args, os.path.dirname(ipecmd)


def _run_ipecmd(ipecmd: str, mcu: str, pgm: str, hex_path: str,
                _retry_after_pack_install: bool = True) -> None:
    """
    Run ipecmd to program the given hex; stream output via _log.

    Mirrors `tpds.flash_program.FlashProgram.load_hex_image_with_ipe(image)`:
        java -jar ipecmd.jar -P<mcu> -TP<tool> -OL -OK -M -F<hex>

    If ipecmd reports "DFP missing" (exit code 36), this function automatically
    invokes `ipecmd -PInstallPack=<DFP>` and retries the program command once.
    """
    cmd, cwd = _build_ipe_cmd(ipecmd, mcu, pgm, hex_path)
    _log(f"Command    : {' '.join(cmd)}")
    _log(f"Working dir: {cwd}")
    _log("-" * 70)
    proc = subprocess.run(
        cmd, cwd=cwd,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace",
    )
    if proc.stdout:
        for line in proc.stdout.splitlines():
            _log(f"  ipecmd | {line}")

    if proc.returncode == 0:
        return

    out_lower = (proc.stdout or "").lower()
    dfp_missing = (
        proc.returncode == 36
        or "unable to locate dfp" in out_lower
        or "install required pack" in out_lower
        or "could not find device" in out_lower
    )

    if dfp_missing and _retry_after_pack_install:
        mcu_arg = _normalize_mcu_for_ipecmd(mcu)
        dfp = _guess_dfp(mcu_arg)
        if not dfp:
            raise RuntimeError(
                f"ipecmd reports missing DFP for '{mcu_arg}', and no pack mapping "
                "is configured for this part. Open MPLAB X → Tools → Packs and "
                "install the matching Device Family Pack manually."
            )
        _log("-" * 70)
        _log(f"DFP missing for {mcu_arg} — attempting auto-install of {dfp} ...")
        if _install_dfp(ipecmd, dfp):
            _log("-" * 70)
            _log("Retrying program command after pack install ...")
            _run_ipecmd(ipecmd, mcu, pgm, hex_path, _retry_after_pack_install=False)
            return

    hint = ""
    if proc.returncode == 9:
        hint = (
            "\nHINT: exit code 9 usually means ipecmd could not detect / connect "
            "to the programmer.\n"
            "  • Verify the kit is connected via USB.\n"
            "  • Close MPLAB X IDE / MPLAB IPE / Data Visualizer.\n"
            "  • Unplug & replug the kit, then retry."
        )
    elif dfp_missing:
        mcu_arg = _normalize_mcu_for_ipecmd(mcu)
        hint = (
            f"\nHINT: missing Device Family Pack for '{mcu_arg}'.\n"
            "  • Open MPLAB X IDE → Tools → Packs and install the matching DFP.\n"
            f"  • Or run: \"{ipecmd}\" -PInstallPack=<DFP id>"
        )
    raise RuntimeError(f"ipecmd exited with code {proc.returncode}{hint}")


def phase_b_flash(image: str, cfg: dict | None = None) -> None:
    """
    Implements Phase B of Method 1 — mirrors the TPDS pattern:

        check_board_status(board, logger)        # validates kit + factory hex
        FlashProgram(...).load_hex_image_with_ipe(image)

    Host does NOT call atcab_* — Slot 14 is owned by KTA.

    Flash settings can be placed in the config YAML / UI:
        board:        board name (e.g. EV89U05A)
        mcu:          MCU part number       (overrides board lookup)
        programmer:   ipecmd -TP tag        (overrides board lookup)
        ipecmd:       path to ipecmd.exe    (auto-detected if omitted)
        factory_hex:  path to factory hex   (programmed first if present)
    """
    _log("=" * 70)
    _log("PHASE B — Flash signed image (host does NOT touch ATECC608)")
    _log("=" * 70)

    image_abs = os.path.abspath(image)
    if not os.path.exists(image_abs):
        raise FileNotFoundError(f"Signed image not found: {image_abs}")

    cfg = cfg or {}

    # ---- Step 1: check_board_status() analog ----
    status = check_board_status(cfg)

    # ---- Step 2: locate ipecmd ----
    ipecmd = cfg.get("ipecmd") or _find_ipecmd()
    _log(f"ipecmd             : {ipecmd}")
    _log(f"MCU                : {status['mcu']}")
    _log(f"Programmer (-TP)   : {status['programmer']}")
    _log(f"Image              : {image_abs}")

    # ---- Step 3: optionally program factory hex first (mirrors FoTA flow) ----
    if status.get("factory_hex"):
        _log("-" * 70)
        _log(f"Programming factory hex: {status['factory_hex']}")
        _run_ipecmd(ipecmd, status["mcu"], status["programmer"], status["factory_hex"])

    # ---- Step 4: program the signed firmware ----
    _log("-" * 70)
    _log("Programming signed firmware ...")
    _run_ipecmd(ipecmd, status["mcu"], status["programmer"], image_abs)

    _log("-" * 70)
    _log("Programmed.  (Slot 14 untouched by host — bootloader will verify at boot.)")


# ----------------------------------------------------------------------------
# CLI
# ----------------------------------------------------------------------------

def _build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Standalone Method-1 signing & flashing tool "
                    "(Phase A: offline sign, Phase B: flash via ipecmd)."
    )
    sub = p.add_subparsers(dest="cmd", required=True)

    s = sub.add_parser("sign", help="Phase A only — sign components and produce signed hex/bin.")
    s.add_argument("--config", required=True, help="Path to mchp_non_tpds_fota_config.yaml")
    s.add_argument("--output-dir", default=None, help="Override output directory")

    f = sub.add_parser("flash", help="Phase B only — flash a signed hex via ipecmd.")
    f.add_argument("--image", required=True, help="Path to signed combined hex")
    f.add_argument("--config", default=None, help="Optional config yaml (for ipecmd/mcu/programmer overrides)")

    a = sub.add_parser("all", help="Run Phase A then Phase B.")
    a.add_argument("--config", required=True, help="Path to mchp_non_tpds_fota_config.yaml")
    a.add_argument("--output-dir", default=None, help="Override output directory")

    r = sub.add_parser(
        "run",
        help="Full workflow: sign (Phase A) then optionally flash (Phase B). "
             "Reads mchp_non_tpds_fota_config.ini — no UI required.",
    )
    r.add_argument(
        "--ini",
        default=None,
        help="Path to mchp_non_tpds_fota_config.ini (default: mchp_non_tpds_fota_config.ini next to this script)",
    )

    sub.add_parser("ui", help="Launch the Tkinter UI.")

    return p


def main(argv=None) -> int:
    # No arguments  →  run the ini-based workflow (no UI).
    if argv is None:
        argv = sys.argv[1:]

    _DEFAULT_INI = Path(__file__).resolve().parent / "mchp_non_tpds_fota_config.ini"

    try:
        if not argv:
            run_from_ini(str(_DEFAULT_INI))
            return 0

        args = _build_parser().parse_args(argv)
        if args.cmd == "run":
            ini_path = args.ini or str(_DEFAULT_INI)
            run_from_ini(ini_path)
            return 0

        if args.cmd == "ui":
            from standalone_ui import main as ui_main
            ui_main()
            return 0

        if args.cmd == "sign":
            phase_a_sign(_load_config(args.config), args.output_dir)

        elif args.cmd == "flash":
            flash_cfg = {}
            if args.config:
                with open(args.config, "r", encoding="utf-8") as f:
                    flash_cfg = yaml.safe_load(f) or {}
            phase_b_flash(args.image, flash_cfg)

        elif args.cmd == "all":
            cfg = _load_config(args.config)
            result = phase_a_sign(cfg, args.output_dir)
            combined = result.get("combined_hex")
            if not combined:
                # Only one component — use its signed hex directly.
                if len(result["signed_files"]) != 1:
                    raise RuntimeError("Phase B needs a combined hex or exactly one signed file.")
                combined = result["signed_files"][0]
            phase_b_flash(combined, cfg)

    except Exception as e:
        if VERBOSE_ERRORS:
            traceback.print_exc()
        _log(f"ERROR: {e}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
