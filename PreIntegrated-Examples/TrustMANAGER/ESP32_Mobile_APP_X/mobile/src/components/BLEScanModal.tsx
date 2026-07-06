import React, { useState, useEffect } from 'react';
import {
  View,
  Text,
  Modal,
  StyleSheet,
  TouchableOpacity,
  FlatList,
  ActivityIndicator,
  PermissionsAndroid,
  Platform,
} from 'react-native';
import { Device } from 'react-native-ble-plx';
import { bleManager } from '../services/bleManager';

interface BLEScanModalProps {
  visible: boolean;
  onClose: () => void;
  onSelectDevice: (deviceName: string) => void;
}

export const BLEScanModal: React.FC<BLEScanModalProps> = ({
  visible,
  onClose,
  onSelectDevice,
}) => {
  const [devices, setDevices] = useState<Device[]>([]);
  const [scanning, setScanning] = useState(false);
  const [connectedDeviceId, setConnectedDeviceId] = useState<string | null>(null);
  const [busyDeviceId, setBusyDeviceId] = useState<string | null>(null);
  const [scanError, setScanError] = useState<string | null>(null);

  useEffect(() => {
    if (visible) {
      startScan();
    } else {
      stopScan();
    }
    
    return () => {
      stopScan();
    };
  }, [visible]);

  // Do not destroy the shared BLE manager here.

  const requestPermissions = async () => {
    if (Platform.OS === 'android') {
      if (Platform.Version >= 31) {
        // Android 12+ uses Nearby Devices permissions. Do not block scan on Location permission here.
        const granted = await PermissionsAndroid.requestMultiple([
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
          PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        ]);
        return (
          granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN] === PermissionsAndroid.RESULTS.GRANTED &&
          granted[PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT] === PermissionsAndroid.RESULTS.GRANTED
        );
      } else {
        const granted = await PermissionsAndroid.request(
          PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION
        );
        return granted === PermissionsAndroid.RESULTS.GRANTED;
      }
    }
    return true;
  };

  const getDeviceLabel = (device: Device) => {
    const name = (device as any)?.localName || device.name;
    return name || 'Unknown Device';
  };

  const startScan = async () => {
    setScanError(null);

    // Reset any previous scan that might still be running.
    try {
      bleManager.stopDeviceScan();
    } catch {
      // ignore
    }

    const hasPermission = await requestPermissions();
    if (!hasPermission) {
      setScanError('Bluetooth permissions not granted. Enable Nearby Devices permission for this app.');
      return;
    }

    try {
      const state = await bleManager.state();
      if (state !== 'PoweredOn') {
        setScanError(`Bluetooth is not ON (state: ${state}). Please turn on Bluetooth.`);
        return;
      }
    } catch {
      // ignore
    }

    setDevices([]);
    setScanning(true);

    bleManager.startDeviceScan(null, null, (error, device) => {
      if (error) {
        console.error('Scan error:', error);
        setScanning(false);
        setScanError(`Scan error: ${error.message}`);
        return;
      }

      if (device) {
        setDevices((prevDevices) => {
          const exists = prevDevices.find((d) => d.id === device.id);
          if (exists) {
            return prevDevices;
          }
          return [...prevDevices, device];
        });
      }
    });

    // Stop scanning after 10 seconds
    setTimeout(() => {
      stopScan();
    }, 15000);
  };

  const stopScan = () => {
    bleManager.stopDeviceScan();
    setScanning(false);
  };

  const handleSelectDevice = (device: Device) => {
    stopScan();
    onSelectDevice(`${getDeviceLabel(device)} (${device.id})`);
  };

  const handleConnect = async (device: Device) => {
    try {
      setBusyDeviceId(device.id);
      stopScan();
      const connected = await bleManager.connectToDevice(device.id, { requestMTU: 247 });
      await connected.discoverAllServicesAndCharacteristics();
      setConnectedDeviceId(device.id);
      onSelectDevice(`${getDeviceLabel(device)} (${device.id})`);
    } catch (e: any) {
      console.error('Connect error:', e);
      setScanError(`Connect error: ${e?.message || String(e)}`);
    } finally {
      setBusyDeviceId(null);
    }
  };

  const handleDisconnect = async (deviceId: string) => {
    try {
      setBusyDeviceId(deviceId);
      await bleManager.cancelDeviceConnection(deviceId);
    } catch (e: any) {
      console.error('Disconnect error:', e);
    } finally {
      setConnectedDeviceId((prev) => (prev === deviceId ? null : prev));
      setBusyDeviceId(null);
    }
  };

  const renderDevice = ({ item }: { item: Device }) => {
    const isConnected = connectedDeviceId === item.id;
    const isBusy = busyDeviceId === item.id;

    return (
      <View style={styles.deviceItem}>
        <TouchableOpacity
          style={styles.devicePressArea}
          onPress={() => handleSelectDevice(item)}
          disabled={isBusy}>
          <View style={styles.deviceInfo}>
            <Text style={styles.deviceName}>{getDeviceLabel(item)}</Text>
            <Text style={styles.deviceAddress}>{item.id}</Text>
          </View>
          <View style={styles.signalStrength}>
            <Text style={styles.rssi}>{item.rssi || 'N/A'} dBm</Text>
          </View>
        </TouchableOpacity>

        <TouchableOpacity
          style={[styles.connButton, isConnected ? styles.disconnectButton : styles.connectButton, isBusy ? styles.connButtonDisabled : null]}
          onPress={() => (isConnected ? handleDisconnect(item.id) : handleConnect(item))}
          disabled={isBusy}>
          <Text style={styles.connButtonText}>
            {isBusy ? '...' : isConnected ? 'Disconnect' : 'Connect'}
          </Text>
        </TouchableOpacity>
      </View>
    );
  };

  return (
    <Modal
      visible={visible}
      animationType="slide"
      transparent={true}
      onRequestClose={onClose}>
      <View style={styles.modalOverlay}>
        <View style={styles.modalContent}>
          <View style={styles.modalHeader}>
            <Text style={styles.modalTitle}>Scan Bluetooth Devices</Text>
            <TouchableOpacity onPress={onClose} style={styles.closeButton}>
              <Text style={styles.closeButtonText}>✕</Text>
            </TouchableOpacity>
          </View>

          <View style={styles.scanControls}>
            {scanning ? (
              <View style={styles.scanningIndicator}>
                <ActivityIndicator size="small" color="#3498db" />
                <Text style={styles.scanningText}>Scanning...</Text>
                <TouchableOpacity
                  style={styles.stopButton}
                  onPress={stopScan}>
                  <Text style={styles.stopButtonText}>Stop</Text>
                </TouchableOpacity>
              </View>
            ) : (
              <TouchableOpacity
                style={styles.rescanButton}
                onPress={startScan}>
                <Text style={styles.rescanButtonText}>🔄 Rescan</Text>
              </TouchableOpacity>
            )}
          </View>

          <FlatList
            data={devices}
            renderItem={renderDevice}
            keyExtractor={(item) => item.id}
            contentContainerStyle={styles.listContainer}
            ListEmptyComponent={
              <View style={styles.emptyContainer}>
                <Text style={styles.emptyText}>
                  {scanning ? 'Searching for devices...' : 'No devices found'}
                </Text>
                {!!scanError && <Text style={styles.errorText}>{scanError}</Text>}
              </View>
            }
          />
        </View>
      </View>
    </Modal>
  );
};

