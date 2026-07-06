import { BleManager } from 'react-native-ble-plx';

// Single shared BLE manager instance for the whole app.
// Using multiple BleManager instances can cause native instability on Android.
export const bleManager = new BleManager();
