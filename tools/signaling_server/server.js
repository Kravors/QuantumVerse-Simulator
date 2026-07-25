/**
 * @file server.js
 * @brief QuantumVerse Signaling Server
 *
 * Lightweight WebSocket server that acts as a message broker for
 * multi-user collaboration sessions. Clients send JSON messages,
 * and the server forwards them to all other clients in the same session.
 *
 * Protocol:
 *   Client -> Server:
 *     { "type": "join", "sessionId": "...", "clientId": "...", "clientName": "..." }
 *     { "type": "leave", "sessionId": "...", "clientId": "..." }
 *     { "type": "state_update", "sessionId": "...", "senderId": "...", "senderName": "...", "state": {...}, "timestamp": 12345 }
 *
 *   Server -> Client:
 *     { "type": "welcome", "sessionId": "...", "clientId": "...", "peers": [...] }
 *     { "type": "peer_joined", "sessionId": "...", "clientId": "...", "clientName": "..." }
 *     { "type": "peer_left", "sessionId": "...", "clientId": "..." }
 *     { "type": "state_update", "sessionId": "...", "senderId": "...", "senderName": "...", "state": {...}, "timestamp": 12345 }
 */

const { WebSocketServer } = require('ws');

const PORT = process.env.PORT || 8080;

const wss = new WebSocketServer({ port: PORT });

console.log(`QuantumVerse Signaling Server listening on ws://localhost:${PORT}`);

/**
 * @type {Map<string, Set<WebSocket>>}
 * Maps session ID to set of connected WebSocket clients
 */
const sessions = new Map();

/**
 * @type {Map<WebSocket, {sessionId: string, clientId: string, clientName: string}>}
 * Maps WebSocket to session metadata
 */
const clients = new Map();

wss.on('connection', (ws) => {
    console.log('New client connection');

    ws.on('message', (data) => {
        let msg;
        try {
            msg = JSON.parse(data.toString());
        } catch (e) {
            console.error('Invalid JSON received:', data.toString());
            return;
        }

        const type = msg.type;

        if (type === 'join') {
            const { sessionId, clientId, clientName } = msg;
            if (!sessionId || !clientId) {
                console.warn('Join message missing sessionId or clientId');
                return;
            }

            // Store client metadata
            clients.set(ws, {
                sessionId,
                clientId,
                clientName: clientName || 'Anonymous'
            });

            // Add to session
            if (!sessions.has(sessionId)) {
                sessions.set(sessionId, new Set());
            }
            const session = sessions.get(sessionId);
            session.add(ws);

            // Send welcome to the joining client with list of existing peers
            const peerIds = [];
            for (const client of session) {
                if (client !== ws && client.readyState === 1) {
                    const meta = clients.get(client);
                    if (meta) peerIds.push(meta.clientId);
                }
            }

            ws.send(JSON.stringify({
                type: 'welcome',
                sessionId,
                clientId,
                peers: peerIds
            }));

            // Notify existing peers about the new client
            const joinerMeta = clients.get(ws);
            for (const client of session) {
                if (client !== ws && client.readyState === 1) {
                    client.send(JSON.stringify({
                        type: 'peer_joined',
                        sessionId,
                        clientId,
                        clientName: joinerMeta ? joinerMeta.clientName : 'Anonymous'
                    }));
                }
            }

            console.log(`Client ${clientId} joined session ${sessionId}`);

        } else if (type === 'leave') {
            const { sessionId, clientId } = msg;
            handleDisconnect(ws, sessionId, clientId);

        } else if (type === 'state_update') {
            const { sessionId, senderId, senderName, state, timestamp } = msg;
            if (!sessionId || !senderId) return;

            const session = sessions.get(sessionId);
            if (!session) return;

            // Forward state update to all other clients in the same session
            const forwardMsg = JSON.stringify({
                type: 'state_update',
                sessionId,
                senderId,
                senderName: senderName || '',
                state: state || {},
                timestamp: timestamp || Date.now()
            });

            for (const client of session) {
                if (client !== ws && client.readyState === 1) {
                    client.send(forwardMsg);
                }
            }
        }
    });

    ws.on('close', () => {
        const meta = clients.get(ws);
        if (meta) {
            handleDisconnect(ws, meta.sessionId, meta.clientId);
        }
        clients.delete(ws);
    });

    ws.on('error', (err) => {
        console.error('WebSocket error:', err.message);
    });
});

/**
 * Handle client disconnection
 * @param {WebSocket} ws - The disconnected WebSocket
 * @param {string} sessionId - Session ID
 * @param {string} clientId - Client ID
 */
function handleDisconnect(ws, sessionId, clientId) {
    if (!sessionId) return;

    const session = sessions.get(sessionId);
    if (!session) return;

    session.delete(ws);

    // Notify remaining peers
    for (const client of session) {
        if (client.readyState === 1) {
            client.send(JSON.stringify({
                type: 'peer_left',
                sessionId,
                clientId
            }));
        }
    }

    console.log(`Client ${clientId} left session ${sessionId}`);

    // Clean up empty sessions
    if (session.size === 0) {
        sessions.delete(sessionId);
    }
}

process.on('SIGINT', () => {
    console.log('\nShutting down signaling server...');
    wss.close(() => {
        console.log('Server closed');
        process.exit(0);
    });
});
