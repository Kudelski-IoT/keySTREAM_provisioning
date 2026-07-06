import argparse
import asyncio
import contextlib
import os
import re
import time
from dataclasses import dataclass
from typing import Optional
from urllib import error as urllib_error
from urllib import request as urllib_request

from bleak import BleakClient, BleakScanner

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------
# Firmware source paths
# ---------------------------------------------------------------------------
_KTA_CONFIG_H = os.path.join(
    _SCRIPT_DIR,
    "firmware", "main", "kta_provisioning", "SOURCE", "include", "ktaConfig.h",
)
_KTA_FIELD_MGNT_HOOK_C = os.path.join(
    _SCRIPT_DIR,
    "firmware", "main", "ktaFieldMgntHook.c",
)
_APP_CONFIG_H = os.path.join(_SCRIPT_DIR, "App_Config.h")


# ---------------------------------------------------------------------------
# Generic C-header parsers (read-only – no firmware files are modified
# except App_Config.h for the device profile UID)
# ---------------------------------------------------------------------------
def _read_file(path: str) -> str:
    """Read a text file and return its content, or empty string on failure."""
    try:
        with open(path, encoding="utf-8") as fh:
            return fh.read()
    except OSError:
        return ""


def _parse_string_macro(content: str, macro_name: str) -> str:
    """Extract a string literal from a #define in C source text.

    Handles:
      #define MACRO  (const uint8_t*)"value"
      #define MACRO  "value"
    Returns empty string if not found.
    """
    pattern = re.compile(
        r'^\s*#define\s+' + re.escape(macro_name) + r'\s+.*?"([^"]+)"',
        re.MULTILINE,
    )
    m = pattern.search(content)
    return m.group(1) if m else ""


def _parse_byte_array_macro(content: str, macro_name: str) -> bytes:
    """Extract a byte-array initializer from a #define in C source text.

    Handles:  #define MACRO  {0x2b, 0x2b, 0x42, ...}
    Returns empty bytes if not found.
    """
    pattern = re.compile(
        r'^\s*#define\s+' + re.escape(macro_name) + r'\s+\{([^}]+)\}',
        re.MULTILINE,
    )
    m = pattern.search(content)
    if not m:
        return b""
    hex_values = re.findall(r'0x([0-9a-fA-F]{1,2})', m.group(1))
    return bytes(int(h, 16) for h in hex_values)


# ---------------------------------------------------------------------------
# Read firmware sources once at import time
# ---------------------------------------------------------------------------
_KTA_CONFIG_TEXT = _read_file(_KTA_CONFIG_H)
_KTA_HOOK_TEXT = _read_file(_KTA_FIELD_MGNT_HOOK_C)


# ---------------------------------------------------------------------------
# Resolve cloud URL from ktaConfig.h (authoritative source)
# ---------------------------------------------------------------------------
def _load_cloud_url() -> str:
    """Derive the cloud URL from C_KTA_APP__KEYSTREAM_HOST_HTTP_URL in ktaConfig.h."""
    host = _parse_string_macro(_KTA_CONFIG_TEXT, "C_KTA_APP__KEYSTREAM_HOST_HTTP_URL")
    if not host:
        raise SystemExit(
            "[CFG] ERROR: could not read C_KTA_APP__KEYSTREAM_HOST_HTTP_URL from ktaConfig.h.\n"
            f"       Looked in: {_KTA_CONFIG_H}"
        )
    url = f"http://{host}/lp1"
    print(f"[CFG] cloud URL sourced from ktaConfig.h: {url}")
    return url


# ---------------------------------------------------------------------------
# Resolve device_profile_uid: read directly from App_Config.h
# ---------------------------------------------------------------------------
def _load_device_profile_uid() -> bytes:
    """Read KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID from App_Config.h and return
    the encoded bytes used at runtime.

    Raises SystemExit with a clear message if the value is not configured.
    """
    app_config_text = _read_file(_APP_CONFIG_H)
    uid = _parse_string_macro(app_config_text, "KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID")
    if not uid or uid == "xxxxxxxxxxx":
        raise SystemExit(
            "[CFG] ERROR: 'KEYSTREAM_DEVICE_PUBLIC_PROFILE_UID' is not properly set in App_Config.h.\n"
            "       Please uncomment and enter the UID of your device profile in App_Config.h."
        )
    print(f"[CFG] device profile UID sourced from App_Config.h: {uid}")
    return uid.encode()


# ---------------------------------------------------------------------------
# Load byte-array values from firmware C sources
# ---------------------------------------------------------------------------
def _load_byte_array(macro_name: str, source_text: str, source_label: str) -> bytes:
    """Parse a byte-array macro from C source text with a descriptive log."""
    data = _parse_byte_array_macro(source_text, macro_name)
    if data:
        print(f"[CFG] {macro_name} ({len(data)} bytes) sourced from {source_label}")
    else:
        print(f"[CFG] WARNING: {macro_name} not found in {source_label}")
    return data


# ---------------------------------------------------------------------------
# Module-level config resolution (runs once at import time)
# ---------------------------------------------------------------------------
_KTA_CLOUD_URL_DEFAULT = _load_cloud_url()

SERVICE_UUID = "0000ffff-0000-1000-8000-00805f9b34fb"
CTRL_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"
DATA_UUID = "0000ff02-0000-1000-8000-00805f9b34fb"

# Simple binary opcodes (must match firmware OP_* defines in main.c)
BRIDGE_CMD_INITIALIZE            = 0x01
BRIDGE_CMD_STARTUP               = 0x02
BRIDGE_CMD_SET_DEVICE_INFO       = 0x03
BRIDGE_CMD_EXCHANGE_MESSAGE      = 0x04
BRIDGE_CMD_KEYSTREAM_STATUS      = 0x08
BRIDGE_CMD_SET_CHIP_CERTIFICATE  = 0x09
BRIDGE_CMD_REFURBISH             = 0xFA

OP_EXCHANGE = BRIDGE_CMD_EXCHANGE_MESSAGE
OP_STATUS   = BRIDGE_CMD_KEYSTREAM_STATUS

DISCOVERY_RETRIES = 4
DISCOVERY_RETRY_DELAY_SEC = 1.5
AUTO_RECONNECT_DELAYS_SEC = [3.0, 5.0, 8.0, 10.0, 15.0]

