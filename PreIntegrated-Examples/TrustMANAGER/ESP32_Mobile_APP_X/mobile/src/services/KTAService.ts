import { Buffer } from 'buffer';
import { BLETransport, BLETransportCallbacks } from './BLETransport';
import AsyncStorage from '@react-native-async-storage/async-storage';

import {
  KTA_OPCODE,
  KTA_KS_STATUS,
  decodeKtaExchangeMessageResponsePayload,
  decodeKtaKeyStreamStatusResponsePayload,
  encodeKtaExchangeMessagePayload,
  encodeKtaKeyStreamStatusPayload,
  encodeKtaSetDeviceInfoPayload,
  encodeKtaStartupPayload,
  type KtaBinaryResponseFrame,
} from './KTAProtocol';
import { KeyStreamClient } from './KeyStreamClient';
import { KtaConfig } from './KtaConfig';

export interface KTAConfig {
  authToken: string;
  fleetProfileUID: string;
  deviceProfileUID?: string;
  serialNumber?: string;
  apiEndpoint?: string;
}

export type KTAState =
  | 'idle'
  | 'initializing'
  | 'ready'
  | 'provisioning'
  | 'provisioned'
  | 'error';

export interface KTAServiceCallbacks {
  onStateChange?: (state: KTAState) => void;
  onLog?: (message: string, type: 'info' | 'success' | 'error' | 'warning' | 'outbound' | 'inbound') => void;
  onProgress?: (progress: number, message: string) => void;
  /** Called when the real device UID (RoT Public UID) is extracted from the ICPP activation message. */
  onDeviceUID?: (uid: string) => void;
  /** Called when BLE connection status changes (true=connected, false=disconnected). */
  onBLEConnectionChange?: (connected: boolean) => void;
}

export class KTAService {
  private bleTransport: BLETransport;
  private config: KTAConfig | null = null;
  private state: KTAState = 'idle';
  private callbacks: KTAServiceCallbacks;
  private keyStreamClient: KeyStreamClient = new KeyStreamClient();
  private readonly initTimeoutMs: number = 40000;
  private readonly exchangeTimeoutMs: number = 100000;
  private readonly keyStreamTimeoutMs: number = 15000;
  private lastConnectedDeviceId: string | null = null;
  private autoReconnecting: boolean = false;
  private userDisconnected: boolean = false;

  private pendingWaits = new Map<
    number,
    Array<{
      resolve: (frame: KtaBinaryResponseFrame) => void;
      reject: (error: Error) => void;
      timer: any;
    }>
  >();

  private flow = {
    initDone: false,
    startupDone: false,
    deviceInfoDone: false,
  };

  private exchange = {
    active: false,
    queue: Promise.resolve() as Promise<void>,
    lastRot2ksB64: null as string | null,
    step: 0,
  };

  /** 0x06 recovery: track consecutive ERROR statuses on empty EXCHANGE polls */
  private consecutive0x06Count: number = 0;
  private rebootstrapAttempts: number = 0;
  private lastRebootstrapTime: number = 0;
  private readonly max0x06BeforeRebootstrap: number = 8;
  private readonly maxRebootstrapAttempts: number = 5;
  private readonly rebootstrapCooldownMs: number = 30000;

  /** Keepalive: send STATUS (0x08) every 30s when idle */
  private keepaliveInterval: ReturnType<typeof setInterval> | null = null;
  private readonly keepaliveIntervalMs: number = 30000;

  /** Saved ks2kta from server when BLE disconnects mid-exchange, so we can resume. */
  private resumeKs2kta: Uint8Array | null = null;
  /** Exchange step to resume from after BLE reconnect. */
  private resumeExchangeStep: number = 0;
  /** Whether we were actively in the exchange loop when BLE dropped. */
  private wasInExchangeLoop: boolean = false;

  private deviceUIDReported: boolean = false;

