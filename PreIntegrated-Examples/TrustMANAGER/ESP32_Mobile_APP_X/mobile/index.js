/**
 * @format
 */

import { AppRegistry } from 'react-native';
import App from './App';
import { name as appName } from './app.json';

// Surface fatal JS exceptions in Logcat/Metro logs.
// This helps distinguish true native crashes from JS runtime errors.
try {
	const ErrorUtils = global.ErrorUtils;
	if (ErrorUtils && typeof ErrorUtils.setGlobalHandler === 'function') {
		const defaultHandler = typeof ErrorUtils.getGlobalHandler === 'function' ? ErrorUtils.getGlobalHandler() : null;

		ErrorUtils.setGlobalHandler((error, isFatal) => {
			// eslint-disable-next-line no-console
			console.log('[GlobalErrorHandler]', {
				isFatal: Boolean(isFatal),
				message: error?.message,
				name: error?.name,
				stack: error?.stack,
			});

			if (typeof defaultHandler === 'function') {
				defaultHandler(error, isFatal);
			}
		});
	}
} catch {
	// ignore
}

// Best-effort unhandled rejection visibility.
try {
	if (typeof process !== 'undefined' && process && typeof process.on === 'function') {
		process.on('unhandledRejection', (reason) => {
			// eslint-disable-next-line no-console
			console.log('[UnhandledRejection]', reason);
		});
	}
} catch {
	// ignore
}

AppRegistry.registerComponent(appName, () => App);