# Conservative FF01 write chunk for broad BLE stack compatibility.
# Large cloud replies (> MTU write size) are fragmented across multiple writes.
CTRL_WRITE_CHUNK_BYTES = 180
STATUS6_RECOVERY_COOLDOWN_SEC = 30.0
COMMAND_PAYLOAD_HINT_MIN_BYTES = 50
COMMAND_RECOVERY_WINDOW_SEC = 120.0
MAX_STATUS6_RECOVERY_ATTEMPTS = 5
STATUS6_EMPTY_POLL_FALLBACK_THRESHOLD = 8
STATUS6_EMPTY_POLL_FALLBACK_COOLDOWN_SEC = 90.0


@dataclass
class AutoCloudResumeState:
	exchange_idx: int = 0
	pending_step_idx: int = 0
	pending_kta2ks: Optional[bytes] = None
	pending_ks2rot: Optional[bytes] = None
	post_attempts: int = 0
	persistent_retry_mode: bool = False
	init_sent: bool = False
	bootstrap_done: bool = False
	status6_recovery_active: bool = False
	status6_recovery_cooldown_until: float = 0.0
	status6_recovery_count: int = 0
	awaiting_reinit_after_command: bool = False
	awaiting_reinit_until: float = 0.0
	status6_seen_since_command: int = 0
	status6_empty_poll_streak: int = 0
	status6_empty_poll_fallback_cooldown_until: float = 0.0


# Values sourced from ktaConfig.h
AUTO_L1_SEED = _load_byte_array("C_KTA_APP__L1_SEG_SEED", _KTA_CONFIG_TEXT, "ktaConfig.h")

# Values sourced from ktaFieldMgntHook.c
AUTO_CONTEXT_PROFILE_UID = _load_byte_array("C_KTA_APP_CONTEXT_PROFILE_UID", _KTA_HOOK_TEXT, "ktaFieldMgntHook.c")
AUTO_CONTEXT_SERIAL      = _load_byte_array("C_KTA_APP_CONTEXT_SERIAL_NUM",  _KTA_HOOK_TEXT, "ktaFieldMgntHook.c")
AUTO_CONTEXT_VERSION     = _load_byte_array("C_KTA_APP_CONTEXT_VERSION",     _KTA_HOOK_TEXT, "ktaFieldMgntHook.c")
AUTO_DEVICE_SERIAL       = _load_byte_array("C_KTA_APP_DEVICE_SERIAL_NUM",   _KTA_HOOK_TEXT, "ktaFieldMgntHook.c")

# Device profile UID sourced from App_Config.h
AUTO_DEVICE_PROFILE_UID = _load_device_profile_uid()


def _pad_fixed(data: bytes, size: int) -> bytes:
	if len(data) >= size:
		return data[:size]
	return data + (b"\x00" * (size - len(data)))


# ============================================================================
# Simple Binary Protocol Helpers
# ============================================================================
# Request  wire format: [opcode:1] [payloadLen:2LE] [payload:N]
# Response wire format: [opcode:1] [status:1] [payloadLen:2LE] [payload:N]

def _get_status_from_response(payload: bytes) -> int:
	"""Extract status byte from a simple-binary response payload."""
	return payload[0] if payload else 0xFF


def _get_kta_msg_from_exchange(payload: bytes) -> bytes:
	"""Extract outgoing KTA message from an EXCHANGE response payload [msgLen:2LE][msg:N]."""
	if len(payload) < 2:
		return b""
	msg_len = int.from_bytes(payload[:2], "little")
	if msg_len == 0 or len(payload) < 2 + msg_len:
		return b""
	return payload[2:2 + msg_len]


def _get_conn_request_from_response(payload: bytes) -> int:
	"""Extract connection-request byte from SET_DEVICE_INFO response payload."""
	return payload[0] if payload else 0


# ============================================================================
# Simple Binary Command Builders
# ============================================================================

def build_initialize() -> bytes:
	return build_frame(BRIDGE_CMD_INITIALIZE)


def build_startup() -> bytes:
	payload = _pad_fixed(AUTO_L1_SEED, 16)
	payload += _pad_fixed(AUTO_CONTEXT_PROFILE_UID, 32)
	payload += len(AUTO_CONTEXT_PROFILE_UID).to_bytes(4, "little")
	payload += _pad_fixed(AUTO_CONTEXT_SERIAL, 16)
	payload += len(AUTO_CONTEXT_SERIAL).to_bytes(4, "little")
	payload += _pad_fixed(AUTO_CONTEXT_VERSION, 16)
	payload += len(AUTO_CONTEXT_VERSION).to_bytes(4, "little")
	return build_frame(BRIDGE_CMD_STARTUP, payload)


def build_set_device_info() -> bytes:
	"""Device-profile UID + serial fallback.

	The firmware always overrides the serial with the ATECC608 chip serial when
	available.  We still need to send a non-zero serialLen so the firmware's
	length-validation check passes (serLen == 0 returns E_K_STATUS_PARAMETER).
	Use AUTO_DEVICE_SERIAL from ktaFieldMgntHook.c if present, else a 1-byte
	placeholder.
	"""
	serial = AUTO_DEVICE_SERIAL if AUTO_DEVICE_SERIAL else b"\x00"
	serial = serial[:16]  # clamp to field size
	payload = _pad_fixed(AUTO_DEVICE_PROFILE_UID, 32)
	payload += len(AUTO_DEVICE_PROFILE_UID).to_bytes(4, "little")
	payload += _pad_fixed(serial, 16)
	payload += len(serial).to_bytes(4, "little")
	return build_frame(BRIDGE_CMD_SET_DEVICE_INFO, payload)


def build_exchange(ks2kta_msg: bytes | None = None) -> bytes:
	ks = ks2kta_msg or b""
	payload = len(ks).to_bytes(2, "little") + ks
	return build_frame(BRIDGE_CMD_EXCHANGE_MESSAGE, payload)


def build_status() -> bytes:
	return build_frame(BRIDGE_CMD_KEYSTREAM_STATUS)


def build_refurbish() -> bytes:
	return build_frame(BRIDGE_CMD_REFURBISH)


# Legacy TLV aliases kept so existing callers still compile; map to new builders
build_initialize_tlv      = build_initialize
build_startup_tlv         = build_startup
build_set_device_info_tlv = build_set_device_info
build_exchange_tlv        = build_exchange
build_status_tlv          = build_status
build_refurbish_tlv       = build_refurbish


# Keep legacy build_frame for raw interactive "op" / "raw" commands
def build_frame(opcode: int, payload: bytes = b"") -> bytes:
	payload_len = len(payload)
	return bytes([opcode & 0xFF, payload_len & 0xFF, (payload_len >> 8) & 0xFF]) + payload