  constructor(callbacks: KTAServiceCallbacks = {}) {
    this.callbacks = callbacks;

    const bleCallbacks: BLETransportCallbacks = {
      onConnected: () => {
        this.callbacks.onBLEConnectionChange?.(true);
        this.log('BLE connected', 'success');
        if (this.wasInExchangeLoop) {
          this.log('Resuming provisioning from where it stopped', 'info');
        }
        this.runProvisioningFlow();
      },
      onDisconnected: (error) => {
        this.callbacks.onBLEConnectionChange?.(false);
        this.log('BLE disconnected', 'warning');

        // If we were in the exchange loop, save state so we can resume after reconnect
        if (this.exchange.active) {
          this.wasInExchangeLoop = true;
          this.resumeExchangeStep = this.exchange.step;
          this.log(`Exchange interrupted at step ${this.exchange.step} — will resume after reconnect`, 'warning');
        }

        this.clearPendingWaits('BLE disconnected');

        if (!this.userDisconnected) {
          // Auto-reconnect on unexpected disconnects
          this.log('Auto-reconnecting...', 'info');
          this.setState('initializing');
          this.attemptAutoReconnect();
        } else {
          this.wasInExchangeLoop = false;
          this.resumeKs2kta = null;
          this.setState('error');
        }
      },
      onBinaryFrameReceived: (frame) => {
        this.routeBinaryFrame(frame);
      },
      onStatusUpdate: (status) => {
      },
      onError: (error) => {
        this.log('BLE error', 'error');
      },
      onLog: (message, type) => {
        // BLETransport only forwards errors now; pass them through
        if (type === 'error') this.log(message, type);
      },
    };

    this.bleTransport = new BLETransport(bleCallbacks);

    // Runtime guard: if some stale bundle mutated bleTransport.isConnected to a boolean,
    // ensure a callable function exists to avoid "isConnected is not a function".
    const bt: any = this.bleTransport as any;
    if (typeof bt.isConnected !== 'function') {
      Object.defineProperty(bt, 'isConnected', {
        value: () => (typeof bt._isConnected === 'boolean' ? bt._isConnected : false),
        writable: true,
        configurable: true,
      });
    }
  }

  // Mirror the reference values used in miniclientdev/DEV/APPS/microchip/sam_e54/keystream_connect/ktaFieldMgntHook.c
  // and miniclientdev/DEV/APPS/microchip/sam_e54/keystream_connect/ktaConfig.h.
  private readonly ktaRef = {
    l1SegSeed: KtaConfig.l1SegSeed,
    contextSerialNum: Uint8Array.from([0x11, 0x22, 0x33, 0x04, 0x05, 0x06, 0x07, 0x08]),
    contextVersion: Uint8Array.from([0x22, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x05]),
    deviceSerialNum: Uint8Array.from([0x22, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09]),
    defaultDeviceProfilePublicUid: KtaConfig.devicePublicUid,
  };

  /**
   * Extract the RoT Public UID from the ICPP message header (bytes 10-17).
   * This is the device's unique identifier on keySTREAM, read from the ATECC608 secure element.
   * Called once per provisioning flow from the first rot2ks message.
   */
  private extractAndReportDeviceUID(rot2ksMsg: Uint8Array): void {
    if (this.deviceUIDReported) return; // already reported
    if (rot2ksMsg.length < 21) return; // too short for ICPP header

    // ICPP header layout (21 bytes):
    //   [0]     = protocol version
    //   [1]     = crypto/msg type
    //   [2-9]   = transaction ID (8 bytes)
    //   [10-17] = RoT Public UID (8 bytes) ← device's unique keySTREAM ID
    //   [18]    = RoT KeySet ID
    //   [19-20] = message length
    const rotPublicUID = rot2ksMsg.slice(10, 18);
    const hexStr = Array.from(rotPublicUID).map(b => b.toString(16).padStart(2, '0')).join('');

    // Skip if all zeros (not yet assigned)
    if (rotPublicUID.every(b => b === 0)) return;

    this.deviceUIDReported = true;
    this.callbacks.onDeviceUID?.(hexStr);
  }

  private log(message: string, type: 'info' | 'success' | 'error' | 'warning' | 'outbound' | 'inbound' = 'info') {
    this.callbacks.onLog?.(message, type);
  }

  private setState(newState: KTAState) {
    if (this.state !== newState) {
      this.state = newState;
      this.callbacks.onStateChange?.(newState);
    }
  }

  private static ktaStatusName(status: number): string {
    // Must match TKStatus in firmware (see firmware/esp-idf-keystream-ble/main/main.c kta_status_str)
    switch (status | 0) {
      case 0:
        return 'OK';
      case 1:
        return 'PARAMETER';
      case 2:
        return 'DATA';
      case 3:
        return 'STATE';
      case 4:
        return 'MEMORY';
      case 5:
        return 'MISSING';
      case 6:
        return 'ERROR';
      case 7:
        return 'DECRYPTION';
      case 8:
        return 'PERSONALIZED';
      case 9:
        return 'NOT_SUPPORTED';
      default:
        return 'UNKNOWN';
    }
  }

  async setConfig(config: KTAConfig): Promise<void> {
    this.config = config;
    await AsyncStorage.setItem('kta_config', JSON.stringify(config));
  }

  async loadConfig(): Promise<KTAConfig | null> {
    try {
      const configStr = await AsyncStorage.getItem('kta_config');
      if (configStr) {
        this.config = JSON.parse(configStr);
        return this.config;
      }
    } catch (error) {
      this.log('Failed to load config', 'error');
    }
    return null;
  }

  async connectToDevice(deviceId: string): Promise<boolean> {
    this.lastConnectedDeviceId = deviceId;
    this.userDisconnected = false;
    this.autoReconnecting = false;
    const connected = await this.bleTransport.connect(deviceId);

    if (!connected) {
      this.setState('error');
    }

    return connected;
  }

