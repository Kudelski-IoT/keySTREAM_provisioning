import { Device, Characteristic } from 'react-native-ble-plx';
import { Buffer } from 'buffer';
import { bleManager } from './bleManager';

import {
  encodeRequestFrame,
  KTA_OPCODE,
  tryDecodeResponseFrames,
  type KtaBinaryResponseFrame,
} from './KTAProtocol';

// keySTREAM BLE GATT Service and Characteristics UUIDs
const KEYSTREAM_SERVICE_UUID = '0000FFFF-0000-1000-8000-00805F9B34FB';
const CHAR_WRITE_COMMAND_UUID = '0000FF01-0000-1000-8000-00805F9B34FB';
const CHAR_NOTIFY_RESPONSE_UUID = '0000FF02-0000-1000-8000-00805F9B34FB';
const CHAR_WRITE_ACK_UUID = '0000FF03-0000-1000-8000-00805F9B34FB';
const CHAR_NOTIFY_STATUS_UUID = '0000FF04-0000-1000-8000-00805F9B34FB';

// BLE ATT write payload is typically (MTU - 3). If we don't know the negotiated MTU,
// we MUST assume the legacy default MTU=23 => payload max=20 bytes.
// Using a larger default can trigger Android "GATT_INVALID_ATTR_LEN" and crash the app
// due to a react-native-ble-plx native Promise rejection bug.
// With MTU=247, max safe chunk is 244 bytes (247-3). Using 240 for safety margin.
const FALLBACK_MAX_CHUNK_SIZE = 240;

// When MTU negotiation succeeds, prefer larger chunks for throughput.
// Keep a conservative cap for stack stability.
const PREFERRED_MAX_CHUNK_SIZE = 180;

export interface BLETransportCallbacks {
  onConnected?: (device: Device) => void;
  onDisconnected?: (error?: string) => void;
  onDataReceived?: (data: string) => void;
  onBinaryFrameReceived?: (frame: KtaBinaryResponseFrame) => void;
  onStatusUpdate?: (status: string) => void;
  onError?: (error: string) => void;
  onLog?: (message: string, type: 'info' | 'success' | 'error' | 'warning') => void;
}

export class BLETransport {
  private connectedDevice: Device | null = null;
  private callbacks: BLETransportCallbacks;
  private responseBuffer: string = '';
  private responseBytes: Buffer = Buffer.alloc(0);
  private _isConnected: boolean = false;
  private responseFlushTimer: any = null;
  private readonly responseFlushDelayMs = 250;
  private primaryNotifySource: string | null = null;
  private maxChunkSize: number = FALLBACK_MAX_CHUNK_SIZE;
  private readonly maxResponseBytesBuffered = 64 * 1024;
  private gattProfile: {
    writeServiceUuid: string;
    writeUuid: string;
    notifyServiceUuid: string;
    notifyUuid: string;
    statusServiceUuid?: string;
    statusUuid?: string;
    ackServiceUuid?: string;
    ackUuid?: string;
    extraNotifies?: Array<{ serviceUuid: string; uuid: string }>;
  } | null = null;

  private readonly maxExtraNotifyMonitors = 8;

  private isKnownUuid(uuid: string, known: string) {
    return this.normalizeUuid(uuid) === this.normalizeUuid(known);
  }

  constructor(callbacks: BLETransportCallbacks = {}) {
    this.callbacks = callbacks;
  }

  private clearResponseFlushTimer() {
    if (this.responseFlushTimer) {
      clearTimeout(this.responseFlushTimer);
      this.responseFlushTimer = null;
    }
  }

  /**
   * Extract complete JSON objects from a buffer that may contain:
   * - partial JSON
   * - multiple JSON objects concatenated (e.g. `{...}{...}`)
   * - optional whitespace/newlines between objects
   *
   * Returns extracted frames and the remaining (incomplete) tail.
   */
  private extractJsonObjectFrames(buffer: string): { frames: string[]; remainder: string } {
    const frames: string[] = [];
    let depth = 0;
    let inString = false;
    let escape = false;
    let startIndex = -1;
    let lastEnd = -1;

    for (let i = 0; i < buffer.length; i++) {
      const ch = buffer[i];

      if (inString) {
        if (escape) {
          escape = false;
          continue;
        }
        if (ch === '\\') {
          escape = true;
          continue;
        }
        if (ch === '"') {
          inString = false;
        }
        continue;
      }

      if (ch === '"') {
        inString = true;
        continue;
      }

      if (ch === '{') {
        if (depth === 0) startIndex = i;
        depth++;
        continue;
      }

      if (ch === '}') {
        if (depth > 0) depth--;
        if (depth === 0 && startIndex >= 0) {
          const endIndex = i + 1;
          const frame = buffer.slice(startIndex, endIndex).trim();
          if (frame) frames.push(frame);
          lastEnd = endIndex;
          startIndex = -1;
        }
      }
    }

    const remainder = lastEnd >= 0 ? buffer.slice(lastEnd) : buffer;
    return { frames, remainder };
  }

