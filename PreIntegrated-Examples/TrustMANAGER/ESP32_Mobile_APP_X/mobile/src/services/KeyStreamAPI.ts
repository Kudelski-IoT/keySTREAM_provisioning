/**
 * KeyStreamAPI – REST client for keySTREAM Device Management APIs.
 *
 * API 1 – List devices in a fleet:
 *   GET /lp/dm/{tenantId}/devices?deviceProfilePublicUid={profileUid}
 *
 * API 2 – Get single device details:
 *   GET /lp/dm/{tenantId}/devices/{deviceId}
 */

import { KtaConfig } from './KtaConfig';

// Strip 'icph.' or 'icpp.' prefix from the provisioning URL to get the REST Portal API URL.
const FALLBACK_BASE_URL = "https://mss.iot.kudelski.com";

function correlationId(): string {
  // Simple v4-ish UUID for the x-correlation-id header
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
    const r = (Math.random() * 16) | 0;
    const v = c === 'x' ? r : (r & 0x3) | 0x8;
    return v.toString(16);
  });
}

export interface KeyStreamDevice {
  deviceId: string;
  chipUid: string;
  status: string;
  deviceProfilePublicUid?: string;
  rotPublicUid?: string;
  deviceState?: string;
  devicePublicUid?: string;
  fleetProfileUid?: string;
  name?: string;
  [key: string]: any;           // allow extra fields from server
}

export interface KeyStreamDeviceDetails {
  deviceId: string;
  chipUid: string;
  status: string;
  fleetProfileUid?: string;
  deviceProfilePublicUid?: string;
  rotPublicUid?: string;
  deviceState?: string;
  name?: string;
  lifecycleState?: string;
  lastSeen?: string;
  [key: string]: any;
}

export class KeyStreamAPI {
  private authToken: string;
  private tenantId: string;
  private baseUrl: string;

  constructor(authToken: string, tenantId?: string, apiBaseUrl?: string) {
    this.authToken = authToken;
    this.tenantId = tenantId || '';
    this.baseUrl = apiBaseUrl || FALLBACK_BASE_URL;
  }

  /** Update the tenant ID (e.g. after lookupTenant resolves it). */
  setTenantId(id: string) {
    this.tenantId = id;
  }

  private headers(accept?: string): Record<string, string> {
    const h: Record<string, string> = {
      Accept: accept || 'application/json',
      'x-correlation-id': correlationId(),
    };
    if (this.authToken) {
      const token = this.authToken.trim();
      const lower = token.toLowerCase();
      if (lower.startsWith('bearer ')) {
        h.Authorization = token;
      } else if (lower.startsWith('basic ')) {
        h.Authorization = token;
      } else if (token.includes('iam-session-id=')) {
        h.Cookie = token;
      } else if (token.includes('.') && token.split('.').length === 3) {
        h.Authorization = `Bearer ${token}`;
      } else if (/^[A-Za-z0-9+/=]+$/.test(token) && token.length > 20) {
        h.Authorization = `Basic ${token}`;
      } else {
        h.Cookie = token.includes('=') ? token : `iam-session-id=${token}`;
      }
    }
    return h;
  }

  /**
   * API 0 – Resolve tenant UUID from a device-profile public UID.
   * GET /dm?dpPublicUid={profileUid}
   */
  async lookupTenant(dpPublicUid: string): Promise<{ uuid: string; publicUid: string }> {
    const encoded = encodeURIComponent(dpPublicUid);
    const url = `${this.baseUrl}/dm?dpPublicUid=${encoded}`;

    const res = await fetch(url, { method: 'GET', headers: this.headers() });

    if (!res.ok) {
      const text = await res.text().catch(() => '');
      throw new Error(`lookupTenant HTTP ${res.status}: ${text.slice(0, 300)}`);
    }

    const json = await res.json();
    const managers: any[] = json.deviceManagers || [];
    if (managers.length === 0) {
      throw new Error('No tenant found for this device profile');
    }
    const tenant = managers[0];
    this.tenantId = tenant.uuid;
    return { uuid: tenant.uuid, publicUid: tenant.publicUid || '' };
  }

  /**
   * API 1 – List all devices that belong to a given device-profile.
   * GET /lp/dm/{tenantId}/devices?deviceProfilePublicUid={profileUid}
   */
  async listDevices(deviceProfilePublicUid: string): Promise<KeyStreamDevice[]> {
    if (!this.tenantId) {
      throw new Error('Tenant ID not set. Call lookupTenant() first.');
    }
    const encoded = encodeURIComponent(deviceProfilePublicUid);
    const url = `${this.baseUrl}/lp/dm/${this.tenantId}/devices?deviceProfilePublicUid=${encoded}`;

    const res = await fetch(url, { method: 'GET', headers: this.headers() });

    if (!res.ok) {
      const text = await res.text().catch(() => '');
      throw new Error(`listDevices HTTP ${res.status}: ${text.slice(0, 300)}`);
    }

    const json = await res.json();
    const items: any[] = Array.isArray(json)
      ? json
      : json.registrationData ?? json.registrations ?? json.content ?? json.devices ?? json.items ?? [];

    return items.map((d: any) => ({
      deviceId: d.deviceId ?? d.devicePublicUid ?? d.id ?? '',
      chipUid: d.chipUid ?? d.chipUID ?? d.uid ?? '',
      status: d.status ?? d.deviceState ?? d.lifecycleState ?? 'unknown',
      deviceProfilePublicUid: d.deviceProfilePublicUid ?? '',
      rotPublicUid: d.rotPublicUid ?? '',
      deviceState: d.deviceState ?? d.status ?? d.lifecycleState ?? '',
      devicePublicUid: d.devicePublicUid ?? '',
      fleetProfileUid: d.fleetProfileUid ?? d.fleetUid ?? '',
      name: d.name ?? d.deviceName ?? '',
      ...d,
    }));
  }

  /**
   * API 2 – Get detailed info for a single device.
   * GET /lp/dm/{tenantId}/devices/{deviceId}
   */
  async getDeviceInfo(deviceId: string): Promise<KeyStreamDeviceDetails> {
    if (!this.tenantId) {
      throw new Error('Tenant ID not set. Call lookupTenant() first.');
    }
    const url = `${this.baseUrl}/lp/dm/${this.tenantId}/devices/${encodeURIComponent(deviceId)}`;

    const res = await fetch(url, { method: 'GET', headers: this.headers('*/*') });

    if (!res.ok) {
      const text = await res.text().catch(() => '');
      throw new Error(`getDeviceInfo HTTP ${res.status}: ${text.slice(0, 300)}`);
    }

    const d: any = await res.json();

    return {
      deviceId: d.deviceId ?? d.devicePublicUid ?? d.id ?? deviceId,
      chipUid: d.chipUid ?? d.chipUID ?? d.uid ?? '',
      status: d.status ?? d.deviceState ?? d.lifecycleState ?? 'unknown',
      fleetProfileUid: d.fleetProfileUid ?? d.fleetUid ?? '',
      deviceProfilePublicUid: d.deviceProfilePublicUid ?? '',
      rotPublicUid: d.rotPublicUid ?? '',
      deviceState: d.deviceState ?? d.status ?? d.lifecycleState ?? '',
      name: d.name ?? d.deviceName ?? '',
      lifecycleState: d.lifecycleState ?? d.deviceState ?? '',
      lastSeen: d.lastSeen ?? d.lastUpdated ?? '',
      ...d,
    };
  }
}