def parse_hex_payload(text: str) -> bytes:
	clean = text.replace(" ", "").replace("0x", "")
	if len(clean) % 2 != 0:
		raise ValueError("hex payload length must be even")
	return bytes.fromhex(clean)


class FrameBuffer:
	"""Reassembles simple binary response frames from chunked BLE notifications.

	Response frame format: [opcode:1] [status:1] [payloadLen:2LE] [payload:N]
	"""
	def __init__(self):
		self.buffer = bytearray()

	def add_data(self, data: bytes) -> list[tuple[int, int, bytes]]:
		"""Add a BLE chunk and return complete frames as (opcode, status, payload_bytes)."""
		self.buffer.extend(data)
		frames = []
		while len(self.buffer) >= 4:
			opcode      = self.buffer[0]
			status      = self.buffer[1]
			payload_len = int.from_bytes(self.buffer[2:4], "little")
			total_len   = 4 + payload_len
			if len(self.buffer) < total_len:
				break
			payload = bytes(self.buffer[4:total_len])
			del self.buffer[:total_len]
			frames.append((opcode, status, payload))
		return frames


def parse_notify_frame(data: bytes) -> Optional[tuple[int, int, bytes]]:
	"""Parse a single simple binary response frame.

	Format: [opcode:1] [status:1] [payloadLen:2LE] [payload:N]
	Returns (opcode, status, payload_bytes) or None if incomplete.
	"""
	if len(data) < 4:
		return None
	opcode      = data[0]
	status      = data[1]
	payload_len = int.from_bytes(data[2:4], "little")
	if len(data) < 4 + payload_len:
		return None
	payload = data[4:4 + payload_len]
	return opcode, status, payload


async def write_ctrl_frame(client: BleakClient, frame: bytes) -> None:
	"""Write one FF01 control frame, fragmenting if needed."""
	if len(frame) <= CTRL_WRITE_CHUNK_BYTES:
		await client.write_gatt_char(CTRL_UUID, frame, response=True)
		return

	total_chunks = (len(frame) + CTRL_WRITE_CHUNK_BYTES - 1) // CTRL_WRITE_CHUNK_BYTES
	for idx in range(total_chunks):
		start = idx * CTRL_WRITE_CHUNK_BYTES
		end = min(start + CTRL_WRITE_CHUNK_BYTES, len(frame))
		await client.write_gatt_char(CTRL_UUID, frame[start:end], response=True)

	print(
		f"[BLE] fragmented FF01 write: total={len(frame)} bytes "
		f"chunks={total_chunks} chunk_size={CTRL_WRITE_CHUNK_BYTES}"
	)


class CloudSession:
	"""Stateful keySTREAM HTTP session (cookie-based)."""

	def __init__(self):
		self._cookie_header: Optional[str] = None

	@staticmethod
	def _extract_cookie_kv(set_cookie: Optional[str]) -> Optional[str]:
		if not set_cookie:
			return None
		cookie_kv = set_cookie.split(";", 1)[0].strip()
		if "=" not in cookie_kv:
			return None
		return cookie_kv

	def post_binary(self, url: str, payload: bytes, timeout_sec: float) -> bytes:
		headers = {
			"Content-Type": "application/octet-stream",
			"Accept": "application/octet-stream",
		}
		if self._cookie_header:
			headers["Cookie"] = self._cookie_header

		print(
			f"[POST] sending {len(payload)} bytes to {url} "
			f"(cookie={'yes' if self._cookie_header else 'no'})..."
		)

		req = urllib_request.Request(url, data=payload, method="POST", headers=headers)

		try:
			with urllib_request.urlopen(req, timeout=timeout_sec) as resp:
				result = resp.read()
				set_cookie = resp.headers.get("Set-Cookie")
				cookie_kv = self._extract_cookie_kv(set_cookie)
				if cookie_kv:
					self._cookie_header = cookie_kv
					print("[POST] session cookie updated from server response")
				print(f"[POST] HTTP {resp.status} received {len(result)} bytes")
				return result
		except urllib_error.HTTPError as exc:
			body = b""
			try:
				body = exc.read() or b""
			except Exception:
				body = b""
			body_preview = body[:200].decode("utf-8", errors="replace").strip()
			if body_preview:
				print(
					f"[POST] ERROR: HTTP {exc.code} {exc.reason} "
					f"body='{body_preview}'"
				)
			else:
				print(f"[POST] ERROR: HTTP {exc.code} {exc.reason}")
			raise
		except Exception as exc:
			print(f"[POST] ERROR: {type(exc).__name__}: {exc}")
			raise


async def wait_for_any_notify(
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	timeout_sec: float,
	disconnected_event: Optional[asyncio.Event] = None,
) -> tuple[int, int, bytes]:
	while True:
		get_task = asyncio.create_task(notify_queue.get())
		disconnect_task: Optional[asyncio.Task[bool]] = None
		tasks: set[asyncio.Task] = {get_task}
		if disconnected_event is not None:
			disconnect_task = asyncio.create_task(disconnected_event.wait())
			tasks.add(disconnect_task)

		done, pending = await asyncio.wait(tasks, timeout=timeout_sec, return_when=asyncio.FIRST_COMPLETED)
		for task in pending:
			task.cancel()
		with contextlib.suppress(asyncio.CancelledError):
			for task in pending:
				await task

		if not done:
			raise asyncio.TimeoutError()

		if disconnect_task is not None and disconnect_task in done and disconnected_event is not None and disconnected_event.is_set():
			if not get_task.done():
				get_task.cancel()
			raise ConnectionError("BLE disconnected while waiting for EXCHANGE notification")

		if get_task not in done:
			continue

		return get_task.result()


async def wait_for_exchange_notify(
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	timeout_sec: float,
	disconnected_event: Optional[asyncio.Event] = None,
) -> tuple[int, bytes]:
	deadline = time.monotonic() + max(0.0, timeout_sec)
	while True:
		remaining = deadline - time.monotonic()
		if remaining <= 0:
			raise asyncio.TimeoutError()

		opcode, status, payload = await wait_for_any_notify(
			notify_queue,
			timeout_sec=remaining,
			disconnected_event=disconnected_event,
		)
		if opcode == OP_EXCHANGE:
			return status, payload


async def wait_for_opcode_notify(
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	expected_opcode: int,
	timeout_sec: float,
	disconnected_event: Optional[asyncio.Event] = None,
) -> tuple[int, bytes]:
	deadline = time.monotonic() + max(0.0, timeout_sec)
	while True:
		remaining = deadline - time.monotonic()
		if remaining <= 0:
			raise asyncio.TimeoutError()

		opcode, status, payload = await wait_for_any_notify(
			notify_queue,
			timeout_sec=remaining,
			disconnected_event=disconnected_event,
		)
		if opcode == expected_opcode:
			return status, payload


