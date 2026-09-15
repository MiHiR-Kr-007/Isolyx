import { useState, useEffect, useRef } from 'react';
import { io } from 'socket.io-client';
import './index.css';

const socket = io('http://localhost:3001');

function App() {
  const [jobs, setJobs] = useState([]);
  const [connected, setConnected] = useState(false);
  const [selectedFile, setSelectedFile] = useState(null);
  const [codeContent, setCodeContent] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [currentResult, setCurrentResult] = useState(null);
  const fileInputRef = useRef(null);

  useEffect(() => {
    socket.on('connect', () => setConnected(true));
    socket.on('disconnect', () => setConnected(false));
    socket.on('job_result', (event) => {
      setJobs((prevJobs) => [event, ...prevJobs].slice(0, 50));
    });

    return () => {
      socket.off('connect');
      socket.off('disconnect');
      socket.off('job_result');
    };
  }, []);

  const handleFileSelect = (e) => {
    const file = e.target.files[0];
    if (!file) return;

    setSelectedFile(file);
    const reader = new FileReader();
    reader.onload = (evt) => {
      setCodeContent(evt.target.result);
      setCurrentResult(null);
    };
    reader.readAsText(file);
  };

  const handleSubmit = async () => {
    if (!codeContent) return;
    setIsSubmitting(true);
    setCurrentResult(null);

    // Get language from extension
    const ext = selectedFile?.name.split('.').pop() || 'cpp';

    try {
      const response = await fetch('http://localhost:3001/api/submit', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code: codeContent, language: ext })
      });

      const data = await response.json();
      if (!response.ok) {
        throw new Error(data.error || 'Submission failed');
      }
      setCurrentResult(data);
    } catch (err) {
      setCurrentResult({
        verdict: 'ERROR',
        exit_code: -1,
        stderr_out: err.message,
        stdout_out: ''
      });
    } finally {
      setIsSubmitting(false);
    }
  };

  const getBadgeClass = (verdict) => {
    if (verdict === 'SUCCESS') return 'badge badge-success';
    if (verdict === 'TIMEOUT' || verdict === 'SECCOMP_VIOLATION') return 'badge badge-warning';
    return 'badge badge-error';
  };

  return (
    <div className="dashboard-container">
      <header className="header">
        <div className="header-brand">
          <div className="logo-pulse"></div>
          <h1>Isolyx <span>Portal</span></h1>
        </div>
        <div className="status-indicator">
          {connected ? (
            <>
              <div className="dot"></div>
              <span>Engine Online</span>
            </>
          ) : (
            <span className="error-text">Engine Offline</span>
          )}
        </div>
      </header>

      <main className="main-layout">
        <section className="left-pane">
          <div className="panel">
            <div className="panel-header">
              <h2>Code Submission</h2>
              <span className="supported-langs">C++ currently supported</span>
            </div>
            
            <div className="upload-zone" onClick={() => fileInputRef.current.click()}>
              <input 
                type="file" 
                ref={fileInputRef} 
                style={{display: 'none'}} 
                accept=".cpp,.py,.java,.c" 
                onChange={handleFileSelect}
              />
              <div className="upload-content">
                <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path>
                  <polyline points="17 8 12 3 7 8"></polyline>
                  <line x1="12" y1="3" x2="12" y2="15"></line>
                </svg>
                {selectedFile ? (
                  <p className="selected-file">{selectedFile.name}</p>
                ) : (
                  <p>Click to browse files or drag and drop</p>
                )}
              </div>
            </div>

            {codeContent && (
              <div className="code-preview-container">
                <pre className="code-preview">
                  <code>{codeContent}</code>
                </pre>
              </div>
            )}

            <button 
              className={`submit-btn ${isSubmitting ? 'submitting' : ''}`}
              onClick={handleSubmit}
              disabled={!codeContent || isSubmitting || !connected}
            >
              {isSubmitting ? 'Executing...' : 'Run Code'}
            </button>
          </div>
        </section>

        <section className="right-pane">
          <div className="panel">
            <div className="panel-header">
              <h2>Execution Result</h2>
            </div>
            
            {!currentResult && !isSubmitting ? (
              <div className="empty-state">
                <p>Submit code to see execution results.</p>
              </div>
            ) : isSubmitting ? (
              <div className="empty-state">
                <div className="spinner"></div>
                <p>Engine is processing your job...</p>
              </div>
            ) : (
              <div className="results-content">
                <div className="result-meta">
                  <div className="meta-item">
                    <span className="meta-label">Verdict</span>
                    <span className={getBadgeClass(currentResult.verdict)}>{currentResult.verdict}</span>
                  </div>
                  <div className="meta-item">
                    <span className="meta-label">Exit Code</span>
                    <span className="meta-value">{currentResult.exit_code}</span>
                  </div>
                </div>

                <div className="output-section">
                  <h3>Standard Output</h3>
                  <pre className="output-block stdout">
                    {currentResult.stdout_out || <span className="empty-out">No output</span>}
                  </pre>
                </div>

                <div className="output-section">
                  <h3>Standard Error</h3>
                  <pre className="output-block stderr">
                    {currentResult.stderr_out || <span className="empty-out">No error output</span>}
                  </pre>
                </div>
              </div>
            )}
          </div>
        </section>
      </main>
    </div>
  );
}

export default App;
