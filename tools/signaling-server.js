import { WebSocketServer, WebSocket } from 'ws';
import { createServer } from 'http';

const PORT = process.env.PORT ? parseInt(process.env.PORT) : 9000;

const server = createServer();
const wss = new WebSocketServer({ server });

const rooms = new Map();

function broadcast(roomId, message, excludeWs) {
  const room = rooms.get(roomId);
  if (!room) return;
  const data = JSON.stringify(message);
  for (const client of room) {
    if (client !== excludeWs && client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  }
}

wss.on('connection', (ws) => {
  console.log('client connected');

  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw.toString());
    } catch {
      return;
    }

    switch (msg.type) {
      case 'join': {
        const { roomId, participant } = msg;
        if (!rooms.has(roomId)) rooms.set(roomId, new Set());
        rooms.get(roomId).add(ws);
        ws.roomId = roomId;
        ws.participant = participant;
        broadcast(roomId, {
          type: 'participant-joined',
          participant,
          participants: Array.from(rooms.get(roomId)).map(c => c.participant).filter(Boolean)
        }, ws);
        break;
      }
      case 'leave': {
        const roomId = ws.roomId;
        if (roomId && rooms.has(roomId)) {
          rooms.get(roomId).delete(ws);
          broadcast(roomId, {
            type: 'participant-left',
            participantId: ws.participant?.id
          }, ws);
        }
        break;
      }
      case 'offer':
      case 'answer':
      case 'ice-candidate':
      case 'state-update': {
        const { roomId, to } = msg;
        if (!roomId || !rooms.has(roomId)) break;
        if (to) {
          for (const client of rooms.get(roomId)) {
            if (client.participant?.id === to && client.readyState === WebSocket.OPEN) {
              client.send(JSON.stringify(msg));
              break;
            }
          }
        } else {
          broadcast(roomId, msg, ws);
        }
        break;
      }
      default:
        break;
    }
  });

  ws.on('close', () => {
    const roomId = ws.roomId;
    if (roomId && rooms.has(roomId)) {
      rooms.get(roomId).delete(ws);
      broadcast(roomId, {
        type: 'participant-left',
        participantId: ws.participant?.id
      }, ws);
    }
  });
});

server.listen(PORT, () => {
  console.log(`signaling server listening on :${PORT}`);
});