  private drainResponseBuffer(reason: 'chunk' | 'terminator' | 'idle') {
    // First try to split concatenated JSON objects safely.
    const { frames, remainder } = this.extractJsonObjectFrames(this.responseBuffer);
    if (frames.length) {
      this.responseBuffer = remainder;
      this.clearResponseFlushTimer();
      for (const frame of frames) {
        this.log(`Complete response received (${reason}): ${frame.length} bytes`, 'success');
        this.callbacks.onDataReceived?.(frame);
      }

      // If we still have a partial tail, keep the idle flush armed.
      if (this.responseBuffer.trim().length) {
        this.scheduleIdleFlush();
      }
      return;
    }

    // If no complete JSON object is found, fall back to the legacy behavior.
    if (reason === 'idle') {
      this.flushResponseBuffer('idle');
      return;
    }

    // Otherwise just keep buffering.
    this.scheduleIdleFlush();
  }

  private flushResponseBuffer(reason: 'terminator' | 'idle') {
    const completeResponse = this.responseBuffer.trim();
    this.responseBuffer = '';
    this.clearResponseFlushTimer();

    if (!completeResponse) return;

    this.log(
      `Complete response received (${reason}): ${completeResponse.length} bytes`,
      'success'
    );
    this.callbacks.onDataReceived?.(completeResponse);
  }

  private scheduleIdleFlush() {
    this.clearResponseFlushTimer();
    this.responseFlushTimer = setTimeout(() => {
      // Only flush if we still have buffered data.
      if (!this.responseBuffer) return;
      this.flushResponseBuffer('idle');
    }, this.responseFlushDelayMs);
  }

  private log(message: string, type: 'info' | 'success' | 'error' | 'warning' = 'info') {
    // Only surface critical errors to the app UI
    if (type === 'error') {
      try {
        this.callbacks.onLog?.(message, type);
      } catch (e: any) {
        const msg = e?.message || String(e);
        console.warn(`[BLETransport] onLog callback threw: ${msg}`);
      }
    }
  }

  private isLikelyTextPayload(bytes: Buffer): boolean {
    if (!bytes.length) return false;
    // Fast-path for JSON-ish payloads.
    const first = bytes[0];
    if (first === 0x7b /* { */ || first === 0x5b /* [ */) return true;

    // Heuristic: treat as text if mostly printable ASCII/whitespace and no NULs.
    let printable = 0;
    for (const b of bytes) {
      if (b === 0x00) return false;
      if (b === 0x09 || b === 0x0a || b === 0x0d || (b >= 0x20 && b <= 0x7e)) printable++;
    }
    return printable / bytes.length >= 0.9;
  }

  private normalizeUuid(uuid: string): string {
    return (uuid || '').toLowerCase();
  }

  private pickPreferredUuid(candidates: Characteristic[], preferredUuid: string): Characteristic | undefined {
    const preferred = this.normalizeUuid(preferredUuid);
    return candidates.find((c) => this.normalizeUuid(c.uuid) === preferred);
  }

