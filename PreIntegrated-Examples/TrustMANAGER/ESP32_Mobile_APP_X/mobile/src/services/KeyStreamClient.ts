import { Buffer } from 'buffer';
import { KtaConfig } from './KtaConfig';

export interface KeyStreamClientOptions {
  /**
   * Full endpoint URL (preferred), e.g. `http://icph.cie.iot-lab.kudelski.com/lp1`.
   * If you pass only a hostname (no scheme), `http://` is assumed.
   */
  endpoint?: string;
  /**
   * Optional bearer token for keySTREAM (if your environment requires auth).
   * If provided, sent as `Authorization: Bearer <token>`.
   */
  authToken?: string;
  timeoutMs?: number;
  /** When true, logs the exact HTTP request details (auth/cookie redacted). */
  debugLog?: boolean;
  /** Max bytes to include as hex preview in logs (default 128). */
  debugHexBytes?: number;
  /** Max bytes to include as base64 preview in logs (default 256). */
  debugBase64Bytes?: number;
  /** When true, also logs full base64 bodies (can be large; may destabilize RN/Logcat). */
  debugIncludeFullBase64?: boolean;
}

const DEFAULT_ENDPOINT = "http://icph.mss.iot.kudelski.com/lp1";

function normalizeEndpoint(endpoint: string | undefined): string {
  let raw = (endpoint || '').trim();
  if (!raw) return DEFAULT_ENDPOINT;

  // Force http:// to avoid TLS/CA issues in mobile environments.
  raw = raw.replace(/^https:\/\//i, 'http://');

  // If user provided only a hostname, assume http.
  const withScheme = /^[a-zA-Z][a-zA-Z0-9+.-]*:\/\//.test(raw) ? raw : `http://${raw}`;
 
  // If path is missing, append /lp1. Avoid relying on DOM URL typings in React Native.
  const trimmed = withScheme.replace(/\s+/g, '');

  // Split into "scheme://host" + "/path?..." (if any)
  const m = trimmed.match(/^([a-zA-Z][a-zA-Z0-9+.-]*:\/\/[^/]+)(\/.*)?$/);
  if (!m) {
    return trimmed.endsWith('/lp1') ? trimmed : `${trimmed.replace(/\/+$/, '')}/lp1`;
  }

  const base = m[1];
  const rest = m[2] || '';
  if (!rest || rest === '/') {
    return `${base}/lp1`;
  }

  // Keep existing path (and any query) but strip trailing slash.
  return `${base}${rest}`.replace(/\/+$/, '');
}

function arrayBufferFromBytes(bytes: Uint8Array): ArrayBuffer {
  return bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) as ArrayBuffer;
}

function cookieValueFromSetCookie(setCookie: string): string | null {
  // Keep only the first `k=v` part.
  const first = setCookie.split(';')[0]?.trim();
  if (!first || !first.includes('=')) return null;
  return first;
}

export class KeyStreamClient {
  private cookie: string | null = null;

  private static redactHeadersForLog(headers: Record<string, string>): Record<string, string> {
    const out: Record<string, string> = {};
    for (const [k, v] of Object.entries(headers)) {
      const key = k;
      const lower = key.toLowerCase();
      if (lower === 'authorization') {
        out[key] = v ? 'Bearer <redacted>' : '';
      } else if (lower === 'cookie') {
        out[key] = v ? '<present>' : '';
      } else {
        out[key] = v;
      }
    }
    return out;
  }

  private static toHexPreview(bytes: Uint8Array, maxBytes: number): string {
    const n = Math.min(bytes.length, Math.max(0, maxBytes | 0));
    const slice = bytes.subarray(0, n);
    return Buffer.from(slice).toString('hex');
  }

  private static toBase64Preview(bytes: Uint8Array, maxBytes: number): { preview: string; truncated: boolean } {
    const n = Math.min(bytes.length, Math.max(0, maxBytes | 0));
    const slice = bytes.subarray(0, n);
    const preview = Buffer.from(slice).toString('base64');
    return { preview, truncated: n < bytes.length };
  }

  private static formatErr(e: unknown): { name?: string; message: string } {
    const anyE = e as any;
    const name = typeof anyE?.name === 'string' ? anyE.name : undefined;
    const message = typeof anyE?.message === 'string' ? anyE.message : String(e);
    return { name, message };
  }

