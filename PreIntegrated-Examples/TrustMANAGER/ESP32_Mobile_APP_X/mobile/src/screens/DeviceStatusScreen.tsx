import React, { useEffect, useMemo, useRef, useState, useCallback } from 'react';
import {
  View,
  Text,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  ScrollView,
  BackHandler,
  ActivityIndicator,
  Modal,
} from 'react-native';

import { KTAService, KTAState } from '../services/KTAService';
import { KeyStreamAPI, KeyStreamDevice } from '../services/KeyStreamAPI';

type LogType = 'info' | 'success' | 'error' | 'warning' | 'outbound' | 'inbound';

interface LogEntry {
  timestamp: string;
  message: string;
  type: LogType;
}

interface DeviceStatusScreenProps {
  bleDeviceName?: string;
  bleDeviceAddress?: string;
  bleDeviceId?: string;
  authToken?: string;
  chipUID?: string;
  fleetUID?: string;
  tenantId?: string;
  serverUrl?: string;
  apiBaseUrl?: string;
  onBack?: () => void;
}

export const DeviceStatusScreen: React.FC<DeviceStatusScreenProps> = ({
  bleDeviceName = 'ESP32-BLE',
  bleDeviceAddress = 'AA:BB:CC:DD:EE:FF',
  bleDeviceId = '',
  authToken = '',
  chipUID = '',
  fleetUID = '',
  tenantId = '',
  serverUrl = '',
  apiBaseUrl = '',
  onBack,
}) => {
  const [bleConnected, setBleConnected] = useState(false);
  const [deviceStatus, setDeviceStatus] = useState<KTAState>('idle');
  const [progress, setProgress] = useState(0);
  const [progressMessage, setProgressMessage] = useState('');

  // keySTREAM device info (from API 2)
  const [ksDeviceStatus, setKsDeviceStatus] = useState<string>('—');
  const [ksLoading, setKsLoading] = useState(false);

  // Fleet list modal (API 1)
  const [showFleetModal, setShowFleetModal] = useState(false);
  const [fleetDevices, setFleetDevices] = useState<KeyStreamDevice[]>([]);
  const [fleetLoading, setFleetLoading] = useState(false);
  const [fleetError, setFleetError] = useState('');

  const [logs, setLogs] = useState<LogEntry[]>([]);
  const logListRef = useRef<FlatList<LogEntry>>(null);

  const addLog = (message: string, type: LogType = 'info') => {
    setLogs((prev) => {
      const next = [
        ...prev,
        { timestamp: new Date().toLocaleTimeString(), message, type },
      ];
      // Auto-scroll to bottom after state update
      setTimeout(() => {
        logListRef.current?.scrollToEnd({ animated: true });
      }, 100);
      return next;
    });
  };

  // ---- KTA provisioning service ----
  const ktaRef = useRef<KTAService | null>(null);
  const kta = useMemo(() => {
    if (ktaRef.current) return ktaRef.current;
    ktaRef.current = new KTAService({
      onStateChange: (state) => setDeviceStatus(state),
      onLog: (message, type) => addLog(message, type as LogType),
      onProgress: (p, msg) => {
        setProgress(p);
        setProgressMessage(msg);
      },
      onDeviceUID: (uid) => {
        setRealChipUID(uid);
      },
      onBLEConnectionChange: (connected) => {
        setBleConnected(connected);
      },
    });
    return ktaRef.current;
  }, []);

  // ---- keySTREAM REST API client ----
  const apiRef = useRef<KeyStreamAPI | null>(null);
  const api = useMemo(() => {
    // Always create a fresh API client when authToken or tenantId change
    apiRef.current = new KeyStreamAPI(authToken, tenantId, apiBaseUrl);
    return apiRef.current;
  }, [authToken, tenantId, apiBaseUrl]);

  // Resolved deviceId from keySTREAM (matched by chipUID after activation)
  const [ksDeviceId, setKsDeviceId] = useState<string>('');

  // Real device UID extracted from ICPP activation message (RoT Public UID)
  const [realChipUID, setRealChipUID] = useState<string>('');

  // Chip UID from API response (separate from RoT UID)
  const [apiChipUID, setApiChipUID] = useState<string>('');

  // Chip UID: prefer API-returned chipUid, then BLE-derived prop
  const displayChipUID = apiChipUID || chipUID || '—';

  // Track last polled status to avoid log spam during periodic polling
  const lastPolledStatusRef = useRef<string>('');

  // ---- API 2: Fetch device status from keySTREAM using RoT UID ----
  // Helper: normalize UID for comparison (strip non-hex, lowercase)
  const normalizeUid = (s: string) => s.replace(/[^0-9a-f]/gi, '').toLowerCase();

  // Retry counter — on failure we retry up to 3 times with 2s gap
  const statusRetryRef = useRef(0);

  const fetchDeviceStatus = useCallback(async (rotUid?: string, silent?: boolean) => {
    const uid = rotUid || realChipUID;
    if (!uid || !tenantId) return;
    if (!silent) {
      setKsLoading(true);
      addLog(`Querying keySTREAM for device status...`, 'outbound');
    }

    const normUid = normalizeUid(uid);

    try {
      let found = false;

      // Approach 1: List fleet devices and match by RoT UID
      if (fleetUID) {
        try {
          const devices = await api.listDevices(fleetUID);
          const matched = devices.find(
            (d) =>
              normalizeUid(d.rotPublicUid || '') === normUid ||
              normalizeUid(d.chipUid || '') === normUid ||
              normalizeUid(d.devicePublicUid || '') === normUid
          );
          if (matched) {
            const devId = matched.deviceId || matched.devicePublicUid || '';
            setKsDeviceId(devId);
            if (matched.chipUid) setApiChipUID(matched.chipUid);
            const st = matched.deviceState || matched.status || 'Unknown';
            const changed = st !== lastPolledStatusRef.current;
            lastPolledStatusRef.current = st;
            setKsDeviceStatus(st);
            if (changed) addLog(`keySTREAM status: ${st}`, 'inbound');
            found = true;

            if (devId) {
              try {
                const detail = await api.getDeviceInfo(devId);
                const detailSt = detail.deviceState || detail.status || detail.lifecycleState || st;
                if (detailSt !== st) {
                  const detChanged = detailSt !== lastPolledStatusRef.current;
                  lastPolledStatusRef.current = detailSt;
                  setKsDeviceStatus(detailSt);
                  if (detChanged) addLog(`keySTREAM status: ${detailSt}`, 'inbound');
                }
                if (detail.chipUid) setApiChipUID(detail.chipUid);
              } catch {
                // Detail fetch failed — fleet list info is sufficient
              }
            }
          }
        } catch (listErr: any) {
          if (!silent) addLog(`Fleet list error: ${listErr?.message?.substring(0, 80)}`, 'warning');
        }
      }

      if (found) {
        statusRetryRef.current = 0; // reset retry counter on success
      } else if (statusRetryRef.current < 3) {
        // Auto-retry on failure (up to 3 times with 2s gap)
        statusRetryRef.current++;
        setTimeout(() => fetchDeviceStatus(uid, silent), 2000);
      } else if (!silent) {
        setKsDeviceStatus('Not found');
        addLog('Device not found in keySTREAM fleet', 'warning');
        statusRetryRef.current = 0;
      }
    } catch (e: any) {
      if (!silent) {
        setKsDeviceStatus('Error');
        addLog(`API error: ${e?.message?.substring(0, 100)}`, 'error');
      }
    } finally {
      if (!silent) setKsLoading(false);
    }
  }, [api, realChipUID, fleetUID, tenantId]);

  // ---- API 1: Fetch fleet devices for modal ----
  const fetchFleetDevices = useCallback(async (profileUid: string) => {
    setFleetLoading(true);
    setFleetError('');
    try {
      const devices = await api.listDevices(profileUid);
      setFleetDevices(devices);
    } catch (e: any) {
      const msg = e?.message || 'Failed to load devices';
      setFleetError(msg.substring(0, 200));
      setFleetDevices([]);
    } finally {
      setFleetLoading(false);
    }
  }, [api]);

  // ---- Android hardware back ----
  useEffect(() => {
    const sub = BackHandler.addEventListener('hardwareBackPress', () => {
      if (onBack) {
        kta.disconnect().catch(() => {}).finally(() => onBack());
      }
      return true;
    });
    return () => sub.remove();
  }, [onBack, kta]);

  // ---- Connect + provision on mount ----
  useEffect(() => {
    let cancelled = false;
    const run = async () => {
      if (!bleDeviceId) {
        addLog('No BLE device selected', 'error');
        return;
      }
      try {
        setProgress(1);
        setProgressMessage('Connecting…');
        addLog(`Connecting to ${bleDeviceId.substring(0, 12)}...`, 'info');
        addLog(`Fleet: ${fleetUID || '(none)'}`, 'info');
        await kta.setConfig({ authToken, fleetProfileUID: fleetUID, apiEndpoint: serverUrl || undefined });
        const ok = await kta.connectToDevice(bleDeviceId);
        if (cancelled) return;
        setBleConnected(ok);
        if (ok) {
          addLog('BLE connected — starting provisioning', 'success');
        } else {
          addLog('BLE connect failed', 'error');
        }
      } catch (e: any) {
        if (cancelled) return;
        addLog(`Operation failed: ${e?.message || String(e)}`, 'error');
      }
    };
    run();
    return () => { cancelled = true; };
  }, [kta, bleDeviceId, authToken, fleetUID]);

  // ---- Refresh keySTREAM status once provisioned / ready ----
  useEffect(() => {
    if (deviceStatus === 'provisioned' || deviceStatus === 'ready') {
      fetchDeviceStatus();
    }
  }, [deviceStatus, fetchDeviceStatus]);

  // ---- Also fetch keySTREAM status once BLE connects (after short delay) ----
  useEffect(() => {
    if (!bleConnected) return;
    const timer = setTimeout(() => fetchDeviceStatus(), 3000);
    return () => clearTimeout(timer);
  }, [bleConnected, fetchDeviceStatus]);

  // ---- Auto-fetch when RoT UID is extracted from ICPP ----
  useEffect(() => {
    if (!realChipUID || !tenantId) return;
    addLog(`RoT UID: ${realChipUID}`, 'info');
    fetchDeviceStatus(realChipUID);
  }, [realChipUID, tenantId, fetchDeviceStatus]);

  // ---- Early fleet poll: show live status before RoT UID is known ----
  // Once realChipUID is set, the main 2s poller below takes over.
  useEffect(() => {
    if (!fleetUID || !tenantId || realChipUID) return;
    const tick = async () => {
      try {
        const devices = await api.listDevices(fleetUID);
        if (devices.length > 0) {
          // Pick the most-recently-active device if possible
          const first = devices[0];
          const st = first.deviceState || first.status || '';
          if (st && st !== lastPolledStatusRef.current) {
            lastPolledStatusRef.current = st;
            setKsDeviceStatus(st);
          }
          if (first.chipUid) setApiChipUID(first.chipUid);
        }
      } catch {
        // silent — will retry next tick
      }
    };
    tick(); // immediate first check
    const interval = setInterval(tick, 3000);
    return () => clearInterval(interval);
  }, [fleetUID, tenantId, realChipUID, api]);

  // ---- Periodic poll: refresh device status from keySTREAM ----
  // Fast initial polling (every 3s for first 30s), then slower (every 8s)
  useEffect(() => {
    if (!realChipUID || !tenantId) return;
    let elapsed = 0;
    const tick = () => {
      fetchDeviceStatus(realChipUID, true);
    };
    // Poll every 2s continuously until unmount
    const interval = setInterval(tick, 2000);
    return () => { clearInterval(interval); };
  }, [realChipUID, tenantId, fetchDeviceStatus]);

  // ---- Status badge color ----
  const statusColor = (() => {
    const s = ksDeviceStatus.toLowerCase();
    if (s === 'active' || s === 'provisioned' || s === 'onboarded') return '#27ae60';
    if (s === 'pending' || s === 'init' || s === 'sealed' || s.includes('pending')) return '#f39c12';
    if (s === 'error' || s === 'blocked') return '#e74c3c';
    return '#7f8c8d';
  })();

  // ---- Render: log row ----
  const renderLogItem = ({ item }: { item: LogEntry }) => {
    const colors: Record<string, string> = {
      info: '#8be9fd',       // cyan – general info
      success: '#50fa7b',    // green – success
      error: '#ff5555',      // red – errors
      warning: '#f1fa8c',    // yellow – warnings
      outbound: '#ff79c6',   // pink – device → keySTREAM
      inbound: '#bd93f9',    // purple – keySTREAM → device
    };
    const prefixes: Record<string, string> = {
      outbound: '▲ ',        // up arrow – sending to server
      inbound: '▼ ',         // down arrow – receiving from server
      error: '✖ ',           // cross – error
      success: '✔ ',         // check – success
      warning: '⚠ ',         // warning triangle
      info: '',
    };
    const prefix = prefixes[item.type] || '';
    return (
      <View style={styles.logItem}>
        <Text style={styles.logTimestamp}>{item.timestamp}</Text>
        <Text style={[styles.logMessage, { color: colors[item.type] || colors.info }]}>
          {prefix}{item.message}
        </Text>
      </View>
    );
  };

  // ---- Render: fleet device row ----
  const renderFleetDevice = ({ item }: { item: KeyStreamDevice }) => {
    const state = (item.deviceState || item.status || '').toLowerCase();
    const badgeColor =
      state === 'active' || state === 'onboarded' ? '#27ae60'
      : state === 'error' || state === 'blocked' ? '#e74c3c'
      : '#f39c12';
    return (
      <View style={styles.fleetCard}>
        <View style={styles.fleetRow}>
          <Text style={styles.fleetLabel}>Profile:</Text>
          <Text style={styles.fleetVal} numberOfLines={1}>{item.deviceProfilePublicUid || '—'}</Text>
        </View>
        <View style={styles.fleetRow}>
          <Text style={styles.fleetLabel}>RoT Public UID:</Text>
          <Text style={styles.fleetVal}>{item.rotPublicUid || '—'}</Text>
        </View>
        <View style={styles.fleetRow}>
          <Text style={styles.fleetLabel}>Device State:</Text>
          <View style={[styles.badge, { backgroundColor: badgeColor }]}>
            <Text style={styles.badgeText}>{item.deviceState || item.status || '—'}</Text>
          </View>
        </View>
      </View>
    );
  };

  // ===================== HEADER (two cards + progress) =====================
  const topSection = (
    <ScrollView style={styles.topScroll} contentContainerStyle={{ paddingBottom: 4 }}>
      <>
      {/* Top bar */}
      <View style={styles.header}>
        {onBack && (
          <TouchableOpacity
            style={styles.backBtn}
            activeOpacity={0.7}
            onPress={() => kta.disconnect().catch(() => {}).finally(() => onBack?.())}>
            <Text style={styles.backBtnText}>← Back</Text>
          </TouchableOpacity>
        )}
        <Text style={styles.headerTitle}>Device Status</Text>
      </View>

      {/* ===== Two side-by-side cards ===== */}
      <View style={styles.cardsRow}>

        {/* BLE Status */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>BLE Status</Text>

          <View style={styles.row}>
            <Text style={styles.label}>Connection:</Text>
            <View style={[styles.connBadge, { backgroundColor: bleConnected ? '#27ae60' : '#e74c3c' }]}>
              <Text style={styles.connBadgeText}>
                ● {bleConnected ? 'Connected' : 'Disconnected'}
              </Text>
            </View>
          </View>

          <View style={styles.row}>
            <Text style={styles.label}>Provisioning:</Text>
            <View style={[styles.connBadge, { backgroundColor:
              deviceStatus === 'provisioned' ? '#27ae60' :
              deviceStatus === 'provisioning' ? '#f39c12' :
              deviceStatus === 'error' ? '#e74c3c' :
              deviceStatus === 'initializing' || deviceStatus === 'ready' ? '#3498db' :
              '#7f8c8d'
            }]}>
              <Text style={styles.connBadgeText}>
                ● {deviceStatus === 'idle' ? 'Idle' :
                   deviceStatus === 'initializing' ? 'Initializing...' :
                   deviceStatus === 'ready' ? 'Ready' :
                   deviceStatus === 'provisioning' ? 'Provisioning...' :
                   deviceStatus === 'provisioned' ? 'Provisioned' :
                   deviceStatus === 'error' ? 'Error' : deviceStatus}
              </Text>
            </View>
          </View>

          <View style={[styles.row, { borderBottomWidth: 0 }]}>
            <Text style={styles.label}>Device:</Text>
            <Text style={styles.value} numberOfLines={1}>{bleDeviceName || '—'}</Text>
          </View>
        </View>

        {/* keySTREAM Connection */}
        <View style={styles.card}>
          <Text style={styles.cardTitle}>keySTREAM Connection</Text>

          {/* Fleet – tappable to open fleet list */}
          <TouchableOpacity
            style={styles.row}
            activeOpacity={0.6}
            onPress={() => { setFleetDevices([]); fetchFleetDevices(fleetUID); setShowFleetModal(true); }}>
            <Text style={styles.label}>Fleet:</Text>
            <Text style={[styles.value, styles.linkValue]} numberOfLines={1}>
              {fleetUID || '—'}
            </Text>
          </TouchableOpacity>

          <View style={styles.row}>
            <Text style={styles.label}>RoT UID:</Text>
            <Text style={styles.value} numberOfLines={1}>{realChipUID || '—'}</Text>
          </View>

          <View style={styles.row}>
            <Text style={styles.label}>Chip UID:</Text>
            <Text style={styles.value} numberOfLines={1}>{displayChipUID}</Text>
          </View>

          <View style={[styles.row, { borderBottomWidth: 0 }]}>
            <Text style={styles.label}>Device Status:</Text>
            {ksLoading ? (
              <ActivityIndicator size="small" color="#3498db" />
            ) : (
              <View style={[styles.connBadge, { backgroundColor: statusColor }]}>
                <Text style={styles.connBadgeText}>● {ksDeviceStatus}</Text>
              </View>
            )}
          </View>
        </View>
      </View>

      {/* Logs heading */}
      <View style={styles.section}>
        <View style={styles.logsHead}>
          <Text style={styles.sectionTitle}>Logs</Text>
          <TouchableOpacity style={styles.clearBtn} onPress={() => setLogs([])}>
            <Text style={styles.clearBtnText}>Clear</Text>
          </TouchableOpacity>
        </View>
      </View>
      </>
    </ScrollView>
  );

  // ===================== MAIN RETURN =====================
  return (
    <View style={{ flex: 1, backgroundColor: '#f5f5f5' }}>
      {/* Top section: cards + progress (scrollable if content is tall) */}
      {topSection}

      {/* Bottom section: dedicated auto-scrolling log panel */}
      <View style={styles.logPanel}>
        <FlatList
          ref={logListRef}
          data={logs}
          renderItem={renderLogItem}
          keyExtractor={(_, i) => String(i)}
          ListEmptyComponent={<Text style={styles.emptyTxt}>No logs yet</Text>}
          showsVerticalScrollIndicator
          onContentSizeChange={() => logListRef.current?.scrollToEnd({ animated: true })}
          onLayout={() => logListRef.current?.scrollToEnd({ animated: false })}
          style={styles.logList}
          contentContainerStyle={{ paddingBottom: 8 }}
        />
      </View>

      {/* ===== Fleet Devices Modal ===== */}
      <Modal
        visible={showFleetModal}
        animationType="slide"
        transparent
        onRequestClose={() => setShowFleetModal(false)}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalBox}>
            <View style={styles.modalHead}>
              <Text style={styles.modalTitle}>
                Devices — {fleetUID || 'Fleet'}
              </Text>
              <TouchableOpacity onPress={() => setShowFleetModal(false)} style={styles.modalClose}>
                <Text style={styles.modalCloseText}>✕</Text>
              </TouchableOpacity>
            </View>

            {fleetLoading ? (
              <View style={styles.modalCenter}>
                <ActivityIndicator size="large" color="#3498db" />
                <Text style={styles.modalHint}>Loading devices…</Text>
              </View>
            ) : fleetError ? (
              <View style={styles.modalCenter}>
                <Text style={[styles.modalHint, { color: '#e74c3c', textAlign: 'center' }]}>{fleetError}</Text>
                <TouchableOpacity
                  style={{ marginTop: 16, paddingVertical: 10, paddingHorizontal: 24, backgroundColor: '#3498db', borderRadius: 6 }}
                  onPress={() => fetchFleetDevices(fleetUID)}>
                  <Text style={{ color: '#fff', fontWeight: 'bold' }}>Retry</Text>
                </TouchableOpacity>
              </View>
            ) : (
              <FlatList
                data={fleetDevices}
                renderItem={renderFleetDevice}
                keyExtractor={(d, i) => d.deviceId || d.rotPublicUid || String(i)}
                contentContainerStyle={{ padding: 14 }}
                ListEmptyComponent={
                  <View style={styles.modalCenter}>
                    <Text style={styles.modalHint}>No devices found</Text>
                    <TouchableOpacity
                      style={{ marginTop: 16, paddingVertical: 10, paddingHorizontal: 24, backgroundColor: '#3498db', borderRadius: 6 }}
                      onPress={() => fetchFleetDevices(fleetUID)}>
                      <Text style={{ color: '#fff', fontWeight: 'bold' }}>Retry</Text>
                    </TouchableOpacity>
                  </View>
                }
              />
            )}
          </View>
        </View>
      </Modal>
    </View>
  );
};

