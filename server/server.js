const WebSocket = require('ws');

const PORT = 8081;
const wss = new WebSocket.Server({ port: PORT });

let sharedState = {
    mass: 10.0,
    spin: 0.0,
    charge: 0.0,
    lastUpdateBy: "System"
};

console.log(`[Relay] Shared Spacetime Server started on ws://localhost:${PORT}`);

wss.on('connection', (ws) => {
    console.log('[Relay] New client connected. clients=' + wss.clients.size);

    ws.send(JSON.stringify({ type: 'SYNC_STATE', state: sharedState }));

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message);

            if (data.type === 'UPDATE_STATE') {
                sharedState = {
                    mass: data.mass,
                    spin: data.spin,
                    charge: data.charge,
                    lastUpdateBy: data.user || "Unknown"
                };

                console.log(`[Relay] State updated by ${sharedState.lastUpdateBy}: M=${sharedState.mass.toFixed(2)}, a=${sharedState.spin.toFixed(2)}`);

                wss.clients.forEach((client) => {
                    if (client !== ws && client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify({ type: 'SYNC_STATE', state: sharedState }));
                    }
                });
            }
            else if (data.type === 'LLM_ANALYSIS') {
                console.log(`[Relay] Broadcasting LLM Analysis: ${data.text.substring(0, 50)}...`);
                wss.clients.forEach((client) => {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(JSON.stringify({ type: 'LLM_ANALYSIS', text: data.text }));
                    }
                });
            }
        } catch (e) {
            console.error('[Relay] Error parsing message:', e);
        }
    });

    ws.on('close', () => {
        console.log('[Relay] Client disconnected. clients=' + wss.clients.size);
    });
});