async def send_and_wait_opcode(
	client: BleakClient,
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	opcode: int,
	payload: bytes,
	step_name: str,
	timeout_sec: float,
	disconnected_event: asyncio.Event,
	retries: int = 3,
) -> tuple[int, bytes]:
	# payload is a pre-built TLV message (or legacy raw frame)
	frame = payload
	last_status = -1
	last_payload = b""

	for attempt in range(1, retries + 1):
		try:
			await write_ctrl_frame(client, frame)
		except Exception as exc:
			raise ConnectionError(f"BLE disconnected while writing {step_name}: {exc}") from exc

		try:
			status, resp_payload = await wait_for_opcode_notify(
				notify_queue,
				expected_opcode=opcode,
				timeout_sec=timeout_sec,
				disconnected_event=disconnected_event,
			)
		except asyncio.TimeoutError:
			print(f"[AUTO] {step_name} attempt {attempt}/{retries} timed out")
			if attempt < retries:
				await asyncio.sleep(1.0)
				continue
			raise

		last_status = status
		last_payload = resp_payload
		if status == 0:
			return status, resp_payload

		print(f"[AUTO] {step_name} attempt {attempt}/{retries} returned status={status}")
		if attempt < retries:
			await asyncio.sleep(1.0)

	return last_status, last_payload


async def run_kta_bootstrap_sequence(
	client: BleakClient,
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	state: AutoCloudResumeState,
	disconnected_event: asyncio.Event,
	step_timeout_sec: float,
) -> None:
	print("[AUTO] flow: INITIALIZE -> STARTUP -> SET_DEVICE_INFO -> EXCHANGE")

	if not state.init_sent:
		init_status, _ = await send_and_wait_opcode(
			client,
			notify_queue,
			opcode=BRIDGE_CMD_INITIALIZE,
			payload=build_initialize(),
			step_name="INITIALIZE",
			timeout_sec=step_timeout_sec,
			disconnected_event=disconnected_event,
			retries=3,
		)
		if init_status != 0:
			raise RuntimeError(f"INITIALIZE failed with status={init_status}")
		state.init_sent = True

	startup_status, _ = await send_and_wait_opcode(
		client,
		notify_queue,
		opcode=BRIDGE_CMD_STARTUP,
		payload=build_startup(),
		step_name="STARTUP",
		timeout_sec=step_timeout_sec,
		disconnected_event=disconnected_event,
		retries=3,
	)
	if startup_status != 0:
		raise RuntimeError(f"STARTUP failed with status={startup_status}")

	dev_info_status, dev_info_payload = await send_and_wait_opcode(
		client,
		notify_queue,
		opcode=BRIDGE_CMD_SET_DEVICE_INFO,
		payload=build_set_device_info(),
		step_name="SET_DEVICE_INFO",
		timeout_sec=step_timeout_sec,
		disconnected_event=disconnected_event,
		retries=3,
	)
	if dev_info_status != 0:
		raise RuntimeError(f"SET_DEVICE_INFO failed with status={dev_info_status}")

	if dev_info_payload:
		conn_req = _get_conn_request_from_response(dev_info_payload)
		print(f"[AUTO] SET_DEVICE_INFO response connectionRequest={conn_req}")

	state.bootstrap_done = True


