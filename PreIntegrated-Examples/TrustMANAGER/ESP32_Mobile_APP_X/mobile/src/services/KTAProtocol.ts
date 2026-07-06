import { Buffer } from 'buffer';

export const KTA_OPCODE = {
  INITIALIZE:             0x01,
  STARTUP:                0x02,
  SET_DEVICE_INFORMATION: 0x03,
  EXCHANGE_MESSAGE:       0x04,
  KEY_STREAM_STATUS:      0x08,
  SET_CHIP_CERTIFICATE:   0x09,
  REFURBISH:              0xFA,
} as const;

export const KTA_KS_STATUS = {
  NO_OPERATION: 0,
  RENEW: 1,
  REFURBISH: 2,
} as const;

export type KtaOpcode = (typeof KTA_OPCODE)[keyof typeof KTA_OPCODE];

export interface KtaBinaryFrame {
  opcode: number;
  payload: Uint8Array;
}

export interface KtaBinaryResponseFrame {
  opcode: number;
  status: number;
  payload: Uint8Array;
}

function asBuffer(value: string | Uint8Array, length: number): Buffer {
  let buf: Buffer;
  if (typeof value === 'string') {
    buf = Buffer.from(value, 'ascii');
  } else {
    buf = Buffer.from(value);
  }
  const result = Buffer.alloc(length);
  buf.copy(result, 0, 0, Math.min(buf.length, length));
  return result;
}

export function encodeRequestFrame(frame: KtaBinaryFrame): Buffer {
  const payload = frame.payload || new Uint8Array();
  const header = Buffer.alloc(3);
  header[0] = frame.opcode;
  header.writeUInt16LE(payload.length, 1);
  return Buffer.concat([header, Buffer.from(payload)]);
}

export function tryDecodeResponseFrames(buffer: Buffer): { frames: KtaBinaryResponseFrame[]; remainder: Buffer } {
  const frames: KtaBinaryResponseFrame[] = [];
  let offset = 0;

  while (buffer.length - offset >= 4) {
    const opcode = buffer[offset];
    const status = buffer[offset + 1];
    const payloadLen = buffer.readUInt16LE(offset + 2);

    if (buffer.length - offset < 4 + payloadLen) {
      break;
    }

    const payload = Buffer.from(buffer.subarray(offset + 4, offset + 4 + payloadLen));
    frames.push({ opcode, status, payload });
    offset += 4 + payloadLen;
  }

  return { frames, remainder: Buffer.from(buffer.subarray(offset)) };
}

export function encodeKtaStartupPayload(params: {
  l1SegmentationSeedHex?: string;
  l1SegmentationSeed?: Uint8Array;
  contextProfileUID: string | Uint8Array;
  contextSerialNumber: string | Uint8Array;
  contextVersion: string | Uint8Array;
}): Buffer {
  const seed = params.l1SegmentationSeed
    ? Buffer.from(params.l1SegmentationSeed)
    : Buffer.from((params.l1SegmentationSeedHex || '').replace(/^0x/i, ''), 'hex');
  
  const seedBuf = Buffer.alloc(16);
  seed.copy(seedBuf, 0, 0, Math.min(seed.length, 16));

  const uidSrc = typeof params.contextProfileUID === 'string' ? Buffer.from(params.contextProfileUID, 'ascii') : Buffer.from(params.contextProfileUID);
  const uidBuf = Buffer.alloc(32);
  uidSrc.copy(uidBuf, 0, 0, Math.min(uidSrc.length, 32));
  const uidLenBuf = Buffer.alloc(4);
  uidLenBuf.writeUInt32LE(Math.min(uidSrc.length, 32), 0);

  const serialSrc = typeof params.contextSerialNumber === 'string' ? Buffer.from(params.contextSerialNumber, 'ascii') : Buffer.from(params.contextSerialNumber);
  const serialBuf = Buffer.alloc(16);
  serialSrc.copy(serialBuf, 0, 0, Math.min(serialSrc.length, 16));
  const serialLenBuf = Buffer.alloc(4);
  serialLenBuf.writeUInt32LE(Math.min(serialSrc.length, 16), 0);

  const versionSrc = typeof params.contextVersion === 'string' ? Buffer.from(params.contextVersion, 'ascii') : Buffer.from(params.contextVersion);
  const versionBuf = Buffer.alloc(16);
  versionSrc.copy(versionBuf, 0, 0, Math.min(versionSrc.length, 16));
  const versionLenBuf = Buffer.alloc(4);
  versionLenBuf.writeUInt32LE(Math.min(versionSrc.length, 16), 0);

  return Buffer.concat([seedBuf, uidBuf, uidLenBuf, serialBuf, serialLenBuf, versionBuf, versionLenBuf]);
}

export function encodeKtaSetDeviceInfoPayload(params: {
  deviceProfilePublicUID: string | Uint8Array;
  deviceSerialNumber: string | Uint8Array;
  conRequest: number;
}): Buffer {
  const uidSrc = typeof params.deviceProfilePublicUID === 'string' ? Buffer.from(params.deviceProfilePublicUID, 'ascii') : Buffer.from(params.deviceProfilePublicUID);
  const uidBuf = Buffer.alloc(32);
  uidSrc.copy(uidBuf, 0, 0, Math.min(uidSrc.length, 32));
  const uidLenBuf = Buffer.alloc(4);
  uidLenBuf.writeUInt32LE(Math.min(uidSrc.length, 32), 0);

  const serialSrc = typeof params.deviceSerialNumber === 'string' ? Buffer.from(params.deviceSerialNumber, 'ascii') : Buffer.from(params.deviceSerialNumber);
  const serialBuf = Buffer.alloc(16);
  serialSrc.copy(serialBuf, 0, 0, Math.min(serialSrc.length, 16));
  const serialLenBuf = Buffer.alloc(4);
  serialLenBuf.writeUInt32LE(Math.min(serialSrc.length, 16), 0);

  return Buffer.concat([uidBuf, uidLenBuf, serialBuf, serialLenBuf]);
}

export function encodeKtaExchangeMessagePayload(params: { ks2ktaMsg: Uint8Array | null }): Buffer {
  const msgLen = params.ks2ktaMsg ? params.ks2ktaMsg.length : 0;
  const buf = Buffer.alloc(2 + msgLen);
  buf.writeUInt16LE(msgLen, 0);
  if (msgLen > 0) {
    Buffer.from(params.ks2ktaMsg!).copy(buf, 2);
  }
  return buf;
}

export function encodeKtaKeyStreamStatusPayload(_currentStatus: number): Buffer {
  return Buffer.alloc(0);
}

export function decodeKtaExchangeMessageResponsePayload(payload: Uint8Array): { rot2ksMsg: Uint8Array } {
  const buf = Buffer.from(payload);
  if (buf.length < 2) return { rot2ksMsg: new Uint8Array() };
  const n = buf.readUInt16LE(0);
  const available = Math.max(0, buf.length - 2);
  const take = Math.min(n, available);
  return { rot2ksMsg: buf.subarray(2, 2 + take) };
}

export function decodeKtaKeyStreamStatusResponsePayload(payload: Uint8Array): { ksStatus: number } {
  const buf = Buffer.from(payload);
  if (buf.length < 4) return { ksStatus: 0 };
  return { ksStatus: buf.readInt32LE(0) };
}