  private async resolveGattProfile(device: Device): Promise<{
    writeServiceUuid: string;
    writeUuid: string;
    notifyServiceUuid: string;
    notifyUuid: string;
    statusServiceUuid?: string;
    statusUuid?: string;
    ackServiceUuid?: string;
    ackUuid?: string;
    extraNotifies?: Array<{ serviceUuid: string; uuid: string }>;
  }> {
    const services = await device.services();
    const serviceUuids = services.map((s) => s.uuid);
    const preferredService = this.normalizeUuid(KEYSTREAM_SERVICE_UUID);

    const allWritable: Array<{ serviceUuid: string; c: Characteristic }> = [];
    const allNotifiable: Array<{ serviceUuid: string; c: Characteristic }> = [];
    const serviceChars: Record<string, Characteristic[]> = {};

    for (const serviceUuid of serviceUuids) {
      try {
        const chars = await device.characteristicsForService(serviceUuid);
        serviceChars[serviceUuid] = chars;
        for (const c of chars) {
          if (!!c.isWritableWithResponse || !!(c as any).isWritableWithoutResponse) {
            allWritable.push({ serviceUuid, c });
          }
          if (!!c.isNotifiable || !!c.isIndicatable) {
            allNotifiable.push({ serviceUuid, c });
          }
        }
      } catch {
        // ignore service
      }
    }

    if (!allWritable.length || !allNotifiable.length) {
      this.log(
        `No suitable GATT characteristics found (writable=${allWritable.length}, notifiable=${allNotifiable.length}). Services: ${
          serviceUuids.join(', ') || '(none)'
        }`,
        'error'
      );
      throw new Error('No writable or notifiable characteristics found on device');
    }

    // First try: find the known keySTREAM characteristic UUIDs (FF01 write + FF02 notify)
    // even if the service UUID differs (some firmwares expose a custom service UUID).
    const knownWrite = allWritable.find((w) => this.isKnownUuid(w.c.uuid, CHAR_WRITE_COMMAND_UUID));
    const knownNotify = allNotifiable.find((n) => this.isKnownUuid(n.c.uuid, CHAR_NOTIFY_RESPONSE_UUID));
    const knownStatus = allNotifiable.find((n) => this.isKnownUuid(n.c.uuid, CHAR_NOTIFY_STATUS_UUID));
    const knownAck = allWritable.find((w) => this.isKnownUuid(w.c.uuid, CHAR_WRITE_ACK_UUID));

    if (knownWrite && knownNotify) {
      const extras = allNotifiable
        .filter(
          (n) =>
            !(n.serviceUuid === knownNotify.serviceUuid && n.c.uuid === knownNotify.c.uuid) &&
            !(knownStatus && n.serviceUuid === knownStatus.serviceUuid && n.c.uuid === knownStatus.c.uuid)
        )
        .slice(0, this.maxExtraNotifyMonitors)
        .map((n) => ({ serviceUuid: n.serviceUuid, uuid: n.c.uuid }));

      const profile = {
        writeServiceUuid: knownWrite.serviceUuid,
        writeUuid: knownWrite.c.uuid,
        notifyServiceUuid: knownNotify.serviceUuid,
        notifyUuid: knownNotify.c.uuid,
        statusServiceUuid: knownStatus?.serviceUuid,
        statusUuid: knownStatus?.c.uuid,
        ackServiceUuid: knownAck?.serviceUuid,
        ackUuid: knownAck?.c.uuid,
        extraNotifies: extras,
      };

      this.log(
        `Using KNOWN keySTREAM UUIDs write=${profile.writeServiceUuid}/${profile.writeUuid}, notify=${profile.notifyServiceUuid}/${profile.notifyUuid}, status=${
          profile.statusServiceUuid && profile.statusUuid
            ? `${profile.statusServiceUuid}/${profile.statusUuid}`
            : '(none)'
        }, ack=${
          profile.ackServiceUuid && profile.ackUuid ? `${profile.ackServiceUuid}/${profile.ackUuid}` : '(none)'
        }, extras=${profile.extraNotifies?.length ?? 0}`,
        'info'
      );

      return profile;
    }

    const preferredServiceUuid = serviceUuids.find((u) => this.normalizeUuid(u) === preferredService);
    const preferredServiceChars = preferredServiceUuid ? serviceChars[preferredServiceUuid] ?? [] : [];

    const preferredServiceWritable = preferredServiceUuid
      ? allWritable.filter((w) => this.normalizeUuid(w.serviceUuid) === preferredService)
      : [];
    const preferredServiceNotifiable = preferredServiceUuid
      ? allNotifiable.filter((n) => this.normalizeUuid(n.serviceUuid) === preferredService)
      : [];

    const preferredWrite =
      (preferredServiceUuid
        ? this.pickPreferredUuid(preferredServiceWritable.map((x) => x.c), CHAR_WRITE_COMMAND_UUID)
        : undefined) ?? undefined;
    const writePick =
      (preferredWrite
        ? preferredServiceWritable.find((x) => x.c.uuid === preferredWrite.uuid)
        : undefined) ?? preferredServiceWritable[0] ?? allWritable[0];

    const preferredNotify =
      (preferredServiceUuid
        ? this.pickPreferredUuid(preferredServiceNotifiable.map((x) => x.c), CHAR_NOTIFY_RESPONSE_UUID)
        : undefined) ?? undefined;
    const notifyPick =
      (preferredNotify
        ? preferredServiceNotifiable.find((x) => x.c.uuid === preferredNotify.uuid)
        : undefined) ??
      preferredServiceNotifiable[0] ??
      allNotifiable[0];

    const preferredStatus =
      (preferredServiceUuid
        ? this.pickPreferredUuid(preferredServiceNotifiable.map((x) => x.c), CHAR_NOTIFY_STATUS_UUID)
        : undefined) ?? undefined;
    const statusPick =
      (preferredStatus
        ? preferredServiceNotifiable.find((x) => x.c.uuid === preferredStatus.uuid)
        : undefined) ??
      allNotifiable.find((n) => n.c.uuid !== notifyPick.c.uuid);

    const preferredAck = preferredServiceUuid
      ? (this.pickPreferredUuid(preferredServiceChars, CHAR_WRITE_ACK_UUID) as Characteristic | undefined)
      : undefined;
    const ackPick = preferredAck
      ? { serviceUuid: preferredServiceUuid!, c: preferredAck }
      : undefined;

    const extraNotifies = allNotifiable
      .filter(
        (n) =>
          !(n.serviceUuid === notifyPick.serviceUuid && n.c.uuid === notifyPick.c.uuid) &&
          !(statusPick && n.serviceUuid === statusPick.serviceUuid && n.c.uuid === statusPick.c.uuid)
      )
      .slice(0, this.maxExtraNotifyMonitors)
      .map((n) => ({ serviceUuid: n.serviceUuid, uuid: n.c.uuid }));

    const profile = {
      writeServiceUuid: writePick.serviceUuid,
      writeUuid: writePick.c.uuid,
      notifyServiceUuid: notifyPick.serviceUuid,
      notifyUuid: notifyPick.c.uuid,
      statusServiceUuid: statusPick?.serviceUuid,
      statusUuid: statusPick?.c.uuid,
      ackServiceUuid: ackPick?.serviceUuid,
      ackUuid: ackPick?.c.uuid,
      extraNotifies,
    };

    this.log(
      `Using GATT profile write=${profile.writeServiceUuid}/${profile.writeUuid}, notify=${profile.notifyServiceUuid}/${profile.notifyUuid}, status=${
        profile.statusServiceUuid && profile.statusUuid
          ? `${profile.statusServiceUuid}/${profile.statusUuid}`
          : '(none)'
      }, ack=${
        profile.ackServiceUuid && profile.ackUuid ? `${profile.ackServiceUuid}/${profile.ackUuid}` : '(none)'
      }, extras=${profile.extraNotifies?.length ?? 0}`,
      'info'
    );

    this.log(
      `Discovery summary: services=${serviceUuids.length}, writable=${allWritable.length}, notifiable=${allNotifiable.length}, hasFF01=${
        Boolean(knownWrite)
      }, hasFF02=${Boolean(knownNotify)}`,
      'info'
    );

    return profile;
  }