  async disconnect(): Promise<void> {
    this.userDisconnected = true;
    this.autoReconnecting = false;
    this.deviceUIDReported = false;
    this.wasInExchangeLoop = false;
    this.resumeKs2kta = null;
    this.resumeExchangeStep = 0;
    this.stopPeriodicServerCheck(); // Stop polling timer
    this.stopKeepalive(); // Stop BLE keepalive
    this.clearPendingWaits('Disconnect requested');
    await this.bleTransport.disconnect();
    this.setState('idle');
    this.flow = { initDone: false, startupDone: false, deviceInfoDone: false };
    this.exchange = { active: false, queue: Promise.resolve(), lastRot2ksB64: null, step: 0 };
  }

  /** Read the current RSSI from the connected BLE device. */
  async readRSSI(): Promise<number | null> {
    return this.bleTransport.readRSSI();
  }

  /**
   * Auto-reconnect after unexpected BLE disconnect.
   * Retries with increasing delays, then keeps retrying every 15s indefinitely
   * until successfully reconnected or user manually disconnects.
   */
  private async attemptAutoReconnect(): Promise<void> {
    if (this.autoReconnecting || this.userDisconnected || !this.lastConnectedDeviceId) return;
    this.autoReconnecting = true;

    // After a long disconnect (>10s), device likely rebooted — clear stale resume state
    this.wasInExchangeLoop = false;
    this.resumeKs2kta = null;
    this.resumeExchangeStep = 0;

    const delays = [3000, 5000, 8000, 10000, 15000];
    let attempt = 0;
    while (!this.userDisconnected) {
      attempt++;
      const delay = attempt <= delays.length ? delays[attempt - 1] : 15000;
      this.log(`Reconnect attempt ${attempt} in ${delay / 1000}s...`, 'info');
      await new Promise(resolve => setTimeout(() => resolve(undefined), delay));
      if (this.userDisconnected) break;

      try {
        const ok = await this.bleTransport.connect(this.lastConnectedDeviceId!);
        if (ok) {
          this.log('Reconnected', 'success');
          this.autoReconnecting = false;
          return;
        }
      } catch (e: any) {
        // Only log first few failures to avoid spam
        if (attempt <= 3) {
          this.log(`Reconnect attempt ${attempt} failed`, 'warning');
        }
      }
    }

    this.autoReconnecting = false;
  }

  private clearPendingWaits(reason: string) {
    for (const [, waits] of this.pendingWaits) {
      for (const w of waits) {
        try {
          clearTimeout(w.timer);
        } catch {
          // ignore
        }
        w.reject(new Error(`Cancelled: ${reason}`));
      }
    }
    this.pendingWaits.clear();
  }

  private waitForFrame(opcode: number, timeoutMs: number): Promise<KtaBinaryResponseFrame> {
    return new Promise<KtaBinaryResponseFrame>((resolve, reject) => {
      const waits = this.pendingWaits.get(opcode) ?? [];

      const waitItem: {
        resolve: (frame: KtaBinaryResponseFrame) => void;
        reject: (error: Error) => void;
        timer: any;
      } = {
        resolve,
        reject,
        timer: null,
      };

      waitItem.timer = setTimeout(() => {
        const current = this.pendingWaits.get(opcode) ?? [];
        const idx = current.indexOf(waitItem);
        if (idx >= 0) current.splice(idx, 1);
        if (!current.length) this.pendingWaits.delete(opcode);
        reject(new Error(`Timeout waiting for opcode=0x${opcode.toString(16)} (${timeoutMs}ms)`));
      }, timeoutMs);

      waits.push(waitItem);
      this.pendingWaits.set(opcode, waits);
    });
  }

  private routeBinaryFrame(frame: KtaBinaryResponseFrame) {
    // If an async flow is awaiting this opcode, resolve the oldest waiter.
    const waits = this.pendingWaits.get(frame.opcode);
    if (!waits || !waits.length) {
      // Keep logs minimal: only surface unsolicited error frames.
      if (frame.status !== 0) {
        // unsolicited error frame; ignore
      }
      return;
    }

    const w = waits.shift()!;
    try {
      clearTimeout(w.timer);
    } catch {
      // ignore
    }
    if (!waits.length) this.pendingWaits.delete(frame.opcode);
    w.resolve(frame);
  }

  private async sendAndWait(
    opcode: number,
    payload: Uint8Array | undefined,
    timeoutMs: number
  ): Promise<KtaBinaryResponseFrame> {
    // Arm the wait first to avoid a race where the notification arrives very quickly.
    const waiting = this.waitForFrame(opcode, timeoutMs);
    const sent = await this.bleTransport.sendBinary(opcode, payload);
    if (!sent) {
      this.clearPendingWaits(`Send failed opcode=0x${opcode.toString(16)}`);
      throw new Error(`Failed to send opcode=0x${opcode.toString(16)}`);
    }
    return await waiting;
  }

