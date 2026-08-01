import express from 'express';
import http from 'http';
import { Server } from 'socket.io';
import cors from 'cors';
import fs from 'fs';
import { spawn } from 'child_process';

const app = express();
app.use(cors());

const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: '*',
    },
});

const PIPE_PATH = '/tmp/isolyx_events';

if (!fs.existsSync(PIPE_PATH)) {
    spawn('mkfifo', [PIPE_PATH]);
}

io.on('connection', (socket) => {
    console.log('Frontend client connected');
});

console.log(`Listening on named pipe: ${PIPE_PATH}`);

function readFromPipe() {
    const stream = fs.createReadStream(PIPE_PATH, { encoding: 'utf8', flags: 'r' });

    stream.on('data', (chunk) => {
        const lines = chunk.split('\n');
        for (const line of lines) {
            if (line.trim().length > 0) {
                try {
                    const event = JSON.parse(line);
                    io.emit('job_result', event);
                    console.log('Broadcasting event:', event);
                } catch (e) {
                    console.error('Failed to parse event JSON:', line, e);
                }
            }
        }
    });

    stream.on('error', (err) => {
        if (err.code !== 'ENOENT') {
            console.error('Error reading pipe:', err);
        }
        setTimeout(readFromPipe, 1000);
    });

    stream.on('end', () => {
        setTimeout(readFromPipe, 100);
    });
}

setTimeout(readFromPipe, 500);

const PORT = 3001;
server.listen(PORT, () => {
    console.log(`Isolyx Dashboard API running on port ${PORT}`);
});
