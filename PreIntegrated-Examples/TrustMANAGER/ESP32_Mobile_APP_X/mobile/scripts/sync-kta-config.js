const fs = require('fs');
const path = require('path');

const ktaConfigPath = path.join(__dirname, '../../firmware/main/kta_provisioning/SOURCE/include/ktaConfig.h');
const outPath = path.join(__dirname, '../src/services/KtaConfig.ts');

const content = fs.readFileSync(ktaConfigPath, 'utf8');

let l1SegSeed = '[]';
const seedMatch = content.match(/#define\s+C_KTA_APP__L1_SEG_SEED\s+\{([^}]+)\}/);
if (seedMatch) {
  l1SegSeed = `[${seedMatch[1].trim()}]`;
}

let devicePublicUid = '""';
const uidMatch = content.match(/#define\s+C_KTA_APP__DEVICE_PUBLIC_UID\s+"([^"]+)"/);
if (uidMatch) {
  devicePublicUid = `"${uidMatch[1].trim()}"`;
}

let keyStreamHttpUrl = '""';
const urlMatch = content.match(/#define\s+C_KTA_APP__KEYSTREAM_HOST_HTTP_URL\s+\([^\)]+\)"([^"]+)"/);
if (urlMatch) {
  keyStreamHttpUrl = `"${urlMatch[1].trim()}"`;
}

const tsContent = `// AUTO-GENERATED from firmware ktaConfig.h
export const KtaConfig = {
  l1SegSeed: Uint8Array.from(${l1SegSeed}),
  devicePublicUid: ${devicePublicUid},
  keyStreamHttpUrl: ${keyStreamHttpUrl},
};
`;

fs.writeFileSync(outPath, tsContent);
console.log('Successfully generated KtaConfig.ts from ktaConfig.h');