// ===================== STYLES =====================
const styles = StyleSheet.create({
  container: { flex: 1, backgroundColor: '#f5f5f5' },
  content: { paddingBottom: 24 },
  topScroll: { flexGrow: 0, flexShrink: 0, backgroundColor: '#f5f5f5' },

  /* Header */
  header: {
    backgroundColor: '#2c3e50',
    padding: 20,
    flexDirection: 'row',
    alignItems: 'center',
  },
  headerTitle: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
    flex: 1,
    textAlign: 'center',
    marginLeft: -60,
  },
  backBtn: { paddingHorizontal: 12, paddingVertical: 8, zIndex: 10 },
  backBtnText: { color: '#fff', fontSize: 16, fontWeight: '600' },

  /* Cards */
  cardsRow: { flexDirection: 'row', paddingHorizontal: 8, paddingTop: 12 },
  card: {
    flex: 1,
    backgroundColor: '#fff',
    marginHorizontal: 4,
    padding: 14,
    borderRadius: 8,
    borderLeftWidth: 4,
    borderLeftColor: '#27ae60',
    elevation: 3,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
  },
  cardTitle: { fontSize: 14, fontWeight: 'bold', color: '#2c3e50', marginBottom: 10 },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    paddingVertical: 7,
    borderBottomWidth: 1,
    borderBottomColor: '#ecf0f1',
  },
  label: { fontSize: 12, fontWeight: '700', color: '#34495e' },
  value: { fontSize: 12, color: '#2c3e50', fontWeight: '500', flexShrink: 1, textAlign: 'right', marginLeft: 4 },
  linkValue: { color: '#2980b9', textDecorationLine: 'underline' },
  connBadge: { paddingHorizontal: 8, paddingVertical: 2, borderRadius: 10 },
  connBadgeText: { color: '#fff', fontSize: 11, fontWeight: 'bold' },

  /* Progress */
  progressWrap: { marginHorizontal: 12, marginTop: 10, backgroundColor: '#fff', borderRadius: 8, padding: 10, elevation: 2 },
  progressBg: { height: 5, backgroundColor: '#ecf0f1', borderRadius: 3, overflow: 'hidden' },
  progressFill: { height: 5, backgroundColor: '#27ae60', borderRadius: 3 },
  progressTxt: { fontSize: 11, color: '#7f8c8d', marginTop: 4, textAlign: 'center' },

  /* Section */
  section: { backgroundColor: '#fff', margin: 10, padding: 14, borderRadius: 8, elevation: 3 },
  sectionTitle: { fontSize: 15, fontWeight: 'bold', color: '#2c3e50' },

  /* Logs */
  logPanel: {
    flex: 1,
    backgroundColor: '#1e1e2e',
    marginHorizontal: 10,
    marginBottom: 10,
    borderRadius: 8,
    overflow: 'hidden',
    elevation: 4,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.15,
    shadowRadius: 4,
  },
  logList: { flex: 1 },
  logsHead: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center' },
  clearBtn: { paddingHorizontal: 10, paddingVertical: 5, borderRadius: 4, backgroundColor: '#e74c3c' },
  clearBtnText: { color: '#fff', fontSize: 11, fontWeight: '600' },
  logItem: { paddingVertical: 4, paddingHorizontal: 12, borderBottomWidth: StyleSheet.hairlineWidth, borderBottomColor: '#2d2d44' },
  logTimestamp: { fontSize: 10, color: '#636e88', marginBottom: 1, fontFamily: 'monospace' },
  logMessage: { fontSize: 12, fontWeight: '500', fontFamily: 'monospace' },
  emptyTxt: { textAlign: 'center', color: '#636e88', padding: 20, fontFamily: 'monospace' },

  /* Fleet Modal */
  modalOverlay: { flex: 1, backgroundColor: 'rgba(0,0,0,0.5)', justifyContent: 'center', alignItems: 'center' },
  modalBox: { backgroundColor: '#fff', borderRadius: 12, width: '92%', maxHeight: '75%', overflow: 'hidden' },
  modalHead: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', padding: 16, backgroundColor: '#2c3e50' },
  modalTitle: { fontSize: 18, fontWeight: 'bold', color: '#fff' },
  modalClose: { padding: 5 },
  modalCloseText: { fontSize: 22, color: '#fff', fontWeight: 'bold' },
  modalCenter: { padding: 40, alignItems: 'center' },
  modalHint: { marginTop: 10, fontSize: 14, color: '#7f8c8d' },
  fleetCard: { backgroundColor: '#f8f9fa', padding: 12, borderRadius: 8, marginBottom: 10, borderWidth: 1, borderColor: '#e1e8ed' },
  fleetRow: { flexDirection: 'row', justifyContent: 'space-between', alignItems: 'center', marginBottom: 5 },
  fleetLabel: { fontSize: 12, fontWeight: '600', color: '#34495e' },
  fleetVal: { fontSize: 12, color: '#2c3e50', flexShrink: 1, textAlign: 'right', marginLeft: 8 },
  badge: { paddingHorizontal: 10, paddingVertical: 2, borderRadius: 10 },
  badgeText: { color: '#fff', fontSize: 11, fontWeight: 'bold' },
});