async def run_auto_cloud_exchange(
	client: BleakClient,
	notify_queue: asyncio.Queue[tuple[int, int, bytes]],
	cloud_session: CloudSession,
	cloud_url: str,
	state: AutoCloudResumeState,
	disconnected_event: asyncio.Event,
	keepalive_pause_event: Optional[asyncio.Event],
	max_exchanges: int,
	cloud_timeout_sec: float,
	cloud_wait_timeout_sec: float,
	cloud_post_retries: int,
	cloud_retry_delay_sec: float,
	cloud_fail_policy: str,
) -> None:
	print(f"[AUTO] cloud relay enabled -> {cloud_url}")

	while max_exchanges <= 0 or state.exchange_idx < max_exchanges:
		if disconnected_event.is_set():
			raise ConnectionError("BLE disconnected during auto-cloud exchange")

		if state.awaiting_reinit_after_command and time.monotonic() > state.awaiting_reinit_until:
			print("[AUTO] command recovery window expired; returning to normal polling mode")
			state.awaiting_reinit_after_command = False
			state.awaiting_reinit_until = 0.0
			state.status6_seen_since_command = 0

		# Build simple binary EXCHANGE request
		ks2kta_msg = state.pending_ks2rot
		req_len = len(ks2kta_msg) if ks2kta_msg is not None else 0
		req_frame = build_exchange(ks2kta_msg)

		sent_step = state.exchange_idx + 1
		if req_len == 0:
			print(f"[AUTO] exchange step {sent_step}: sent EXCHANGE request with empty ks2kta payload")
		else:
			print(f"[AUTO] exchange step {sent_step}: sent EXCHANGE request with ks2kta len={req_len}")

		ex_frame: Optional[tuple[int, bytes]] = None
		ex_last_status: Optional[int] = None
		for ex_attempt in range(1, 4):
			if keepalive_pause_event is not None:
				keepalive_pause_event.set()
			try:
				await write_ctrl_frame(client, req_frame)
			except Exception as exc:
				if keepalive_pause_event is not None:
					keepalive_pause_event.clear()
				raise ConnectionError(f"BLE disconnected while writing EXCHANGE step {sent_step}: {exc}") from exc

			try:
				status, payload = await wait_for_exchange_notify(
					notify_queue,
					timeout_sec=cloud_wait_timeout_sec,
					disconnected_event=disconnected_event,
				)
				ex_last_status = status
			except asyncio.TimeoutError:
				if keepalive_pause_event is not None:
					keepalive_pause_event.clear()
				print(f"[AUTO] exchange step {sent_step} attempt {ex_attempt}/3 timed out waiting for EXCHANGE response")
				if ex_attempt < 3:
					await asyncio.sleep(1.0)
					continue
				if cloud_fail_policy == "stay-connected":
					print("[AUTO] keeping BLE link open after EXCHANGE timeout; retrying step")
					await asyncio.sleep(cloud_retry_delay_sec)
					continue
				return
			finally:
				if keepalive_pause_event is not None:
					keepalive_pause_event.clear()

			if status == 0:
				ex_frame = (status, payload)
				break

			print(f"[AUTO] exchange step {sent_step} attempt {ex_attempt}/3 returned status={status}")
			if ex_attempt < 3:
				await asyncio.sleep(1.0)
				continue

			if cloud_fail_policy == "stay-connected":
				print("[AUTO] keeping BLE link open after EXCHANGE status failure; retrying step")
				await asyncio.sleep(cloud_retry_delay_sec)
				continue
			return

		if ex_frame is None:
			if ex_last_status == 0x06:
				if req_len == 0:
					state.status6_empty_poll_streak += 1
					now = time.monotonic()
					if (
						not state.awaiting_reinit_after_command
						and state.status6_empty_poll_streak >= STATUS6_EMPTY_POLL_FALLBACK_THRESHOLD
					):
						if now >= state.status6_empty_poll_fallback_cooldown_until:
							state.awaiting_reinit_after_command = True
							state.awaiting_reinit_until = now + COMMAND_RECOVERY_WINDOW_SEC
							state.status6_seen_since_command = 1
							state.status6_recovery_count = 0
							state.status6_empty_poll_fallback_cooldown_until = now + STATUS6_EMPTY_POLL_FALLBACK_COOLDOWN_SEC
							print(
								"[AUTO] sustained EXCHANGE status=0x06 on empty polls; "
								"opening recovery window and triggering controlled re-init"
							)
						else:
							wait_left = max(0.0, state.status6_empty_poll_fallback_cooldown_until - now)
							print(
								"[AUTO] sustained EXCHANGE status=0x06 but fallback cooldown is active; "
								f"waiting {wait_left:.1f}s"
							)

					if not state.awaiting_reinit_after_command:
						print(
							"[AUTO] EXCHANGE status=0x06 on empty poll; "
							f"staying in listen mode (streak={state.status6_empty_poll_streak}) and continuing poll loop"
						)
						await asyncio.sleep(cloud_retry_delay_sec)
						continue

					state.status6_seen_since_command += 1
					print(
						"[AUTO] EXCHANGE status=0x06 after command relay "
						f"({state.status6_seen_since_command} sightings); preparing re-init flow"
					)
					if state.status6_recovery_count >= MAX_STATUS6_RECOVERY_ATTEMPTS:
						print(
							"[AUTO] max re-init attempts reached for this command window; "
							"continuing to poll without bootstrap"
						)
						await asyncio.sleep(cloud_retry_delay_sec)
						continue

					if state.status6_seen_since_command < 2:
						# Wait for one more poll to avoid reacting to a single transient frame.
						await asyncio.sleep(cloud_retry_delay_sec)
						continue

					now = time.monotonic()
					if state.status6_recovery_active and now < state.status6_recovery_cooldown_until:
						wait_left = max(0.0, state.status6_recovery_cooldown_until - now)
						print(
							f"[AUTO] waiting {wait_left:.1f}s before next re-init attempt"
						)
						await asyncio.sleep(cloud_retry_delay_sec)
						continue

					print(
						"[AUTO] triggering post-refurbish re-init flow: "
						"INITIALIZE -> STARTUP -> SET_DEVICE_INFO"
					)

					state.pending_ks2rot = None
					state.pending_kta2ks = None
					state.pending_step_idx = 0
					state.post_attempts = 0
					state.persistent_retry_mode = False
					state.bootstrap_done = False
					state.init_sent = False
					if keepalive_pause_event is not None:
						keepalive_pause_event.set()
					try:
						await run_kta_bootstrap_sequence(
							client,
							notify_queue,
							state,
							disconnected_event,
							step_timeout_sec=cloud_wait_timeout_sec,
						)
						state.status6_recovery_active = True
						state.status6_recovery_count += 1
						state.status6_recovery_cooldown_until = time.monotonic() + STATUS6_RECOVERY_COOLDOWN_SEC
						state.status6_empty_poll_streak = 0
						print("[AUTO] post-refurbish re-init completed; resuming EXCHANGE polling")
					except Exception as exc:
						state.status6_recovery_active = True
						state.status6_recovery_count += 1
						state.status6_recovery_cooldown_until = time.monotonic() + STATUS6_RECOVERY_COOLDOWN_SEC
						state.status6_empty_poll_streak = 0
						print(f"[AUTO] post-refurbish re-init failed: {exc}")
						await asyncio.sleep(cloud_retry_delay_sec)
					finally:
						if keepalive_pause_event is not None:
							keepalive_pause_event.clear()
					await asyncio.sleep(cloud_retry_delay_sec)
					continue

				now = time.monotonic()
				if state.status6_recovery_active and now < state.status6_recovery_cooldown_until:
					wait_left = max(0.0, state.status6_recovery_cooldown_until - now)
					print(
						f"[AUTO] EXCHANGE status=0x06 persists after bootstrap; "
						f"waiting {wait_left:.1f}s before retrying bootstrap"
					)
					await asyncio.sleep(cloud_retry_delay_sec)
					continue

				if state.status6_recovery_active:
					print("[AUTO] EXCHANGE status=0x06 while replaying cloud data; retrying bootstrap sequence.")
				else:
					print(
						"[AUTO] EXCHANGE status=0x06 while sending cloud data "
						"(device requires STARTUP/SET_DEVICE_INFO). Running bootstrap sequence."
					)

				# Clear the cloud payload that caused status=6 so we don't
				# re-send the same server ack after re-init completes.
				state.pending_ks2rot = None
				state.pending_kta2ks = None
				state.pending_step_idx = 0
				state.post_attempts = 0
				state.persistent_retry_mode = False
				state.bootstrap_done = False
				state.init_sent = False
				if keepalive_pause_event is not None:
					keepalive_pause_event.set()
				try:
					await run_kta_bootstrap_sequence(
						client,
						notify_queue,
						state,
						disconnected_event,
						step_timeout_sec=cloud_wait_timeout_sec,
					)
					state.status6_recovery_active = True
					state.status6_recovery_count += 1
					state.status6_recovery_cooldown_until = time.monotonic() + STATUS6_RECOVERY_COOLDOWN_SEC
					print(
						"[AUTO] bootstrap recovered; resuming EXCHANGE polling "
						f"(cooldown {STATUS6_RECOVERY_COOLDOWN_SEC:.0f}s)"
					)
				except Exception as exc:
					state.status6_recovery_active = True
					state.status6_recovery_count += 1
					state.status6_recovery_cooldown_until = time.monotonic() + STATUS6_RECOVERY_COOLDOWN_SEC
					print(f"[AUTO] bootstrap recovery failed: {exc}")
					print(
						"[AUTO] entering 0x06 cooldown mode; "
						f"next bootstrap retry in {STATUS6_RECOVERY_COOLDOWN_SEC:.0f}s"
					)
					await asyncio.sleep(cloud_retry_delay_sec)
				finally:
					if keepalive_pause_event is not None:
						keepalive_pause_event.clear()
			if ex_last_status != 0x06:
				state.status6_empty_poll_streak = 0
			continue

		if state.status6_recovery_active:
			print("[AUTO] EXCHANGE recovered from status=0x06; exiting recovery mode")
			state.status6_recovery_active = False
			state.status6_recovery_cooldown_until = 0.0
			state.status6_seen_since_command = 0
			state.status6_empty_poll_streak = 0

		if state.awaiting_reinit_after_command and req_len == 0:
			print("[AUTO] post-command polling is healthy again; re-init window closed")
			state.awaiting_reinit_after_command = False
			state.awaiting_reinit_until = 0.0
			state.status6_seen_since_command = 0
			state.status6_recovery_count = 0
			state.status6_empty_poll_streak = 0

		_, payload = ex_frame
		# Simple binary EXCHANGE response payload: [msgLen:2LE][msg:N]
		if len(payload) < 2:
			print("[AUTO] invalid EXCHANGE response (too short)")
			if cloud_fail_policy == "stay-connected":
				await asyncio.sleep(cloud_retry_delay_sec)
				continue
			return
		rot2ks = _get_kta_msg_from_exchange(payload)
		if len(rot2ks) == 0:
			print("[AUTO] device returned empty rot2ks; keeping BLE alive and polling for portal commands")
			state.pending_ks2rot = None
			state.pending_kta2ks = None
			state.pending_step_idx = 0
			state.post_attempts = 0
			state.persistent_retry_mode = False
			state.exchange_idx += 1
			await asyncio.sleep(cloud_retry_delay_sec)
			continue

		print(f"[AUTO] exchange #{sent_step}: extracted kta2ks len={len(rot2ks)}")
		state.pending_step_idx = sent_step
		state.pending_kta2ks = rot2ks
		state.post_attempts = 0
		state.persistent_retry_mode = False

		while True:
			try:
				ks2kta = await asyncio.to_thread(
					cloud_session.post_binary,
					cloud_url,
					state.pending_kta2ks,
					cloud_timeout_sec,
				)
				break
			except Exception as exc:
				state.post_attempts += 1
				max_attempts = cloud_post_retries + 1
				if state.post_attempts <= max_attempts:
					print(
						f"[AUTO] cloud POST failed for exchange #{state.pending_step_idx} "
						f"(attempt {state.post_attempts}/{max_attempts}): {exc}"
					)
				else:
					if cloud_fail_policy != "stay-connected":
						print(f"[AUTO] cloud POST failed for exchange #{state.pending_step_idx}: {exc}")
						return
					if not state.persistent_retry_mode:
						state.persistent_retry_mode = True
						print(
							f"[AUTO] entering persistent retry mode for exchange #{state.pending_step_idx}; "
							"same payload is preserved until cloud accepts it"
						)
					else:
						print(f"[AUTO] retrying preserved exchange #{state.pending_step_idx}: {exc}")

				await asyncio.sleep(cloud_retry_delay_sec)
				if disconnected_event.is_set():
					raise ConnectionError("BLE disconnected during cloud retry backoff")

		print(f"[AUTO] exchange #{state.pending_step_idx}: cloud replied {len(ks2kta)} bytes")
		if len(ks2kta) >= COMMAND_PAYLOAD_HINT_MIN_BYTES:
			state.awaiting_reinit_after_command = True
			state.awaiting_reinit_until = time.monotonic() + COMMAND_RECOVERY_WINDOW_SEC
			state.status6_seen_since_command = 0
			state.status6_recovery_count = 0
			print(
				"[AUTO] command-sized cloud payload observed; "
				f"watching for post-command re-init trigger for {COMMAND_RECOVERY_WINDOW_SEC:.0f}s"
			)

		if len(ks2kta) == 0:
			print("[AUTO] cloud returned empty response; continuing EXCHANGE polling")
			state.pending_ks2rot = None
			state.pending_kta2ks = None
			state.pending_step_idx = 0
			state.post_attempts = 0
			state.persistent_retry_mode = False
			state.exchange_idx += 1
			await asyncio.sleep(cloud_retry_delay_sec)
			continue
		if len(ks2kta) > 0xFFFF:
			print(f"[AUTO] cloud response too large: {len(ks2kta)} bytes")
			return

		state.pending_ks2rot = ks2kta
		state.pending_kta2ks = None
		state.post_attempts = 0
		state.persistent_retry_mode = False
		state.exchange_idx += 1

	if max_exchanges > 0:
		print(f"[AUTO] reached max exchanges ({max_exchanges})")


