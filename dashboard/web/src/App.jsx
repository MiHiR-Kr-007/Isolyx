import { useState, useEffect } from 'react';
import { io } from 'socket.io-client';
import './index.css';

const socket = io('http://localhost:3001');

function App() {
  const [jobs, setJobs] = useState([]);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    socket.on('connect', () => {
      setConnected(true);
    });

    socket.on('disconnect', () => {
      setConnected(false);
    });

    socket.on('job_result', (event) => {
      setJobs((prevJobs) => [event, ...prevJobs].slice(0, 50)); // Keep last 50
    });

    return () => {
      socket.off('connect');
      socket.off('disconnect');
      socket.off('job_result');
    };
  }, []);

  const getBadgeClass = (verdict) => {
    if (verdict === 'SUCCESS') return 'badge badge-success';
    if (verdict === 'TIMEOUT' || verdict === 'SECCOMP_VIOLATION') return 'badge badge-warning';
    return 'badge badge-error';
  };

  return (
    <div className="dashboard-container">
      <header className="header">
        <h1>Isolyx <span>Dashboard</span></h1>
        <div className="status-indicator">
          {connected ? (
            <>
              <div className="dot"></div>
              <span>Connected to Core Engine</span>
            </>
          ) : (
            <span style={{ color: 'var(--error)' }}>Disconnected</span>
          )}
        </div>
      </header>

      <main className="grid">
        <div className="card">
          <div className="card-title">Total Jobs Run</div>
          <div className="card-value">{jobs.length}</div>
        </div>
        
        <div className="card">
          <div className="card-title">Success Rate</div>
          <div className="card-value">
            {jobs.length > 0
              ? Math.round((jobs.filter((j) => j.verdict === 'SUCCESS').length / jobs.length) * 100) + '%'
              : '--'}
          </div>
        </div>

        <div className="card">
          <div className="card-title">Last Event ID</div>
          <div className="card-value">{jobs.length > 0 ? jobs[0].job_id : '--'}</div>
        </div>

        <div className="card jobs-list">
          <div className="card-title" style={{ marginBottom: '1.5rem' }}>Recent Job History</div>
          
          <table className="jobs-table">
            <thead>
              <tr>
                <th>Job ID</th>
                <th>Command</th>
                <th>Verdict</th>
                <th>Exit Code</th>
                <th>Signal</th>
              </tr>
            </thead>
            <tbody>
              {jobs.length === 0 ? (
                <tr>
                  <td colSpan="5" style={{ textAlign: 'center', color: 'var(--text-secondary)' }}>
                    No jobs recorded yet. Run a command in the Isolyx shell!
                  </td>
                </tr>
              ) : (
                jobs.map((job, idx) => (
                  <tr key={`${job.job_id}-${idx}`}>
                    <td>#{job.job_id}</td>
                    <td><span className="cmd-text">{job.command_line}</span></td>
                    <td><span className={getBadgeClass(job.verdict)}>{job.verdict}</span></td>
                    <td>{job.exit_code}</td>
                    <td>{job.term_signal === 0 ? '--' : `SIG(${job.term_signal})`}</td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </main>
    </div>
  );
}

export default App;
