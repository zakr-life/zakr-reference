const { test } = require('node:test');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const { DeviceAuthenticator } = require('../ingestion/deviceAuth');

function genKeyPair() {
  return crypto.generateKeyPairSync('ed25519');
}

test('valid signature from a registered device is accepted', () => {
  const { publicKey, privateKey } = genKeyPair();
  const pubPem = publicKey.export({ type: 'spki', format: 'pem' });
  const registry = new Map([['device-1', pubPem]]);
  const auth = new DeviceAuthenticator(registry);

  const payload = Buffer.from(JSON.stringify({ sessionId: 's1', durationMs: 60000 }));
  const signature = crypto.sign(null, payload, privateKey);

  const result = auth.verifyUpload({ deviceId: 'device-1', payload, signature });
  assert.equal(result.accepted, true);
});

test('unknown device id is rejected (not password-based, not IP-based)', () => {
  const registry = new Map();
  const auth = new DeviceAuthenticator(registry);
  const result = auth.verifyUpload({
    deviceId: 'device-never-provisioned',
    payload: Buffer.from('x'),
    signature: Buffer.from('y'),
  });
  assert.equal(result.accepted, false);
  assert.match(result.reason, /unknown device id/);
});

test('tampered payload fails signature verification', () => {
  const { publicKey, privateKey } = genKeyPair();
  const pubPem = publicKey.export({ type: 'spki', format: 'pem' });
  const registry = new Map([['device-1', pubPem]]);
  const auth = new DeviceAuthenticator(registry);

  const payload = Buffer.from(JSON.stringify({ sessionId: 's1' }));
  const signature = crypto.sign(null, payload, privateKey);
  const tamperedPayload = Buffer.from(JSON.stringify({ sessionId: 's1-EVIL' }));

  const result = auth.verifyUpload({ deviceId: 'device-1', payload: tamperedPayload, signature });
  assert.equal(result.accepted, false);
});

test('signature from a DIFFERENT device key is rejected', () => {
  const attacker = genKeyPair();
  const legit = genKeyPair();
  const registry = new Map([['device-1', legit.publicKey.export({ type: 'spki', format: 'pem' })]]);
  const auth = new DeviceAuthenticator(registry);

  const payload = Buffer.from('data');
  const forgedSignature = crypto.sign(null, payload, attacker.privateKey);

  const result = auth.verifyUpload({ deviceId: 'device-1', payload, signature: forgedSignature });
  assert.equal(result.accepted, false);
});

test('malformed upload missing fields is rejected', () => {
  const auth = new DeviceAuthenticator(new Map());
  assert.equal(auth.verifyUpload({}).accepted, false);
  assert.equal(auth.verifyUpload(null).accepted, false);
});
