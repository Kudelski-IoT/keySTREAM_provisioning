/**
 * keySTREAM Provisioning App
 * 
 * @format
 */

import React, { useState } from 'react';
import { StatusBar, StyleSheet, View } from 'react-native';
import {
  SafeAreaProvider,
  SafeAreaView,
} from 'react-native-safe-area-context';
import { MainConfigScreen } from './src/screens/MainConfigScreen';
import { DeviceStatusScreen } from './src/screens/DeviceStatusScreen';

type Screen = 'config' | 'status';

function App() {
  const [currentScreen, setCurrentScreen] = useState<Screen>('config');
  const [deviceConfig, setDeviceConfig] = useState({
    bleDeviceName: '',
    bleDeviceAddress: '',
    chipUID: '',
    fleetUID: '',
    authToken: '',
    bleDeviceId: '',
    tenantId: '',
    serverUrl: '',
    apiBaseUrl: '',
  });

  const handleConfigSubmit = (config: {
    authToken: string;
    fleetProfileUID: string;
    tenantId: string;
    selectedDevice: string;
    passkey: string;
    serverUrl: string;
    apiBaseUrl: string;
  }) => {
    // Extract device name and address from selectedDevice string
    // Format: "DeviceName (MAC:Address)"
    const match = config.selectedDevice.match(/^(.+?) \((.+?)\)$/);
    const deviceName = match ? match[1] : config.selectedDevice;
    const deviceAddress = match ? match[2] : 'Unknown';

    setDeviceConfig({
      bleDeviceName: deviceName,
      bleDeviceAddress: deviceAddress,
      chipUID: '0x' + deviceAddress.replace(/:/g, '').substring(0, 16),
      fleetUID: config.fleetProfileUID,
      authToken: config.authToken,
      bleDeviceId: deviceAddress,
      tenantId: config.tenantId,
      serverUrl: config.serverUrl,
      apiBaseUrl: config.apiBaseUrl,
    });
    
    setCurrentScreen('status');
  };

  const navigateToConfig = () => {
    setCurrentScreen('config');
  };

  return (
    <SafeAreaProvider>
      <StatusBar barStyle="light-content" backgroundColor="#2c3e50" />
      <SafeAreaView style={styles.container} edges={['top']}>
        {currentScreen === 'config' ? (
          <MainConfigScreen onSubmit={handleConfigSubmit} />
        ) : (
          <DeviceStatusScreen
            bleDeviceName={deviceConfig.bleDeviceName}
            bleDeviceAddress={deviceConfig.bleDeviceAddress}
            chipUID={deviceConfig.chipUID}
            fleetUID={deviceConfig.fleetUID}
            authToken={deviceConfig.authToken}
            bleDeviceId={deviceConfig.bleDeviceId}
            tenantId={deviceConfig.tenantId}
            serverUrl={deviceConfig.serverUrl}
            apiBaseUrl={deviceConfig.apiBaseUrl}
            onBack={navigateToConfig}
          />
        )}
      </SafeAreaView>
    </SafeAreaProvider>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
});

export default App;