  private async verifyKeystreamGatt(device: Device): Promise<void> {
    const requiredService = this.normalizeUuid(KEYSTREAM_SERVICE_UUID);
    const requiredNotify = this.normalizeUuid(CHAR_NOTIFY_RESPONSE_UUID);
    const requiredWrite = this.normalizeUuid(CHAR_WRITE_COMMAND_UUID);
    const optionalStatus = this.normalizeUuid(CHAR_NOTIFY_STATUS_UUID);

    const services = await device.services();
    const serviceUuids = services.map((s) => s.uuid);
    const hasService = serviceUuids.some((u) => this.normalizeUuid(u) === requiredService);

    if (!hasService) {
      this.log(
        `Expected keySTREAM service not found: ${KEYSTREAM_SERVICE_UUID}. Found services: ${serviceUuids.join(', ') || '(none)'}`,
        'error'
      );
      throw new Error('keySTREAM GATT service not found on device');
    }

    let chars = [] as Characteristic[];
    try {
      chars = await device.characteristicsForService(KEYSTREAM_SERVICE_UUID);
    } catch (e: any) {
      this.log(`Failed to enumerate characteristics for service ${KEYSTREAM_SERVICE_UUID}: ${e?.message || String(e)}`, 'error');
      throw new Error('Unable to enumerate keySTREAM characteristics');
    }

    const charUuids = chars.map((c) => c.uuid);
    const notifyChar = chars.find((c) => this.normalizeUuid(c.uuid) === requiredNotify);
    const writeChar = chars.find((c) => this.normalizeUuid(c.uuid) === requiredWrite);
    const statusChar = chars.find((c) => this.normalizeUuid(c.uuid) === optionalStatus);
    const hasNotify = !!notifyChar;
    const hasWrite = !!writeChar;
    const hasStatus = !!statusChar;

    if (!hasNotify || !hasWrite) {
      this.log(
        `Missing required characteristic(s). Need notify=${CHAR_NOTIFY_RESPONSE_UUID}, write=${CHAR_WRITE_COMMAND_UUID}. Found: ${charUuids.join(', ') || '(none)'}`,
        'error'
      );
      throw new Error('keySTREAM required characteristics not found on device');
    }

    const canNotify = !!notifyChar?.isNotifiable || !!notifyChar?.isIndicatable;
    if (!canNotify) {
      this.log(
        `Response characteristic ${CHAR_NOTIFY_RESPONSE_UUID} is not notifiable/indicatable (isNotifiable=${String(
          notifyChar?.isNotifiable
        )}, isIndicatable=${String(notifyChar?.isIndicatable)})`,
        'error'
      );
      throw new Error('Response characteristic cannot be monitored');
    }

    if (!hasStatus) {
      this.log(`Status characteristic ${CHAR_NOTIFY_STATUS_UUID} not found; status updates will be disabled`, 'warning');
    } else {
      const canStatusNotify = !!statusChar?.isNotifiable || !!statusChar?.isIndicatable;
      if (!canStatusNotify) {
        this.log(
          `Status characteristic ${CHAR_NOTIFY_STATUS_UUID} is not notifiable/indicatable; status updates will be disabled`,
          'warning'
        );
      }
    }
  }