  private async runProvisioningFlow(): Promise<void> {
    // Enforce strict sequencing: send -> await device response -> next step.
    try {
      if (!this.config) {
        this.log('Configuration missing. Go back and set up.', 'error');
        this.setState('error');
        return;
      }

      // Reset UID reporting so it gets extracted fresh on each flow
      this.deviceUIDReported = false;

      this.exchange.active = false;
      this.exchange.lastRot2ksB64 = null;
      this.exchange.step = 0;

      this.log(`Fleet: ${this.config.fleetProfileUID || '(none)'}`, 'info');

      // ---- INITIALIZE ----
      this.setState('initializing');
      this.flow = { initDone: false, startupDone: false, deviceInfoDone: false };
      this.log('[1/4] Initializing secure element (ATECC608)...', 'info');
      this.callbacks.onProgress?.(10, 'Initializing...');

      let initFrame: KtaBinaryResponseFrame | null = null;
      for (let attempt = 1; attempt <= 2; attempt++) {
        try {
          initFrame = await this.sendAndWait(KTA_OPCODE.INITIALIZE, undefined, this.initTimeoutMs);
          break;
        } catch (e: any) {
          if (attempt < 2) {
            continue;
          }
          throw e;
        }
      }

      if (!initFrame) throw new Error('ktaInitialize() failed to produce a response');
      if (initFrame.status !== 0) {
        // STATUS 0x03 (STATE) or 0x06 (ERROR) from INITIALIZE means the device
        // is already initialized — treat as OK and continue the flow.
        if (initFrame.status === 0x03 || initFrame.status === 0x06) {
          this.log(`[1/4] Device already initialized (status=${KTAService.ktaStatusName(initFrame.status)}), continuing`, 'warning');
        } else {
          throw new Error(`ktaInitialize() failed (status=${initFrame.status})`);
        }
      }

      this.flow.initDone = true;
      this.log('[1/4] Secure element initialized', 'success');
      this.setState('ready');
      this.callbacks.onProgress?.(18, 'Initialization complete');

      // ---- STARTUP ----
      this.log('[2/4] Configuring fleet on device...', 'info');
      this.callbacks.onProgress?.(20, 'Configuring device...');

      // Use the fleet UID entered by the user as the context profile UID.
      // Fleet UIDs are typically ASCII identifiers like "9S4F:esp32".
      // Send them as ASCII bytes to the device.  The firmware stores and
      // uses this value as-is in the ICPP activation message.
      let contextProfileUID: Uint8Array;
      const rawFleet = this.config?.fleetProfileUID?.trim();
      if (!rawFleet) {
        throw new Error("Fleet Profile UID must be provided by the user");
      }
      contextProfileUID = new Uint8Array(Buffer.from(rawFleet, 'utf8'));

      const startupPayload = encodeKtaStartupPayload({
        l1SegmentationSeed: this.ktaRef.l1SegSeed,
        contextProfileUID,
        contextSerialNumber: this.ktaRef.contextSerialNum,
        contextVersion: this.ktaRef.contextVersion,
      });

      const startupFrame = await this.sendAndWait(KTA_OPCODE.STARTUP, startupPayload, 20000);
      if (startupFrame.status !== 0) {
        throw new Error(
          `ktaStartup() failed (status=${startupFrame.status} ${KTAService.ktaStatusName(startupFrame.status)})`
        );
      }
      this.flow.startupDone = true;
      this.log('[2/4] Fleet configured on device', 'success');
      this.callbacks.onProgress?.(25, 'ktaStartup OK');

      // ---- SET_DEVICE_INFORMATION ----
      this.log('[3/4] Registering device profile...', 'info');
      this.callbacks.onProgress?.(28, 'Setting device information...');

      const deviceProfilePublicUID =
        this.config?.deviceProfileUID ||
        this.config?.fleetProfileUID ||
        this.ktaRef.defaultDeviceProfilePublicUid;
      const devInfoPayload = encodeKtaSetDeviceInfoPayload({
        deviceProfilePublicUID,
        deviceSerialNumber: this.ktaRef.deviceSerialNum,
        // Matches ktaSetDeviceInformation() semantics: connectionRequest is returned by device in the response.
        conRequest: 0,
      });

      const devInfoFrame = await this.sendAndWait(
        KTA_OPCODE.SET_DEVICE_INFORMATION,
        devInfoPayload,
        20000
      );
      if (devInfoFrame.status !== 0) {
        throw new Error(
          `ktaSetDeviceInformation() failed (status=${devInfoFrame.status} ${KTAService.ktaStatusName(devInfoFrame.status)})`
        );
      }

      const connectionRequest = devInfoFrame.payload?.[0];
      this.flow.deviceInfoDone = true;
      this.log(`[3/4] Device profile registered (connectionRequest=${connectionRequest})`, 'success');
      if (connectionRequest === 1) {
        this.log('Device needs provisioning', 'info');
      } else if (connectionRequest === 0) {
        this.log('Device already provisioned — verifying', 'info');
      }

      // Enforce required call order:
      // initialize() -> startup() -> setDeviceInformation() -> exchangeMessage() -> keyStreamStatus()
      // Even when connectionRequest=0 (already provisioned), calling exchangeMessage(0) is safe;
      // the device should reply with rot2ksLen=0 and we proceed to status.
      if (connectionRequest !== 0 && connectionRequest !== 1) {
        throw new Error(`Unexpected connectionRequest '${String(connectionRequest)}'`);
      }

      this.callbacks.onProgress?.(35, 'Exchanging with keySTREAM...');

      this.log('[4/4] Key exchange with keySTREAM server...', 'info');

      // If device indicates provisioned, do a single exchange round-trip to preserve ordering.
      if (connectionRequest === 0) {
        this.log('Sending verification exchange to device', 'outbound');
        const exPayload = encodeKtaExchangeMessagePayload({ ks2ktaMsg: null });
        let exFrame: KtaBinaryResponseFrame | null = null;
        for (let exAttempt = 1; exAttempt <= 3; exAttempt++) {
          exFrame = await this.sendAndWait(
            KTA_OPCODE.EXCHANGE_MESSAGE,
            exPayload,
            this.exchangeTimeoutMs
          );
          if (exFrame.status === 0) break;
          this.log(`Exchange attempt ${exAttempt}/3 status=${exFrame.status}`, 'warning');
          if (exAttempt < 3) await new Promise(r => setTimeout(() => r(undefined), 1000));
        }
        if (!exFrame || exFrame.status !== 0) {
          throw new Error(`ktaExchangeMessage() failed (status=${exFrame?.status ?? 'null'})`);
        }

        const decoded = decodeKtaExchangeMessageResponsePayload(exFrame.payload);
        const rotLen = decoded.rot2ksMsg.length;

        // Extract RoT UID from the activation response ICPP header
        this.extractAndReportDeviceUID(decoded.rot2ksMsg);

        if (decoded.rot2ksMsg.length !== 0) {
          this.log('Device has pending exchange data — continuing to exchange loop', 'info');
        } else {
          this.log('Device confirmed provisioned', 'success');
          this.setState('provisioned');
          this.callbacks.onProgress?.(100, 'Provisioning complete');
          this.log('Reading device status...', 'info');
          await this.callKtaKeyStreamStatus();
          return;
        }
      }

      // ---- EXCHANGE_MESSAGE loop ----
      this.setState('provisioning');

      this.exchange.active = true;
      this.exchange.lastRot2ksB64 = null;

      let lastKs2kta: Uint8Array | null = null;
      let ks2ktaMsg: Uint8Array | null = null;

      // Resume from saved state if BLE reconnected mid-exchange
      if (this.wasInExchangeLoop && this.resumeKs2kta) {
        ks2ktaMsg = this.resumeKs2kta;
        lastKs2kta = this.resumeKs2kta;
        this.exchange.step = this.resumeExchangeStep;
        this.log(`Resuming exchange from step ${this.resumeExchangeStep}`, 'success');
        this.wasInExchangeLoop = false;
        this.resumeKs2kta = null;
        this.resumeExchangeStep = 0;
      } else {
        this.exchange.step = 0;
        this.wasInExchangeLoop = false;
        this.resumeKs2kta = null;
        this.resumeExchangeStep = 0;
        this.log('Starting key exchange loop', 'info');
      }
      while (true) {
        const exPayload = encodeKtaExchangeMessagePayload({ ks2ktaMsg });
        let exFrame: KtaBinaryResponseFrame | null = null;
        for (let exAttempt = 1; exAttempt <= 3; exAttempt++) {
          exFrame = await this.sendAndWait(
            KTA_OPCODE.EXCHANGE_MESSAGE,
            exPayload,
            this.exchangeTimeoutMs
          );
          if (exFrame.status === 0) break;
          this.log(`Exchange step ${this.exchange.step} attempt ${exAttempt}/3 status=${exFrame.status}`, 'warning');
          if (exAttempt < 3) await new Promise(r => setTimeout(() => r(undefined), 1000));
        }
        if (!exFrame || exFrame.status !== 0) {
          // 0x06 recovery: track consecutive ERROR statuses
          if (exFrame && exFrame.status === 0x06) {
            this.consecutive0x06Count++;
            this.log(`Exchange 0x06 ERROR count: ${this.consecutive0x06Count}/${this.max0x06BeforeRebootstrap}`, 'warning');

            if (this.consecutive0x06Count >= this.max0x06BeforeRebootstrap) {
              const now = Date.now();
              const elapsed = now - this.lastRebootstrapTime;

              if (this.rebootstrapAttempts >= this.maxRebootstrapAttempts) {
                throw new Error(`Max re-bootstrap attempts (${this.maxRebootstrapAttempts}) reached after persistent 0x06 errors`);
              }

              if (elapsed < this.rebootstrapCooldownMs) {
                const waitMs = this.rebootstrapCooldownMs - elapsed;
                this.log(`Cooldown: waiting ${Math.ceil(waitMs / 1000)}s before re-bootstrap`, 'warning');
                await new Promise(r => setTimeout(() => r(undefined), waitMs));
              }

              this.rebootstrapAttempts++;
              this.consecutive0x06Count = 0;
              this.lastRebootstrapTime = Date.now();
              this.log(`Re-bootstrap attempt ${this.rebootstrapAttempts}/${this.maxRebootstrapAttempts}`, 'warning');
              this.exchange.active = false;
              this.flow = { initDone: false, startupDone: false, deviceInfoDone: false };
              return this.runProvisioningFlow();
            }
            // Keep trying exchange with next iteration
            ks2ktaMsg = null;
            continue;
          }
          throw new Error(`ktaExchangeMessage() failed (status=${exFrame?.status ?? 'null'})`);
        }
        // Reset 0x06 counter on any successful exchange
        this.consecutive0x06Count = 0;

        const decoded = decodeKtaExchangeMessageResponsePayload(exFrame.payload);
        const rotLen = decoded.rot2ksMsg.length;

        // Extract RoT Public UID from ICPP header (bytes 10-17) — the device's unique keySTREAM identifier
        this.extractAndReportDeviceUID(decoded.rot2ksMsg);

        this.callbacks.onProgress?.(55, 'Processing server response...');

        if (decoded.rot2ksMsg.length === 0) {
          this.log('Exchange complete — no more data from device', 'success');
          break;
        }

        this.log(`Device → keySTREAM: ${rotLen} bytes`, 'outbound');

        const rotB64 = KeyStreamClient.toBase64(decoded.rot2ksMsg);
        if (this.exchange.lastRot2ksB64 === rotB64) {

          if (!lastKs2kta) {
            // Without a cached response, continue waiting for the next message.
            continue;
          }
          ks2ktaMsg = lastKs2kta;
          continue;
        }

        this.exchange.lastRot2ksB64 = rotB64;
        this.exchange.step += 1;

        const endpoint = this.config?.apiEndpoint;
        const authToken = this.config?.authToken;
        this.callbacks.onProgress?.(60, `Sending to keySTREAM (step ${this.exchange.step})...`);

        const ks2kta = await this.keyStreamClient.postBinary(decoded.rot2ksMsg, {
          endpoint,
          authToken,
          timeoutMs: this.keyStreamTimeoutMs,
          debugLog: false,
        });

        this.log(`keySTREAM → Device: ${ks2kta.length} bytes (step ${this.exchange.step})`, 'inbound');
        this.callbacks.onProgress?.(
          65,
          `Receiving keySTREAM response (step ${this.exchange.step})...`
        );

        lastKs2kta = ks2kta;
        ks2ktaMsg = ks2kta;
        // Save latest server response in case BLE drops — we can resume with this
        this.resumeKs2kta = ks2kta;
        this.resumeExchangeStep = this.exchange.step;
      }

      this.exchange.active = false;
      this.resumeKs2kta = null; // Clear resume state on success
      this.wasInExchangeLoop = false;
      this.setState('provisioned');
      this.log('Provisioning complete', 'success');
      this.callbacks.onProgress?.(100, 'Provisioning complete');
      this.log('Reading device status...', 'info');
      await this.callKtaKeyStreamStatus();
    } catch (e: any) {
      const msg = typeof e?.message === 'string' ? e.message : String(e);
      const isBleDisconnect = msg.includes('BLE disconnected') || msg.includes('Cancelled');

      // If BLE dropped mid-flow, don't treat as a fatal error — auto-reconnect will handle it
      if (isBleDisconnect && !this.userDisconnected) {
        this.log('BLE lost during provisioning — waiting for reconnect', 'warning');
        return; // auto-reconnect callback will re-trigger runProvisioningFlow()
      }

      // No automatic 0xFA here — 0xFA is a LOCAL FACTORY RESET that wipes all
      // cryptographic material (keys, certs) from the ATECC608.  Sending it on
      // a transient exchange failure (e.g. I2C bus error) would destroy the
      // device's provisioning state permanently.  Exchange retries (3 attempts
      // per step) handle transient errors.  If all retries fail, show error and
      // let the user decide whether to factory-reset via the portal or try again.
      //
      // True refurbish is SERVER-INITIATED via keySTREAM (0x50 DELETE_OBJECT
      // commands through the exchange loop), not a local 0xFA wipe.

      this.log(`Provisioning failed: ${msg}`, 'error');
      this.setState('error');
    }
  }



