import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  TextInput,
  TouchableOpacity,
  StyleSheet,
  ScrollView,
  Modal,
  FlatList,
  Alert,
  ActivityIndicator,
} from 'react-native';
import AsyncStorage from '@react-native-async-storage/async-storage';
import { BLEScanModal } from '../components/BLEScanModal';
import { KeyStreamAPI, KeyStreamDevice } from '../services/KeyStreamAPI';

type ServerEnv = 'prod' | 'cie';

const SERVER_ENVS: { key: ServerEnv; label: string }[] = [
  { key: 'prod', label: 'Prod' },
  { key: 'cie',  label: 'Cie' },
];

const ENV_HTTP_HOST: Record<ServerEnv, string> = {
  prod: 'icph.mss.iot.kudelski.com',
  cie:  'icph.cie.iot-lab.kudelski.com',
};

function buildServerUrl(env: ServerEnv): string {
  return `http://${ENV_HTTP_HOST[env]}/lp1`;
}

interface MainConfigScreenProps {
  onSubmit?: (config: {
    authToken: string;
    fleetProfileUID: string;
    tenantId: string;
    passkey: string;
    serverUrl: string;
    apiBaseUrl: string;
  }) => void;
}

export const MainConfigScreen: React.FC<MainConfigScreenProps> = ({ onSubmit }) => {
  const [authToken, setAuthToken] = useState('');
  const [showAuthToken, setShowAuthToken] = useState(false);
  const [selectedEnv, setSelectedEnv] = useState<ServerEnv>('prod');
  const [showEnvDropdown, setShowEnvDropdown] = useState(false);
  const [fleetProfileUID, setFleetProfileUID] = useState('');
  const [selectedDevice, setSelectedDevice] = useState<string | null>(null);
  const [showBLEModal, setShowBLEModal] = useState(false);
  const [passkey, setPasskey] = useState('');

  // Tenant resolution state (API 0)
  const [tenantId, setTenantId] = useState('');
  const [tenantResolved, setTenantResolved] = useState(false);
  const [tenantLoading, setTenantLoading] = useState(false);
  const [tenantError, setTenantError] = useState('');

  // Device list state (API 1 + API 2)
  const [devices, setDevices] = useState<KeyStreamDevice[]>([]);
  const [showDeviceList, setShowDeviceList] = useState(false);
  const [devicesLoading, setDevicesLoading] = useState(false);
  const [devicesError, setDevicesError] = useState('');

  // Load saved config on mount
  useEffect(() => {
    const loadSavedConfig = async () => {
      try {
        const configStr = await AsyncStorage.getItem('kta_config');
        if (configStr) {
          const config = JSON.parse(configStr);
          if (config.authToken) setAuthToken(config.authToken);
          if (config.fleetProfileUID) setFleetProfileUID(config.fleetProfileUID);
        }
      } catch (error) {

      }
    };
    loadSavedConfig();
  }, []);

  // ---- Fleet Profile Submit (API 0: resolve tenant) ----
  const handleFleetSubmit = async () => {
    if (!authToken.trim()) {
      Alert.alert('Validation', 'Please enter your auth token first');
      return;
    }
    if (!fleetProfileUID.trim()) {
      Alert.alert('Validation', 'Please enter Fleet Profile UID');
      return;
    }
    setTenantLoading(true);
    setTenantError('');
    setTenantResolved(false);
    setDevices([]);
    try {
      const apiBaseUrl = `https://${ENV_HTTP_HOST[selectedEnv].replace(/^icp[hp]\./, '')}`;
      const api = new KeyStreamAPI(authToken.trim(), undefined, apiBaseUrl);
      const result = await api.lookupTenant(fleetProfileUID.trim());
      setTenantId(result.uuid);
      setTenantResolved(true);
    } catch (e: any) {
      const msg = e?.message || 'Failed to resolve tenant';
      setTenantError(msg);
      Alert.alert('Error', msg);
    } finally {
      setTenantLoading(false);
    }
  };

  // ---- Cancel / reset fleet state ----
  const handleFleetCancel = () => {
    setTenantResolved(false);
    setTenantId('');
    setTenantError('');
    setDevices([]);
    setFleetProfileUID('');
  };

  // ---- Get Device List (API 1 + API 2) ----
  const handleGetDeviceList = async () => {
    if (!tenantId) {
      Alert.alert('Error', 'Please submit Fleet Profile UID first');
      return;
    }
    setDevicesLoading(true);
    setDevicesError('');
    try {
      const apiBaseUrl = `https://${ENV_HTTP_HOST[selectedEnv].replace(/^icp[hp]\./, '')}`;
      const api = new KeyStreamAPI(authToken.trim(), tenantId, apiBaseUrl);
      const list = await api.listDevices(fleetProfileUID.trim());

      // For each device, fetch detailed info via API 2
      const detailed: KeyStreamDevice[] = [];
      for (const device of list) {
        try {
          const deviceKey = device.rotPublicUid || device.deviceId;
          if (deviceKey) {
            const info = await api.getDeviceInfo(deviceKey);
            detailed.push({ ...device, ...info });
          } else {
            detailed.push(device);
          }
        } catch {
          detailed.push(device);
        }
      }
      setDevices(detailed);
      setShowDeviceList(true);
    } catch (e: any) {
      const msg = e?.message || 'Failed to fetch device list';
      setDevicesError(msg);
      Alert.alert('Error', msg);
    } finally {
      setDevicesLoading(false);
    }
  };

  const handleSubmit = () => {
    // Validate inputs
    if (!authToken.trim()) {
      Alert.alert('Validation Error', 'Please enter Auth Token');
      return;
    }
    if (!fleetProfileUID.trim()) {
      Alert.alert('Validation Error', 'Please enter Fleet Profile UID');
      return;
    }
    if (!selectedDevice) {
      Alert.alert('Validation Error', 'Please select a Bluetooth device');
      return;
    }
    const serverUrl = buildServerUrl(selectedEnv);
    const apiBaseUrl = `https://${ENV_HTTP_HOST[selectedEnv].replace(/^icp[hp]\./, '')}`;

    // Call the onSubmit callback with configuration
    onSubmit?.({
      authToken,
      fleetProfileUID,
      tenantId,
      selectedDevice,
      passkey,
      serverUrl,
      apiBaseUrl,
    });
  };

  return (
    <ScrollView style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.headerText}>keySTREAM provisioning app</Text>
        <Text style={{fontSize: 12, color: '#95a5a6', marginTop: 4}}>v2.1.0</Text>
      </View>

      {/* App Configuration Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>App Configuration</Text>
        
        <View style={styles.inputGroup}>
          <Text style={styles.label}>keySTREAM auth token</Text>
          <View style={styles.passwordContainer}>
            <TextInput
              style={styles.passwordInput}
              value={authToken}
              onChangeText={setAuthToken}
              secureTextEntry={!showAuthToken}
              placeholder="Enter your authentication token"
              placeholderTextColor="#999"
            />
            <TouchableOpacity
              style={styles.eyeButton}
              onPress={() => setShowAuthToken(!showAuthToken)}>
              <Text style={styles.eyeIcon}>{showAuthToken ? 'Hide' : 'Show'}</Text>
            </TouchableOpacity>
          </View>
        </View>
      </View>

      {/* KTA Configuration Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>KTA Configuration</Text>

        {/* Server Environment Dropdown */}
        <View style={styles.inputGroup}>
          <Text style={styles.label}>Onboarding Server</Text>
          <TouchableOpacity
            style={styles.dropdownBtn}
            onPress={() => setShowEnvDropdown(true)}
            activeOpacity={0.8}>
            <Text style={styles.dropdownBtnText}>
              {SERVER_ENVS.find(e => e.key === selectedEnv)?.label ?? 'Prod'}
            </Text>
            <Text style={styles.dropdownArrow}>?</Text>
          </TouchableOpacity>
        </View>

        {/* Environment Dropdown Modal */}
        <Modal
          visible={showEnvDropdown}
          animationType="fade"
          transparent
          onRequestClose={() => setShowEnvDropdown(false)}>
          <TouchableOpacity
            style={styles.dropdownOverlay}
            activeOpacity={1}
            onPress={() => setShowEnvDropdown(false)}>
            <View style={styles.dropdownMenu}>
              {SERVER_ENVS.map(env => (
                <TouchableOpacity
                  key={env.key}
                  style={[
                    styles.dropdownItem,
                    selectedEnv === env.key && styles.dropdownItemSelected,
                  ]}
                  onPress={() => {
                    setSelectedEnv(env.key);
                    setShowEnvDropdown(false);
                  }}>
                  <Text
                    style={[
                      styles.dropdownItemText,
                      selectedEnv === env.key && styles.dropdownItemTextSelected,
                    ]}>
                    {env.label}
                  </Text>
                  {selectedEnv === env.key && (
                    <Text style={styles.dropdownCheck}>?</Text>
                  )}
                </TouchableOpacity>
              ))}
            </View>
          </TouchableOpacity>
        </Modal>
        
        <View style={styles.inputGroup}>
          <Text style={styles.label}>Fleet Profile UID</Text>
          <TextInput
            style={styles.textInput}
            value={fleetProfileUID}
            onChangeText={(text) => {
              setFleetProfileUID(text);
              if (tenantResolved) {
                setTenantResolved(false);
                setTenantId('');
                setDevices([]);
                setTenantError('');
              }
            }}
            placeholder="Enter fleet profile identifier"
            placeholderTextColor="#999"
          />
        </View>

        {/* Submit / Cancel for fleet profile resolution */}
        <View style={styles.fleetButtonRow}>
          <TouchableOpacity
            style={[styles.fleetSubmitBtn, tenantLoading && { opacity: 0.6 }]}
            onPress={handleFleetSubmit}
            disabled={tenantLoading}>
            <Text style={styles.fleetSubmitText}>
              {tenantLoading ? 'Resolving...' : 'Submit'}
            </Text>
          </TouchableOpacity>
          <TouchableOpacity style={styles.fleetCancelBtn} onPress={handleFleetCancel}>
            <Text style={styles.fleetCancelText}>Cancel</Text>
          </TouchableOpacity>
        </View>

        {tenantError ? (
          <Text style={styles.errorMsg}>{tenantError}</Text>
        ) : null}

        {/* Get Device List – appears after tenant is resolved */}
        {tenantResolved && (
          <TouchableOpacity
            style={[styles.getDeviceListBtn, devicesLoading && { opacity: 0.6 }]}
            onPress={handleGetDeviceList}
            disabled={devicesLoading}>
            <Text style={styles.getDeviceListText}>
              {devicesLoading ? 'Loading…' : 'Get Device List'}
            </Text>
          </TouchableOpacity>
        )}
      </View>

      {/* Bluetooth Connections Section */}
      <View style={styles.section}>
        <Text style={styles.sectionTitle}>Bluetooth Connections</Text>
        
        <View style={styles.inputGroup}>
          <Text style={styles.label}>Selected Device</Text>
          <View style={styles.deviceContainer}>
            <View style={styles.deviceDisplay}>
              {selectedDevice ? (
                <View style={styles.selectedDeviceRow}>
                  <Text style={styles.deviceText}>{selectedDevice}</Text>
                  <Text style={styles.checkmark}>?</Text>
                </View>
              ) : (
                <Text style={styles.placeholderText}>No device selected</Text>
              )}
            </View>
            <TouchableOpacity
              style={styles.scanButton}
              onPress={() => {
                setShowBLEModal(true);
              }}>
              <Text style={styles.scanButtonText}>Scan Bluetooth</Text>
            </TouchableOpacity>
          </View>
        </View>

        {selectedDevice && (
          <View style={styles.inputGroup}>
            <Text style={styles.label}>Passkey / PIN</Text>
            <TextInput
              style={styles.textInput}
              value={passkey}
              onChangeText={setPasskey}
              placeholder="Enter passkey or PIN"
              placeholderTextColor="#999"
              keyboardType="numeric"
              secureTextEntry
            />
          </View>
        )}
      </View>

      {/* Action Buttons */}
      <View style={styles.buttonRow}>
        <TouchableOpacity style={styles.cancelButton}>
          <Text style={styles.cancelButtonText}>Cancel</Text>
        </TouchableOpacity>
        <TouchableOpacity style={styles.submitButton} onPress={handleSubmit}>
          <Text style={styles.submitButtonText}>Onboard Device</Text>
        </TouchableOpacity>
      </View>

      {/* Device List Modal */}
      <Modal
        visible={showDeviceList}
        animationType="slide"
        transparent
        onRequestClose={() => setShowDeviceList(false)}>
        <View style={styles.modalOverlay}>
          <View style={styles.modalBox}>
            <View style={styles.modalHead}>
              <Text style={styles.modalTitle}>Devices - {fleetProfileUID}</Text>
              <TouchableOpacity onPress={() => setShowDeviceList(false)} style={styles.modalClose}>
                <Text style={styles.modalCloseText}>X</Text>
              </TouchableOpacity>
            </View>
            {devices.length === 0 ? (
              <View style={styles.modalCenter}>
                <Text style={styles.modalHint}>No devices found</Text>
              </View>
            ) : (
              <FlatList
                data={devices}
                keyExtractor={(d, i) => d.deviceId || String(i)}
                contentContainerStyle={{ padding: 14 }}
                renderItem={({ item }) => {
                  const state = (item.deviceState || item.status || '').toLowerCase();
                  const badgeColor =
                    state === 'active' || state === 'onboarded' ? '#27ae60'
                    : state === 'error' || state === 'blocked' ? '#e74c3c'
                    : '#f39c12';
                  return (
                    <View style={styles.deviceCard}>
                      <View style={styles.deviceRow}>
                        <Text style={styles.deviceLabel}>deviceProfilePublicUid:</Text>
                        <Text style={styles.deviceValue}>{item.deviceProfilePublicUid || '—'}</Text>
                      </View>
                      <View style={styles.deviceRow}>
                        <Text style={styles.deviceLabel}>rotPublicUid:</Text>
                        <Text style={styles.deviceValue}>{item.rotPublicUid || '—'}</Text>
                      </View>
                      <View style={styles.deviceRow}>
                        <Text style={styles.deviceLabel}>deviceState:</Text>
                        <View style={[styles.statusBadge, { backgroundColor: badgeColor }]}>
                          <Text style={styles.statusText}>{item.deviceState || item.status || '—'}</Text>
                        </View>
                      </View>
                    </View>
                  );
                }}
              />
            )}
          </View>
        </View>
      </Modal>

      <BLEScanModal
        visible={showBLEModal}
        onClose={() => setShowBLEModal(false)}
        onSelectDevice={(device) => {
          setSelectedDevice(device);
          setShowBLEModal(false);
        }}
      />
    </ScrollView>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  header: {
    backgroundColor: '#2c3e50',
    padding: 20,
    alignItems: 'center',
  },
  headerText: {
    fontSize: 24,
    fontWeight: 'bold',
    color: '#fff',
  },
  section: {
    backgroundColor: '#fff',
    margin: 10,
    padding: 15,
    borderRadius: 8,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 2 },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  sectionTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#2c3e50',
    marginBottom: 15,
  },
  inputGroup: {
    marginBottom: 15,
  },
  label: {
    fontSize: 14,
    fontWeight: '600',
    color: '#34495e',
    marginBottom: 8,
  },
  passwordContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 6,
    backgroundColor: '#fff',
  },
  passwordInput: {
    flex: 1,
    padding: 12,
    fontSize: 16,
    color: '#2c3e50',
  },
  eyeButton: {
    padding: 12,
  },
  eyeIcon: {
    fontSize: 20,
  },
  inputRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  textInput: {
    flex: 1,
    padding: 12,
    fontSize: 16,
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 6,
    backgroundColor: '#fff',
    color: '#2c3e50',
  },
  smallButton: {
    marginLeft: 10,
    padding: 12,
    backgroundColor: '#3498db',
    borderRadius: 6,
  },
  smallButtonText: {
    fontSize: 20,
  },
  deviceContainer: {
    gap: 10,
  },
  deviceDisplay: {
    padding: 12,
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 6,
    backgroundColor: '#fff',
    minHeight: 50,
    justifyContent: 'center',
  },
  selectedDeviceRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  deviceText: {
    fontSize: 16,
    color: '#2c3e50',
  },
  checkmark: {
    fontSize: 24,
    color: '#27ae60',
    fontWeight: 'bold',
  },
  placeholderText: {
    fontSize: 16,
    color: '#999',
  },
  scanButton: {
    backgroundColor: '#3498db',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
  },
  scanButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '600',
  },
  buttonRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    padding: 20,
    gap: 10,
  },
  cancelButton: {
    flex: 1,
    padding: 15,
    borderRadius: 6,
    borderWidth: 2,
    borderColor: '#e74c3c',
    alignItems: 'center',
  },
  cancelButtonText: {
    color: '#e74c3c',
    fontSize: 16,
    fontWeight: 'bold',
  },
  submitButton: {
    flex: 1,
    padding: 15,
    borderRadius: 6,
    backgroundColor: '#27ae60',
    alignItems: 'center',
  },
  submitButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  // Fleet Submit / Cancel row
  fleetButtonRow: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 12,
    marginTop: 5,
    marginBottom: 10,
  },
  fleetSubmitBtn: {
    flex: 1,
    paddingVertical: 12,
    backgroundColor: '#3498db',
    borderRadius: 6,
    alignItems: 'center',
  },
  fleetSubmitText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: 'bold',
  },
  fleetCancelBtn: {
    flex: 1,
    paddingVertical: 12,
    borderRadius: 6,
    borderWidth: 2,
    borderColor: '#3498db',
    alignItems: 'center',
  },
  fleetCancelText: {
    color: '#3498db',
    fontSize: 15,
    fontWeight: 'bold',
  },
  getDeviceListBtn: {
    paddingVertical: 12,
    backgroundColor: '#7f8c8d',
    borderRadius: 6,
    alignItems: 'center',
    marginTop: 4,
  },
  getDeviceListText: {
    color: '#fff',
    fontSize: 15,
    fontWeight: '600',
  },
  errorMsg: {
    color: '#e74c3c',
    fontSize: 13,
    textAlign: 'center',
    marginBottom: 8,
  },
  // Device List Modal
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.5)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalBox: {
    backgroundColor: '#fff',
    borderRadius: 12,
    width: '92%',
    maxHeight: '80%',
    overflow: 'hidden',
  },
  modalHead: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 16,
    backgroundColor: '#2c3e50',
  },
  modalTitle: {
    fontSize: 18,
    fontWeight: 'bold',
    color: '#fff',
    flex: 1,
  },
  modalClose: {
    padding: 5,
  },
  modalCloseText: {
    fontSize: 22,
    color: '#fff',
    fontWeight: 'bold',
  },
  modalCenter: {
    padding: 40,
    alignItems: 'center',
  },
  modalHint: {
    fontSize: 14,
    color: '#7f8c8d',
  },
  deviceCard: {
    backgroundColor: '#f8f9fa',
    padding: 14,
    borderRadius: 8,
    marginBottom: 12,
    borderWidth: 1,
    borderColor: '#e1e8ed',
  },
  deviceRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    marginBottom: 6,
  },
  deviceLabel: {
    fontSize: 12,
    fontWeight: '600',
    color: '#34495e',
  },
  deviceValue: {
    fontSize: 12,
    color: '#2c3e50',
    flex: 1,
    textAlign: 'right',
    marginLeft: 8,
  },
  statusBadge: {
    paddingHorizontal: 10,
    paddingVertical: 3,
    borderRadius: 10,
  },
  statusText: {
    color: '#fff',
    fontSize: 11,
    fontWeight: 'bold',
  },
  // Server environment dropdown
  dropdownBtn: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    borderWidth: 1,
    borderColor: '#ddd',
    borderRadius: 6,
    backgroundColor: '#fff',
    paddingHorizontal: 12,
    paddingVertical: 12,
  },
  dropdownBtnText: {
    fontSize: 16,
    color: '#2c3e50',
  },
  dropdownArrow: {
    fontSize: 18,
    color: '#7f8c8d',
  },
  dropdownOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0,0,0,0.35)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  dropdownMenu: {
    backgroundColor: '#fff',
    borderRadius: 10,
    width: 240,
    overflow: 'hidden',
    elevation: 8,
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 4 },
    shadowOpacity: 0.2,
    shadowRadius: 8,
  },
  dropdownItem: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingVertical: 14,
    paddingHorizontal: 18,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  dropdownItemSelected: {
    backgroundColor: '#eaf4fb',
  },
  dropdownItemText: {
    fontSize: 16,
    color: '#2c3e50',
    flex: 1,
  },
  dropdownItemTextSelected: {
    fontWeight: 'bold',
    color: '#2980b9',
  },
  dropdownCheck: {
    fontSize: 16,
    color: '#27ae60',
    fontWeight: 'bold',
  },
  urlPreview: {
    fontSize: 11,
    color: '#7f8c8d',
    marginTop: 4,
    fontFamily: 'monospace',
  },
});