  async connect(deviceId: string): Promise<boolean> {
    try {
      this.log('Connecting to device...', 'info');

      // Clear any stale buffered data from prior sessions.
      this.responseBuffer = '';
      this.responseBytes = Buffer.alloc(0);
      this.clearResponseFlushTimer();

      // Connect to the device
      let device = await bleManager.connectToDevice(deviceId, {
        requestMTU: 247, // Request larger MTU for better throughput
      });

      // Some Android stacks report a non-default MTU on the Device object even when
      // the MTU request didn't truly take effect. To avoid oversized writes, we
      // re-request MTU here and only trust the result if it succeeds.
      try {
        device = await bleManager.requestMTUForDevice(device.id, 247);
      } catch (e: any) {
        this.log(`MTU request failed (continuing with fallback chunking): ${e?.message || String(e)}`, 'warning');
      }

      // IMPORTANT:
      // We previously forced 20-byte chunking for stability against Android GATT_INVALID_ATTR_LEN (status=13),
      // but if the MTU is reliably negotiated to 247, a 200-byte chunk will fit well within the MTU-3 limit.
      const negotiatedMtu = (device as any)?.mtu;
      this.maxChunkSize = 200; // Increased to 200 as requested
      this.log(
        `Negotiated MTU=${typeof negotiatedMtu === 'number' ? negotiatedMtu : '(unknown)'}; using maxChunkSize=${this.maxChunkSize} bytes`,
        'info'
      );

      this.connectedDevice = device;
      this._isConnected = true;

      this.log('Device connected, discovering services...', 'success');

      // Discover services and characteristics
      await device.discoverAllServicesAndCharacteristics();

      try {
        this.gattProfile = await this.resolveGattProfile(device);
      } catch (e: any) {
        this.log(`GATT profile resolution failed: ${e?.message || String(e)}`, 'error');
        this._isConnected = false;
        this.connectedDevice = null;
        try {
          await bleManager.cancelDeviceConnection(device.id);
        } catch {
          // ignore
        }
        this.callbacks.onError?.('Device GATT profile not compatible');
        return false;
      }

      this.log('Services discovered, setting up notifications...', 'success');

      // Setup notifications for response characteristic (required)
      try {
        if (!this.gattProfile) throw new Error('GATT profile not set');
        this.primaryNotifySource = `${this.gattProfile.notifyServiceUuid}/${this.gattProfile.notifyUuid}`;
        await device.monitorCharacteristicForService(
          this.gattProfile.notifyServiceUuid,
          this.gattProfile.notifyUuid,
          (error, characteristic) => {
            if (error) {
              this.log(`Response notification error: ${error.message}`, 'error');
              return;
            }
            this.handleResponseNotification(characteristic, this.primaryNotifySource!);
          }
        );
      } catch (e: any) {
        this.log(`Failed to start response notifications: ${e?.message || String(e)}`, 'error');
        throw e;
      }

      // Setup notifications for status characteristic (optional)
      try {
        if (this.gattProfile?.statusServiceUuid && this.gattProfile?.statusUuid) {
          await device.monitorCharacteristicForService(
            this.gattProfile.statusServiceUuid,
            this.gattProfile.statusUuid,
            (error, characteristic) => {
              if (error) {
                this.log(`Status notification error: ${error.message}`, 'error');
                return;
              }
              this.handleStatusNotification(characteristic);
            }
          );
        }
      } catch (e: any) {
        this.log(`Failed to start status notifications (continuing): ${e?.message || String(e)}`, 'warning');
      }

      // Setup notifications for any extra notifiable characteristics (debug/fallback)
      try {
        const extras = this.gattProfile?.extraNotifies ?? [];
        for (const extra of extras) {
          try {
            await device.monitorCharacteristicForService(
              extra.serviceUuid,
              extra.uuid,
              (error, characteristic) => {
                if (error) {
                  this.log(`Extra notification error (${extra.serviceUuid}/${extra.uuid}): ${error.message}`, 'warning');
                  return;
                }
                this.handleResponseNotification(characteristic, `${extra.serviceUuid}/${extra.uuid}`);
              }
            );
          } catch (e: any) {
            this.log(
              `Failed to monitor extra notify characteristic (${extra.serviceUuid}/${extra.uuid}): ${
                e?.message || String(e)
              }`,
              'warning'
            );
          }
        }
      } catch (e: any) {
        this.log(`Failed to start extra notifications (continuing): ${e?.message || String(e)}`, 'warning');
      }

      // Monitor device disconnection
      device.onDisconnected((error) => {
        this.log('Device disconnected', 'warning');
        this._isConnected = false;
        this.connectedDevice = null;
        this.gattProfile = null;
        this.callbacks.onDisconnected?.(error?.message);
      });

      this.log('BLE connection established successfully', 'success');
      this.callbacks.onConnected?.(device);

      return true;
    } catch (error: any) {
      this.log(`Connection failed: ${error.message}`, 'error');
      this.callbacks.onError?.(error.message);
      this._isConnected = false;
      if (this.connectedDevice) {
        try {
          await bleManager.cancelDeviceConnection(this.connectedDevice.id);
        } catch {
          // ignore
        }
      }
      this.connectedDevice = null;
      this.gattProfile = null;
      return false;
    }
  }