  private static ksStatusName(status: number): string {
    switch (status) {
      case KTA_KS_STATUS.NO_OPERATION: return 'NO_OPERATION';
      case KTA_KS_STATUS.RENEW: return 'RENEW';
      case KTA_KS_STATUS.REFURBISH: return 'REFURBISH';
      default: return `UNKNOWN(${status})`;
    }
  }

  private async callKtaKeyStreamStatus(): Promise<void> {
    const payload = encodeKtaKeyStreamStatusPayload(-1);
    const frame = await this.sendAndWait(KTA_OPCODE.KEY_STREAM_STATUS, payload, 20000);
    if (frame.status !== 0) {
      throw new Error(`ktaKeyStreamStatus() failed (status=${frame.status})`);
    }
    const decoded = decodeKtaKeyStreamStatusResponsePayload(frame.payload);
    const statusName = KTAService.ksStatusName(decoded.ksStatus);

    if (decoded.ksStatus === KTA_KS_STATUS.RENEW) {
      this.log(`Device status: ${statusName} — key renewal pending`, 'inbound');
    } else if (decoded.ksStatus === KTA_KS_STATUS.REFURBISH) {
      this.log(`Device status: ${statusName} — refurbish pending`, 'warning');
    } else {
      this.log(`Device status: ${statusName}`, 'success');
    }

    // ✅ ENABLED: Periodic server command polling (every 10 seconds)
    this.startPeriodicServerCheck();
    this.startKeepalive();
    this.log('Monitoring keySTREAM for commands (RENEW / REFURBISH)', 'info');
  }