async def find_device(
	target: str,
	timeout: float,
	retries: int = DISCOVERY_RETRIES,
	retry_delay_sec: float = DISCOVERY_RETRY_DELAY_SEC,
) -> Optional[str]:
	target_l = target.lower()

	for attempt in range(retries + 1):
		devices = await BleakScanner.discover(timeout=timeout)
		for d in devices:
			name = (d.name or "").lower()
			addr = (d.address or "").lower()
			if target_l == addr or target_l in name:
				return d.address

		if attempt < retries:
			print(
				f"[BLE] device '{target}' not visible yet "
				f"(scan attempt {attempt + 1}/{retries + 1}); retrying..."
			)
			await asyncio.sleep(retry_delay_sec)

	return None


async def keepalive_loop(
	client: BleakClient,
	interval_sec: float,
	pause_event: Optional[asyncio.Event] = None,
) -> None:
	# Send simple binary KEY_STREAM_STATUS periodically to keep FF01 flow active.
	while True:
		await asyncio.sleep(interval_sec)
		if pause_event is not None and pause_event.is_set():
			continue
		frame = build_status()
		try:
			await write_ctrl_frame(client, frame)
			print(f"[KA] sent {frame.hex()}")
		except Exception as exc:
			print(f"[KA] stopped (write failed): {exc}")
			return