const styles = StyleSheet.create({
  modalOverlay: {
    flex: 1,
    backgroundColor: 'rgba(0, 0, 0, 0.5)',
    justifyContent: 'center',
    alignItems: 'center',
  },
  modalContent: {
    backgroundColor: '#fff',
    borderRadius: 12,
    width: '90%',
    maxHeight: '80%',
    overflow: 'hidden',
  },
  modalHeader: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: 20,
    backgroundColor: '#2c3e50',
  },
  modalTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    color: '#fff',
  },
  closeButton: {
    padding: 5,
  },
  closeButtonText: {
    fontSize: 24,
    color: '#fff',
    fontWeight: 'bold',
  },
  scanControls: {
    padding: 15,
    borderBottomWidth: 1,
    borderBottomColor: '#e1e8ed',
  },
  scanningIndicator: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  scanningText: {
    fontSize: 16,
    color: '#3498db',
    marginLeft: 10,
    flex: 1,
  },
  stopButton: {
    backgroundColor: '#e74c3c',
    paddingHorizontal: 20,
    paddingVertical: 8,
    borderRadius: 6,
  },
  stopButtonText: {
    color: '#fff',
    fontWeight: 'bold',
  },
  rescanButton: {
    backgroundColor: '#3498db',
    padding: 12,
    borderRadius: 6,
    alignItems: 'center',
  },
  rescanButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: 'bold',
  },
  listContainer: {
    padding: 15,
  },
  deviceItem: {
    flexDirection: 'row',
    alignItems: 'center',
    backgroundColor: '#f8f9fa',
    padding: 12,
    borderRadius: 8,
    marginBottom: 10,
    borderWidth: 1,
    borderColor: '#e1e8ed',
  },
  devicePressArea: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingRight: 10,
  },
  deviceInfo: {
    flex: 1,
  },
  deviceName: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#2c3e50',
    marginBottom: 4,
  },
  deviceAddress: {
    fontSize: 12,
    color: '#7f8c8d',
  },
  signalStrength: {
    marginLeft: 10,
  },
  rssi: {
    fontSize: 12,
    color: '#3498db',
    fontWeight: '600',
  },
  connButton: {
    paddingHorizontal: 12,
    paddingVertical: 8,
    borderRadius: 6,
    minWidth: 90,
    alignItems: 'center',
    justifyContent: 'center',
  },
  connectButton: {
    backgroundColor: '#27ae60',
  },
  disconnectButton: {
    backgroundColor: '#e74c3c',
  },
  connButtonDisabled: {
    opacity: 0.6,
  },
  connButtonText: {
    color: '#fff',
    fontWeight: 'bold',
    fontSize: 12,
  },
  emptyContainer: {
    padding: 40,
    alignItems: 'center',
  },
  emptyText: {
    fontSize: 16,
    color: '#7f8c8d',
    textAlign: 'center',
  },
  errorText: {
    marginTop: 10,
    fontSize: 13,
    color: '#e74c3c',
    textAlign: 'center',
  },
});