  // Periodic polling for server commands (REFURBISH/RENEW/ROTATE)
  private serverCheckInterval: ReturnType<typeof setInterval> | null = null;
  private serverCheckRunning: boolean = false;  // Guard against concurrent checks

  private startPeriodicServerCheck(): void {
    // Clear any existing interval
    if (this.serverCheckInterval) {
      clearInterval(this.serverCheckInterval);
    }
    this.serverCheckRunning = false;

    const interval = 10000;
    this.serverCheckInterval = setInterval(async () => {
      try {
        // Prevent concurrent checkForServerCommands calls (network can be slow)
        if (this.serverCheckRunning) {
          return;
        }

        let connected = false;
        try {
          connected = this.isConnected();
        } catch (e) {

          connected = false;
        }

        if (this.state !== 'provisioned') {
          return;
        }

        if (!connected) {
          return;
        }

        this.serverCheckRunning = true;
        await this.checkForServerCommands();
      } catch (error) {

      } finally {
        this.serverCheckRunning = false;
      }
    }, interval);

    // Also do an immediate check (don't wait for first interval)
    setTimeout(async () => {
      try {
        let connected = false;
        try {
          connected = this.isConnected();
        } catch (e) {
          connected = false;
        }
        if (this.state === 'provisioned' && connected) {
          try {
            await this.checkForServerCommands();
          } catch (error) {

          }
        }
      } catch (e) {

      }
    }, 500);
  }