async def run_auto_cloud_with_reconnect(
	target: str,
	scan_timeout: float,
	send_init: bool,
	keepalive_sec: float,
	cloud_url: str,
	cloud_timeout_sec: float,
	cloud_wait_timeout_sec: float,
	cloud_post_retries: int,
	cloud_retry_delay_sec: float,
	cloud_fail_policy: str,
	max_exchanges: int,
) -> None:
	cloud_session = CloudSession()
	state = AutoCloudResumeState()
	effective_keepalive_sec = keepalive_sec
	if effective_keepalive_sec <= 0:
		effective_keepalive_sec = 2.0
		print("[KA] keepalive-sec <= 0 in auto-cloud mode; forcing 2.0s for BLE stability")
	reconnect_attempt = 0

	while True:
		address = await find_device(target, scan_timeout)
		if not address:
			raise RuntimeError(
				f"device not found for target '{target}' after {DISCOVERY_RETRIES + 1} scans; "
				"ensure ESP32 is advertising and try again"
			)

		notify_queue: asyncio.Queue[tuple[int, int, bytes]] = asyncio.Queue()
		frame_buffer = FrameBuffer()
		disconnected_event = asyncio.Event()
		loop = asyncio.get_running_loop()
		keepalive_task: Optional[asyncio.Task] = None
		keepalive_pause_event = asyncio.Event()

		def on_notify(_: int, data: bytearray) -> None:
			raw = bytes(data)
			print(f"[FF02] {raw.hex()}")
			frames = frame_buffer.add_data(raw)
			for frame in frames:
				print(f"[FRAME] complete: cmd=0x{frame[0]:02x} status=0x{frame[1]:02x} msg_len={len(frame[2])}")
				notify_queue.put_nowait(frame)

		def on_disconnected(_: BleakClient) -> None:
			loop.call_soon_threadsafe(disconnected_event.set)

		try:
			print(f"[BLE] Connecting to {address}")
			async with BleakClient(address, timeout=20.0, disconnected_callback=on_disconnected) as client:
				if not client.is_connected:
					raise ConnectionError("failed to connect")

				reconnect_attempt = 0
				print("[BLE] Connected")

				if hasattr(client, "get_services"):
					services = await client.get_services()
				else:
					services = client.services
				svc = services.get_service(SERVICE_UUID) if services is not None else None
				if svc is None:
					print("[WARN] keySTREAM service UUID not found; continuing with known UUIDs")

				last_notify_error: Optional[Exception] = None
				notify_enabled = False
				for attempt in range(1, 4):
					try:
						await client.start_notify(DATA_UUID, on_notify)
						notify_enabled = True
						print("[BLE] Subscribed to FF02 notifications")
						break
					except Exception as exc:
						last_notify_error = exc
						print(f"[WARN] FF02 notify subscribe attempt {attempt}/3 failed: {exc}")
						await asyncio.sleep(0.5)

				if not notify_enabled:
					raise ConnectionError(f"failed to subscribe FF02 notifications: {last_notify_error}")

				if not state.bootstrap_done:
					await run_kta_bootstrap_sequence(
						client,
						notify_queue,
						state,
						disconnected_event,
						step_timeout_sec=cloud_wait_timeout_sec,
					)

				if state.pending_ks2rot is not None:
					print(f"[AUTO] resuming exchange #{state.pending_step_idx}: pending cloud response will be sent to ESP32")
				elif state.pending_kta2ks is not None:
					print(f"[AUTO] resuming exchange #{state.pending_step_idx}: retrying preserved payload to cloud")

				if effective_keepalive_sec > 0:
					keepalive_task = asyncio.create_task(
						keepalive_loop(client, effective_keepalive_sec, keepalive_pause_event)
					)
					print(f"[KA] enabled every {effective_keepalive_sec:.1f}s using opcode 0x08")

				await run_auto_cloud_exchange(
					client,
					notify_queue,
					cloud_session,
					cloud_url,
					state,
					disconnected_event,
					keepalive_pause_event,
					max_exchanges,
					cloud_timeout_sec,
					cloud_wait_timeout_sec,
					cloud_post_retries,
					cloud_retry_delay_sec,
					cloud_fail_policy,
				)

				if keepalive_task is not None:
					keepalive_task.cancel()
					with contextlib.suppress(asyncio.CancelledError):
						await keepalive_task
				with contextlib.suppress(Exception):
					await client.stop_notify(DATA_UUID)
				print("[BLE] Disconnected")
				return

		except (ConnectionError, asyncio.TimeoutError) as exc:
			print(f"[BLE] connection interrupted: {exc}")
		except Exception as exc:
			print(f"[BLE] connection error: {type(exc).__name__}: {exc}")
		finally:
			if keepalive_task is not None:
				keepalive_task.cancel()
				with contextlib.suppress(asyncio.CancelledError):
					await keepalive_task

		delay_idx = reconnect_attempt if reconnect_attempt < len(AUTO_RECONNECT_DELAYS_SEC) else len(AUTO_RECONNECT_DELAYS_SEC) - 1
		delay_sec = AUTO_RECONNECT_DELAYS_SEC[delay_idx]
		reconnect_attempt += 1
		print(f"[AUTO] reconnect attempt {reconnect_attempt} in {delay_sec:.0f}s (will resume step #{state.pending_step_idx if state.pending_step_idx else state.exchange_idx + 1})")
		await asyncio.sleep(delay_sec)