  async disconnect(): Promise<void> {
    if (this.connectedDevice) {
      try {
        await bleManager.cancelDeviceConnection(this.connectedDevice.id);
        this.log('Disconnected from device', 'info');
      } catch (error: any) {
        this.log(`Disconnect error: ${error.message}`, 'error');
      }
    }

    this.clearResponseFlushTimer();
    this.responseBuffer = '';
    this.responseBytes = Buffer.alloc(0);
    this._isConnected = false;
    this.connectedDevice = null;
    this.gattProfile = null;
  }

  /** Read the current RSSI from the connected device. Returns null if unavailable. */
  async readRSSI(): Promise<number | null> {
    if (!this.connectedDevice || !this._isConnected) return null;
    try {
      const dev = await this.connectedDevice.readRSSI();
      return (dev as any)?.rssi ?? null;
    } catch {
      return null;
    }
  }

  /**
   * Send a binary request frame over the write characteristic.
   * Frame format: [opcode:1][payloadLen:2 LE][payload]
   */
  async sendBinary(opcode: number, payload?: Uint8Array): Promise<boolean> {
    if (!this._isConnected || !this.connectedDevice) {
      this.log('Cannot send binary: device not connected', 'error');
      return false;
    }

    if (!this.gattProfile) {
      this.log('Cannot send binary: GATT profile not set', 'error');
      return false;
    }

    try {
      const frame = encodeRequestFrame({ opcode, payload: payload ?? new Uint8Array() });
      this.log(`Sending binary frame: opcode=0x${opcode.toString(16)}, payload=${frame.length - 3} bytes`, 'info');

      // To keep BLE stacks stable, avoid very large single writes.
      // Chunk proactively when the frame exceeds our per-write chunk size.
      if (frame.length <= this.maxChunkSize) {
        const base64Frame = frame.toString('base64');
        await this.connectedDevice.writeCharacteristicWithResponseForService(
          this.gattProfile.writeServiceUuid,
          this.gattProfile.writeUuid,
          base64Frame
        );
        this.log(`Sent binary frame in 1 write (${frame.length} bytes)`, 'success');
      } else {
        const chunks = this.chunkData(frame);
        this.log(`Frame is ${frame.length} bytes; sending ${chunks.length} chunk(s) (maxChunkSize=${this.maxChunkSize})...`, 'info');

        for (let i = 0; i < chunks.length; i++) {
          const base64Chunk = chunks[i].toString('base64');
          try {
            await this.connectedDevice.writeCharacteristicWithResponseForService(
              this.gattProfile.writeServiceUuid,
              this.gattProfile.writeUuid,
              base64Chunk
            );
          } catch (e: any) {
            this.log(
              `Write-with-response failed for binary chunk ${i + 1}/${chunks.length}; retrying without-response (${e?.message || String(
                e
              )})`,
              'warning'
            );
            await this.connectedDevice.writeCharacteristicWithoutResponseForService(
              this.gattProfile.writeServiceUuid,
              this.gattProfile.writeUuid,
              base64Chunk
            );
          }

          this.log(`Sent chunk ${i + 1}/${chunks.length}`, 'info');
          await this.waitForAck();
        }
      }

      this.log('Binary frame sent successfully', 'success');
      return true;
    } catch (error: any) {
      this.log(`Send binary error: ${error?.message || String(error)}`, 'error');
      this.callbacks.onError?.(error?.message || String(error));
      return false;
    }
  }

