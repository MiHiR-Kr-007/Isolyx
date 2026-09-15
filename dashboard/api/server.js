import express from 'express';
import http from 'http';
import { Server } from 'socket.io';
import cors from 'cors';
import fs from 'fs';
import { spawn, execSync } from 'child_process';
import path from 'path';
import { v4 as uuidv4 } from 'uuid';

const app = express();
app.use(cors());
app.use(express.json());

const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: '*',
    },
});

const PIPE_PATH = '/tmp/isolyx_events';
const CMD_PIPE_PATH = '/tmp/isolyx_cmds';

if (!fs.existsSync(PIPE_PATH)) {
    spawn('mkfifo', [PIPE_PATH]);
}
if (!fs.existsSync(CMD_PIPE_PATH)) {
    spawn('mkfifo', [CMD_PIPE_PATH]);
}

const pendingRequests = new Map();

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

                    // Check if this event matches any pending REST requests
                    for (const [uuid, res] of pendingRequests.entries()) {
                        if (event.command_line && event.command_line.includes(uuid)) {
                            res.json(event);
                            pendingRequests.delete(uuid);
                            break;
                        }
                    }

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

const PROJECT_ROOT = path.resolve(import.meta.dirname, '../../');
const ROOTFS_TMP = path.join(PROJECT_ROOT, 'rootfs_base', 'tmp');

if (!fs.existsSync(ROOTFS_TMP)) {
    fs.mkdirSync(ROOTFS_TMP, { recursive: true });
}

app.post('/api/submit', (req, res) => {
    const { code, language } = req.body;

    if (!code || language !== 'cpp') {
        return res.status(400).json({ error: 'Only C++ (cpp) is currently supported or code is missing.' });
    }

    const jobId = uuidv4();
    const sourceFile = path.join(ROOTFS_TMP, `job_${jobId}.cpp`);
    const binFile = path.join(ROOTFS_TMP, `job_${jobId}.bin`);

    try {
        fs.writeFileSync(sourceFile, code);

        execSync(`g++ -static ${sourceFile} -o ${binFile}`);

        fs.unlinkSync(sourceFile);

        const payload = JSON.stringify({
            executable: `/tmp/job_${jobId}.bin`,
            args: []
        });

        const cmdFd = fs.openSync(CMD_PIPE_PATH, 'w');
        fs.writeSync(cmdFd, payload + '\n');
        fs.closeSync(cmdFd);

        pendingRequests.set(jobId, res);

    } catch (e) {
        console.error('Compilation or Submission Error:', e);
        return res.status(500).json({
            error: 'Compilation failed or engine offline',
            details: e.message
        });
    }
});

const PORT = 3001;
server.listen(PORT, () => {
    console.log(`Isolyx Dashboard API running on port ${PORT}`);
});