async def run(
	target: str,
	scan_timeout: float,
	send_init: bool,
	keepalive_sec: float,
	no_notify: bool,
	auto_cloud: bool,
	cloud_url: str,
	cloud_timeout_sec: float,
	cloud_wait_timeout_sec: float,
	cloud_post_retries: int,
	cloud_retry_delay_sec: float,
	cloud_fail_policy: str,
	max_exchanges: int,
) -> None:
	if auto_cloud:
		await run_auto_cloud_with_reconnect(
			target,
			scan_timeout,
			send_init,
			keepalive_sec,
			cloud_url,
			cloud_timeout_sec,
			cloud_wait_timeout_sec,
			cloud_post_retries,
			cloud_retry_delay_sec,
			cloud_fail_policy,
			max_exchanges,
		)
		return

	address = await find_device(target, scan_timeout)
	if not address:
		raise RuntimeError(
			f"device not found for target '{target}' after {DISCOVERY_RETRIES + 1} scans; "
			"ensure ESP32 is advertising and try again"
		)

	print(f"[BLE] Connecting to {address}")

	notify_queue: asyncio.Queue[tuple[int, int, bytes]] = asyncio.Queue()
	frame_buffer = FrameBuffer()

	def on_notify(_: int, data: bytearray) -> None:
		raw = bytes(data)
		print(f"[FF02] {raw.hex()}")
		frames = frame_buffer.add_data(raw)
		for frame in frames:
			print(f"[FRAME] complete: cmd=0x{frame[0]:02x} status=0x{frame[1]:02x} msg_len={len(frame[2])}")
			notify_queue.put_nowait(frame)

	async with BleakClient(address, timeout=20.0) as client:
		keepalive_task: Optional[asyncio.Task] = None
		if not client.is_connected:
			raise RuntimeError("failed to connect")

		print("[BLE] Connected")

		if hasattr(client, "get_services"):
			services = await client.get_services()
		else:
			services = client.services
		svc = services.get_service(SERVICE_UUID) if services is not None else None
		if svc is None:
			print("[WARN] keySTREAM service UUID not found; continuing with known UUIDs")

		notify_enabled = False
		if no_notify:
			print("[WARN] --no-notify set; skipping FF02 subscription")
		else:
			last_notify_error: Optional[Exception] = None
			for attempt in range(1, 4):
				try:
					await client.start_notify(DATA_UUID, on_notify)
					notify_enabled = True
					print("[BLE] Subscribed to FF02 notifications")
					break
				except Exception as exc:
					last_notify_error = exc
					print(f"[WARN] FF02 notify subscribe attempt {attempt}/3 failed: {exc}")
					await asyncio.sleep(0.5)

			if not notify_enabled:
				print("[WARN] Continuing without FF02 notifications (write-only mode)")
				if last_notify_error is not None:
					print(f"[WARN] Last notify error: {last_notify_error}")

		if send_init:
			frame = build_initialize_tlv()
			await write_ctrl_frame(client, frame)
			print("[TX] INITIALIZE sent")

		if keepalive_sec > 0:
			# Prime the link immediately so FF01 flow starts before Windows idles the session.
			try:
				frame = build_status_tlv()
				await write_ctrl_frame(client, frame)
				print(f"[KA] prime sent {frame.hex()}")
			except Exception as exc:
				print(f"[WARN] keepalive prime failed: {exc}")
			keepalive_task = asyncio.create_task(keepalive_loop(client, keepalive_sec))
			print(f"[KA] enabled every {keepalive_sec:.1f}s")

		print("[CLI] Enter commands:")
		print("  init")
		print("  status")
		print("  op <hex_opcode> [hex_payload]")
		print("  raw <hex_bytes>")
		print("  quit")

		while True:
			if keepalive_task is not None and keepalive_task.done():
				print("[CLI] link no longer writable; exiting")
				break

			try:
				line = await asyncio.to_thread(input, "> ")
			except (EOFError, KeyboardInterrupt):
				break
			line = line.strip()
			if not line:
				continue

			if line.lower() in {"q", "quit", "exit"}:
				break

			parts = line.split(maxsplit=2)
			cmd = parts[0].lower()

			try:
				if cmd == "init":
					frame = build_initialize_tlv()
				elif cmd == "status":
					frame = build_status_tlv()
				elif cmd == "op":
					if len(parts) < 2:
						print("usage: op <hex_opcode> [hex_payload]")
						continue
					opcode = int(parts[1], 16)
					payload = parse_hex_payload(parts[2]) if len(parts) >= 3 else b""
					frame = build_frame(opcode, payload)
				elif cmd == "raw":
					if len(parts) < 2:
						print("usage: raw <hex_bytes>")
						continue
					frame = parse_hex_payload(parts[1])
				else:
					print("unknown command")
					continue

				await write_ctrl_frame(client, frame)
				print(f"[TX] {frame.hex()}")
			except Exception as exc:
				print(f"[ERR] {exc}")

		if keepalive_task is not None:
			keepalive_task.cancel()
			with contextlib.suppress(asyncio.CancelledError):
				await keepalive_task

		if notify_enabled:
			await client.stop_notify(DATA_UUID)
		print("[BLE] Disconnected")


async def main() -> None:
	parser = argparse.ArgumentParser(description="Simple Windows BLE client for ESP32 keySTREAM service")
	parser.add_argument("target", help="BLE address or part of device name (e.g. ESP32-Keystream)")
	parser.add_argument("--scan-timeout", type=float, default=8.0)
	parser.add_argument("--init", action="store_true", help="send INITIALIZE right after connect")
	parser.add_argument(
		"--no-notify",
		action="store_true",
		help="skip FF02 notification subscription (useful if Windows returns start_notify Unreachable)",
	)
	parser.add_argument(
		"--keepalive-sec",
		type=float,
		default=2.0,
		help="send periodic OP_KEY_STREAM_STATUS (0x08) on FF01 to keep session active (0=off, default 2.0)",
	)
	parser.add_argument(
		"--auto-cloud",
		action="store_true",
		help="automatically relay OP_EXCHANGE (0x04) data to keySTREAM cloud and feed replies back to ESP32",
	)
	parser.add_argument(
		"--cloud-url",
		default=_KTA_CLOUD_URL_DEFAULT,
		help="keySTREAM endpoint URL for binary POST (default sourced from ktaConfig.h; can be overridden on the command line)",
	)
	parser.add_argument(
		"--cloud-timeout-sec",
		type=float,
		default=20.0,
		help="HTTP timeout for cloud POSTs",
	)
	parser.add_argument(
		"--cloud-wait-timeout-sec",
		type=float,
		default=30.0,
		help="seconds to wait for each EXCHANGE notification in auto-cloud mode",
	)
	parser.add_argument(
		"--cloud-post-retries",
		type=int,
		default=2,
		help="quick retry count before persistent retry mode (stay-connected policy)",
	)
	parser.add_argument(
		"--cloud-retry-delay-sec",
		type=float,
		default=3.0,
		help="delay between cloud retry attempts for the same preserved exchange payload",
	)
	parser.add_argument(
		"--cloud-fail-policy",
		choices=["disconnect", "stay-connected"],
		default="stay-connected",
		help="on cloud POST failure: disconnect immediately or keep BLE connected and retry same state",
	)
	parser.add_argument(
		"--max-exchanges",
		type=int,
		default=0,
		help="maximum cloud exchange iterations (0 = unlimited)",
	)
	args = parser.parse_args()

	await run(
		args.target,
		args.scan_timeout,
		args.init,
		args.keepalive_sec,
		args.no_notify,
		args.auto_cloud,
		args.cloud_url,
		args.cloud_timeout_sec,
		args.cloud_wait_timeout_sec,
		args.cloud_post_retries,
		args.cloud_retry_delay_sec,
		args.cloud_fail_policy,
		args.max_exchanges,
	)


if __name__ == "__main__":
	try:
		asyncio.run(main())
	except KeyboardInterrupt:
		print("[CLI] interrupted")