  private handleResponseNotification(characteristic: Characteristic | null, source?: string) {
    if (!characteristic?.value) return;

    let chunkBytes: Buffer;
    try {
      const base64Data = characteristic.value;
      chunkBytes = Buffer.from(base64Data, 'base64');
    } catch (e: any) {
      const msg = e?.message || String(e);
      this.log(`Failed to decode notify base64 from ${source || '(unknown source)'}: ${msg}`, 'error');
      this.callbacks.onError?.(`BLE notify decode error: ${msg}`);
      return;
    }

    // In binary mode, only accept bytes from the primary response characteristic.
    // Mixing multiple notify characteristics into the same byte stream can corrupt
    // frame boundaries and lead to decoder desync / app crashes.
    if (this.callbacks.onBinaryFrameReceived) {
      if (this.primaryNotifySource && source && source !== this.primaryNotifySource) {
        this.log(`Ignoring non-primary notify bytes from ${source} (${chunkBytes.length} bytes)`, 'warning');
        return;
      }
    }

    // Always buffer bytes for binary frame decoding.
    try {
      this.responseBytes = Buffer.concat([this.responseBytes, chunkBytes]);
    } catch (e: any) {
      const msg = e?.message || String(e);
      this.log(`Failed to append notify bytes to buffer (${chunkBytes.length} bytes): ${msg}`, 'error');
      this.callbacks.onError?.(`BLE buffer append error: ${msg}`);
      this.responseBytes = Buffer.alloc(0);
      return;
    }

    if (this.responseBytes.length > this.maxResponseBytesBuffered) {
      this.log(
        `Binary response buffer overflow (${this.responseBytes.length} bytes). Clearing buffer to avoid OOM; likely notify stream corruption.`,
        'warning'
      );
      this.responseBytes = Buffer.alloc(0);
      return;
    }

    this.drainBinaryFrames();

    // If binary mode is active, do not attempt to interpret notifications as UTF-8/JSON.
    // This avoids spurious "terminator" logs when binary bytes happen to contain 0x00 or '\n'.
    if (this.callbacks.onBinaryFrameReceived) {
      return;
    }

    const data = chunkBytes.toString('utf-8');
    const prefix = source ? `[${source}] ` : '';
    this.log(`${prefix}Received response chunk: ${data.substring(0, 80)}...`, 'info');

    // Accumulate chunks
    this.responseBuffer += data;

    // Prefer JSON-boundary detection over newline terminators.
    if (data.includes('\n') || data.includes('\0')) {
      this.drainResponseBuffer('terminator');
      return;
    }

    this.drainResponseBuffer('chunk');
  }

  private drainBinaryFrames() {
    if (!this.callbacks.onBinaryFrameReceived) return;
    if (!this.responseBytes.length) return;

    try {
      const { frames, remainder } = tryDecodeResponseFrames(this.responseBytes);
      if (!frames.length) return;

      this.responseBytes = Buffer.from(remainder);
      for (const frame of frames) {
        this.log(
          `Binary frame received: opcode=0x${frame.opcode.toString(16)}, status=${frame.status}, payload=${frame.payload.length} bytes`,
          'success'
        );

        // Helpful debug for activation flow: exchange response payload is
        // [rot2ksLenLE:2][rot2ks bytes...]. We expect rot2ksLen=213 in your logs.
        if (frame.opcode === KTA_OPCODE.EXCHANGE_MESSAGE && frame.payload.length >= 2) {
          try {
            const b = Buffer.from(frame.payload);
            const rot2ksLen = b.readUInt16LE(0);
            this.log(
              `[RX] EXCHANGE_MESSAGE rot2ksLen=${rot2ksLen} (payload=${frame.payload.length} bytes)`,
              rot2ksLen === 213 ? 'success' : 'info'
            );
          } catch {
            // ignore
          }
        }

        try {
          this.callbacks.onBinaryFrameReceived?.(frame);
        } catch (e: any) {
          const msg = e?.message || String(e);
          this.log(`onBinaryFrameReceived handler threw: ${msg}`, 'error');
          this.callbacks.onError?.(`Binary frame handler error: ${msg}`);
        }
      }
    } catch (e: any) {
      const msg = e?.message || String(e);
      this.log(`Binary frame decode failure; clearing buffer: ${msg}`, 'error');
      this.callbacks.onError?.(`Binary frame decode error: ${msg}`);
      this.responseBytes = Buffer.alloc(0);
      return;
    }
  }