  public stopPeriodicServerCheck(): void {
    if (this.serverCheckInterval) {
      clearInterval(this.serverCheckInterval);
      this.serverCheckInterval = null;

    }
  }

  /** Start BLE keepalive: send STATUS (0x08) every 30s when idle to prevent BLE timeout. */
  private startKeepalive(): void {
    this.stopKeepalive();
    this.keepaliveInterval = setInterval(async () => {
      try {
        // Skip keepalive during active exchange
        if (this.exchange.active) return;
        if (this.state !== 'provisioned') return;

        let connected = false;
        try { connected = this.isConnected(); } catch { connected = false; }
        if (!connected) return;

        const payload = encodeKtaKeyStreamStatusPayload(-1);
        await this.sendAndWait(KTA_OPCODE.KEY_STREAM_STATUS, payload, 15000);
      } catch {
        // Keepalive failure is non-fatal
      }
    }, this.keepaliveIntervalMs);
  }

  /** Stop BLE keepalive polling. */
  private stopKeepalive(): void {
    if (this.keepaliveInterval) {
      clearInterval(this.keepaliveInterval);
      this.keepaliveInterval = null;
    }
  }

  /**
   * Check for and relay server commands (NOOP polling).
   *
   * After provisioning, this runs every 10 seconds. It relays messages
   * between the device and keySTREAM server in a loop:
   *   1. Send NOOP (null exchange) to device → device returns rot2ks
   *   2. Post rot2ks to keySTREAM → server returns ks2kta (commands)
   *   3. Send ks2kta to device → device processes and returns response
   *   4. Post response to server → repeat from 2 if server has more
   *   5. Loop ends when server returns empty or device returns empty
   *
   * For REFURBISH, the server sends 0x50 DELETE_OBJECT commands through
   * this relay. The device processes them, resets to INIT state, and on
   * the NEXT polling cycle returns status=6 (ERROR) which triggers full
   * re-provisioning: INITIALIZE → STARTUP → SET_DEVICE_INFO → EXCHANGE(0).
   */
  public async checkForServerCommands(): Promise<void> {
    try {
      const exPayload = encodeKtaExchangeMessagePayload({ ks2ktaMsg: null });
      const exFrame = await this.sendAndWait(
        KTA_OPCODE.EXCHANGE_MESSAGE,
        exPayload,
        this.exchangeTimeoutMs
      );

      if (exFrame.status !== 0) {
        // STATUS 6 (ERROR) while previously provisioned = device was reset (refurbish).
        if ((this.state === 'provisioned' || this.state === 'error') && exFrame.status === 6) {
          this.log('REFURBISH detected — device was reset by keySTREAM', 'warning');

          this.stopPeriodicServerCheck();
          this.flow = { initDone: false, startupDone: false, deviceInfoDone: false };

          let succeeded = false;
          for (let attempt = 1; attempt <= 3; attempt++) {
            try {
              this.log(`Re-provisioning attempt ${attempt}/3...`, 'info');
              await this.runProvisioningFlow();
              this.log('Re-provisioning after REFURBISH complete', 'success');
              succeeded = true;
              break;
            } catch (error) {
              if (attempt < 3) {
                await new Promise(r => setTimeout(() => r(undefined), 2000));
                this.flow = { initDone: false, startupDone: false, deviceInfoDone: false };
              }
            }
          }

          if (!succeeded) {
            this.log('Re-provisioning failed — will retry on next cycle', 'error');
            this.setState('provisioned');
            this.startPeriodicServerCheck();
          }
        }
        return;
      }

      // Relay loop — bounce messages between device and server
      let currentRot2ks = decodeKtaExchangeMessageResponsePayload(exFrame.payload).rot2ksMsg;
      let relayRound = 0;
      const MAX_RELAY_ROUNDS = 10;

      while (currentRot2ks.length > 0 && relayRound < MAX_RELAY_ROUNDS) {
        relayRound++;
        this.log(`Device → keySTREAM: ${currentRot2ks.length} bytes (relay round ${relayRound})`, 'outbound');

        const endpoint = this.config?.apiEndpoint;
        const authToken = this.config?.authToken;
        let ks2kta: Uint8Array;
        try {
          ks2kta = await this.keyStreamClient.postBinary(currentRot2ks, {
            endpoint,
            authToken,
            timeoutMs: this.keyStreamTimeoutMs,
            debugLog: false,
          });
        } catch (postErr: any) {
          this.log(`keySTREAM error: ${postErr?.message?.substring(0, 80)}`, 'error');
          break;
        }

        if (ks2kta.length === 0) break;

        this.log(`keySTREAM → Device: ${ks2kta.length} bytes (relay round ${relayRound})`, 'inbound');

        const responsePayload = encodeKtaExchangeMessagePayload({ ks2ktaMsg: ks2kta });
        let responseFrame;
        try {
          responseFrame = await this.sendAndWait(
            KTA_OPCODE.EXCHANGE_MESSAGE,
            responsePayload,
            this.exchangeTimeoutMs
          );
        } catch (devErr: any) {
          this.log('Device error during relay', 'error');
          break;
        }

        if (responseFrame.status !== 0) {
          if (responseFrame.status === 6) {
            this.log('REFURBISH processed — device will re-provision on next cycle', 'warning');
          }
          break;
        }

        const ackDecoded = decodeKtaExchangeMessageResponsePayload(responseFrame.payload);
        currentRot2ks = ackDecoded.rot2ksMsg;
      }

      if (relayRound > 0) {
        this.log(`Server command relay done (${relayRound} round${relayRound > 1 ? 's' : ''}) — RENEW/ROTATE processed`, 'inbound');
      }
    } catch (error: any) {
      throw error;
    }
  }

  getState(): KTAState {
    return this.state;
  }

  // Robust implementation: tolerate function or boolean or legacy shapes.
  isConnected(): boolean {
    const bt: any = this.bleTransport as any;
    if (typeof bt.isConnected === 'function') return bt.isConnected();
    if (typeof bt.isDeviceConnected === 'function') return bt.isDeviceConnected();
    if (typeof bt.isConnected === 'boolean') return bt.isConnected;
    if (typeof bt._isConnected === 'boolean') return bt._isConnected;
    return false;
  }

  destroy() {
    this.stopPeriodicServerCheck(); // Stop polling timer
    this.stopKeepalive(); // Stop BLE keepalive
    this.clearPendingWaits('Service destroyed');
    this.bleTransport.destroy();
  }
}