  private static classifyNetworkHint(name: string | undefined, message: string): string | null {
    const msg = (message || '').toLowerCase();

    if (name === 'AbortError' || msg.includes('aborted')) {
      return 'request timed out (AbortController)';
    }

    // React Native commonly reports this for DNS/TLS/offline/firewall/proxy.
    if (msg.includes('network request failed') || msg.includes('failed to fetch')) {
      return 'network layer failure (offline, DNS, TLS/CA trust, proxy/VPN, or firewall)';
    }

    // Android can throw SSL-related messages depending on the stack.
    if (msg.includes('ssl') || msg.includes('cert') || msg.includes('handshake')) {
      return 'TLS handshake/certificate issue';
    }

    return null;
  }

  async postBinary(payload: Uint8Array, options: KeyStreamClientOptions = {}): Promise<Uint8Array> {
    const endpoint = normalizeEndpoint(options.endpoint);
    const timeoutMs = options.timeoutMs ?? 5000;
    const startMs = Date.now();

    const controller = new AbortController();
    const timeout = setTimeout(() => controller.abort(), timeoutMs);

    try {
      const headers: Record<string, string> = {
        'Content-Type': 'application/octet-stream',
        Accept: 'application/octet-stream',
      };

      const authToken = (options.authToken || '').trim();
      if (authToken) {
        headers.Authorization = authToken.toLowerCase().startsWith('bearer ') ? authToken : `Bearer ${authToken}`;
      }
      if (this.cookie) headers.Cookie = this.cookie;

      if (options.debugLog) {
        const redacted = KeyStreamClient.redactHeadersForLog(headers);
        const hexBytes = options.debugHexBytes ?? 128;
        const hexPreview = KeyStreamClient.toHexPreview(payload, hexBytes);
        const b64Bytes = options.debugBase64Bytes ?? 256;
        const { preview: b64Preview, truncated: b64Truncated } = KeyStreamClient.toBase64Preview(payload, b64Bytes);
        const includeFull = Boolean(options.debugIncludeFullBase64);
        const bodyBase64 = includeFull ? Buffer.from(payload).toString('base64') : undefined;

        // Debug request info available via options.debugLog
      }

      const res = await fetch(endpoint, {
        method: 'POST',
        headers,
        body: arrayBufferFromBytes(payload) as any,
        signal: controller.signal,
      });

      const elapsedMs = Date.now() - startMs;

      const setCookie = (res.headers as any)?.get?.('set-cookie') as string | null;
      if (setCookie) {
        const cookieKV = cookieValueFromSetCookie(setCookie);
        if (cookieKV) this.cookie = cookieKV;
      }

      if (!res.ok) {
        const contentType = (res.headers as any)?.get?.('content-type') as string | null;
        const text = await res.text().catch(() => '');
        const bodySnippet = text ? text.slice(0, 500) : '';

        throw new Error(
          `keySTREAM HTTP ${res.status} ${res.statusText}` +
            `${contentType ? ` (content-type=${contentType})` : ''}` +
            `${bodySnippet ? `: ${bodySnippet}` : ''}`
        );
      }

      const ab = await res.arrayBuffer();
      const out = new Uint8Array(ab);

      return out;
    } catch (e: any) {
      const { name, message } = KeyStreamClient.formatErr(e);
      const hint = KeyStreamClient.classifyNetworkHint(name, message);
      const authProvided = Boolean((options.authToken || '').trim());
      const cookieProvided = Boolean(this.cookie);
      const elapsedMs = Date.now() - startMs;

      // Attach endpoint and request shape to make mobile logs actionable.
      const details = [
        `${payload.length} bytes -> ${endpoint}`,
        `timeoutMs=${timeoutMs}`,
        `elapsedMs=${elapsedMs}`,
        `auth=${authProvided ? 'yes' : 'no'}`,
        `cookie=${cookieProvided ? 'yes' : 'no'}`,
      ].join(', ');

      const base = `keySTREAM exchange failed (${details}): ${message}`;
      throw new Error(hint ? `${base} [${hint}]` : base);
    } finally {
      clearTimeout(timeout);
    }
  }

  /** Convenience helper for logging/debugging. */
  static toBase64(bytes: Uint8Array): string {
    return Buffer.from(bytes).toString('base64');
  }
}