  private handleStatusNotification(characteristic: Characteristic | null) {
    if (!characteristic?.value) return;

    let status: string;
    try {
      const base64Data = characteristic.value;
      status = Buffer.from(base64Data, 'base64').toString('utf-8');
    } catch (e: any) {
      const msg = e?.message || String(e);
      this.log(`Failed to decode status notify: ${msg}`, 'warning');
      return;
    }

    this.log(`Status update: ${status}`, 'info');
    this.callbacks.onStatusUpdate?.(status);
  }

  async sendCommand(command: string): Promise<boolean> {
    if (!this.isConnected || !this.connectedDevice) {
      this.log('Cannot send command: device not connected', 'error');
      return false;
    }

    if (!this.gattProfile) {
      this.log('Cannot send command: GATT profile not set', 'error');
      return false;
    }

    try {
      const framed = command.endsWith('\n') ? command : `${command}\n`;
      this.log(`Sending command: ${framed.substring(0, 50)}...`, 'info');

      // Convert command to buffer
      const buffer = Buffer.from(framed, 'utf-8');
      
      // Split into chunks if needed
      const chunks = this.chunkData(buffer);
      
      this.log(`Sending ${chunks.length} chunk(s) (maxChunkSize=${this.maxChunkSize})...`, 'info');

      for (let i = 0; i < chunks.length; i++) {
        const chunk = chunks[i];
        const base64Chunk = chunk.toString('base64');

        // Some firmwares expose a write characteristic that is "without response" only.
        // Try with-response first (better reliability), then fall back to without-response.
        try {
          await this.connectedDevice.writeCharacteristicWithResponseForService(
            this.gattProfile.writeServiceUuid,
            this.gattProfile.writeUuid,
            base64Chunk
          );
        } catch (e: any) {
          this.log(
            `Write-with-response failed for chunk ${i + 1}/${chunks.length}; retrying without-response (${e?.message || String(
              e
            )})`,
            'warning'
          );
          await this.connectedDevice.writeCharacteristicWithoutResponseForService(
            this.gattProfile.writeServiceUuid,
            this.gattProfile.writeUuid,
            base64Chunk
          );
        }

        this.log(`Sent chunk ${i + 1}/${chunks.length}`, 'info');

        // Wait for ACK (simplified - should implement proper ACK/NACK protocol)
        await this.waitForAck();
      }

      this.log('Command sent successfully', 'success');
      return true;
    } catch (error: any) {
      this.log(`Send command error: ${error.message}`, 'error');
      this.callbacks.onError?.(error.message);
      return false;
    }
  }

  private chunkData(buffer: Buffer): Buffer[] {
    const chunks: Buffer[] = [];
    let offset = 0;

    while (offset < buffer.length) {
      const chunkSize = Math.min(this.maxChunkSize, buffer.length - offset);
      const chunk = buffer.slice(offset, offset + chunkSize);
      chunks.push(chunk);
      offset += chunkSize;
    }

    return chunks;
  }

  private async waitForAck(): Promise<void> {
    // Simplified ACK handling - should implement proper ACK/NACK protocol
    // In a real implementation, you would monitor CHAR_WRITE_ACK_UUID for ACK/NACK responses
    return new Promise((resolve) => {
      setTimeout(resolve, 120); // Delay between chunks to keep BLE stack stable
    });
  }

  async sendAck(ackValue: number = 1): Promise<boolean> {
    if (!this._isConnected || !this.connectedDevice) {
      return false;
    }

    if (!this.gattProfile?.ackUuid) {
      return true;
    }

    try {
      const ackBuffer = Buffer.from([ackValue]);
      const base64Ack = ackBuffer.toString('base64');

      await this.connectedDevice.writeCharacteristicWithoutResponseForService(
        this.gattProfile.ackServiceUuid ?? this.gattProfile.writeServiceUuid,
        this.gattProfile.ackUuid,
        base64Ack
      );

      return true;
    } catch (error) {
      return false;
    }
  }

  isDeviceConnected(): boolean {
      return this._isConnected;
  }

  // Backwards-compatible API: some older bundles call `bleTransport.isConnected()`
  // as a function. Provide a public method to avoid "isConnected is not a function".
  public isConnected(): boolean {
    return this._isConnected;
  }

  getConnectedDevice(): Device | null {
    return this.connectedDevice;
  }

  destroy() {
    this.disconnect();
    // Do not destroy the shared BleManager here.
  }
}
