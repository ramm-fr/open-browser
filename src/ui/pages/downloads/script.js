'use strict';

// Downloads page — in production, the C++ host pushes updates via
// window.browserDownloads. For standalone display, we show a demo.

const STORAGE_KEY = 'ob_downloads';

function loadDownloads() {
  if (window.browserDownloads?.getAll) return window.browserDownloads.getAll();
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch (_) {}
  return [];
}

function getStateLabel(state) {
  const labels = {
    Downloading: 'Downloading',
    Completed:   'Completed',
    Failed:      'Failed',
    Paused:      'Paused',
    Cancelled:   'Cancelled',
    Pending:     'Pending',
  };
  return labels[state] || state;
}

function getStateClass(state) {
  const classes = {
    Downloading: 'state-downloading',
    Completed:   'state-completed',
    Failed:      'state-failed',
    Paused:      'state-paused',
    Cancelled:   'state-cancelled',
    Pending:     'state-downloading',
  };
  return classes[state] || '';
}

function getFileEmoji(filename) {
  const ext = filename?.split('.').pop()?.toLowerCase();
  const map = {
    pdf: '📄', zip: '🗜️', tar: '🗜️', gz: '🗜️',
    mp4: '🎬', mkv: '🎬', avi: '🎬', mov: '🎬',
    mp3: '🎵', wav: '🎵', flac: '🎵',
    jpg: '🖼️', jpeg: '🖼️', png: '🖼️', gif: '🖼️', svg: '🖼️', webp: '🖼️',
    exe: '⚙️', deb: '⚙️', rpm: '⚙️', AppImage: '⚙️',
    js: '📜', ts: '📜', py: '📜', cpp: '📜',
  };
  return map[ext] || '📁';
}

function formatBytes(bytes) {
  if (!bytes || bytes <= 0) return '—';
  if (bytes < 1024)       return `${bytes} B`;
  if (bytes < 1048576)    return `${(bytes / 1024).toFixed(1)} KB`;
  if (bytes < 1073741824) return `${(bytes / 1048576).toFixed(1)} MB`;
  return `${(bytes / 1073741824).toFixed(2)} GB`;
}

function getProgress(dl) {
  if (!dl.total_bytes || dl.total_bytes <= 0) return 0;
  return Math.round((dl.received_bytes / dl.total_bytes) * 100);
}

function escHtml(str) {
  return String(str ?? '').replace(/&/g,'&amp;').replace(/</g,'&lt;')
    .replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}

function clearCompleted() {
  if (window.browserDownloads?.clearCompleted) {
    window.browserDownloads.clearCompleted();
  } else {
    const all = loadDownloads();
    const active = all.filter(d =>
      d.state === 'Downloading' || d.state === 'Paused' || d.state === 'Pending');
    try { localStorage.setItem(STORAGE_KEY, JSON.stringify(active)); } catch (_) {}
  }
  render();
}

function render() {
  const container  = document.getElementById('downloads-container');
  const emptyState = document.getElementById('empty-state');
  if (!container) return;

  const downloads = loadDownloads();

  if (downloads.length === 0) {
    container.innerHTML = '';
    if (emptyState) emptyState.style.display = 'flex';
    return;
  }
  if (emptyState) emptyState.style.display = 'none';

  const sorted = [...downloads].sort((a, b) => (b.id || 0) - (a.id || 0));

  container.innerHTML = `<div class="download-list">
    ${sorted.map(dl => {
      const progress    = getProgress(dl);
      const isActive    = dl.state === 'Downloading';
      const isCompleted = dl.state === 'Completed';
      const showBar     = isActive || dl.state === 'Paused';

      return `
        <div class="download-card" data-id="${dl.id}">
          <div class="download-icon">${getFileEmoji(dl.filename)}</div>
          <div class="download-info">
            <div class="download-name">${escHtml(dl.filename || 'download')}</div>
            <div class="download-url">${escHtml(dl.url || '')}</div>
            ${showBar ? `
              <div class="download-progress-bar">
                <div class="download-progress-fill" style="width:${progress}%"></div>
              </div>` : ''}
            <div class="download-meta">
              <span>${escHtml(formatBytes(dl.received_bytes))}${dl.total_bytes ? ` / ${formatBytes(dl.total_bytes)}` : ''}</span>
              ${isActive ? `<span>${progress}%</span>` : ''}
              ${isCompleted ? `<span>Saved to ${escHtml(dl.destination || '~/Downloads')}</span>` : ''}
            </div>
          </div>
          <div class="download-actions">
            <span class="download-state ${getStateClass(dl.state)}">${getStateLabel(dl.state)}</span>
            ${isActive ? `<button class="action-btn" data-cancel="${dl.id}">Cancel</button>` : ''}
            ${dl.state === 'Paused' ? `<button class="action-btn" data-resume="${dl.id}">Resume</button>` : ''}
            ${isCompleted ? `<button class="action-btn" data-open="${dl.destination}">Open</button>` : ''}
            <button class="action-btn danger" data-remove="${dl.id}">✕</button>
          </div>
        </div>`;
    }).join('')}
  </div>`;

  // Button handlers
  container.querySelectorAll('[data-cancel]').forEach(btn => {
    btn.addEventListener('click', () => {
      if (window.browserDownloads?.cancel) window.browserDownloads.cancel(parseInt(btn.dataset.cancel));
      render();
    });
  });

  container.querySelectorAll('[data-resume]').forEach(btn => {
    btn.addEventListener('click', () => {
      if (window.browserDownloads?.resume) window.browserDownloads.resume(parseInt(btn.dataset.resume));
      render();
    });
  });

  container.querySelectorAll('[data-remove]').forEach(btn => {
    btn.addEventListener('click', () => {
      if (window.browserDownloads?.remove) {
        window.browserDownloads.remove(parseInt(btn.dataset.remove));
      } else {
        const all = loadDownloads().filter(d => d.id !== parseInt(btn.dataset.remove));
        try { localStorage.setItem(STORAGE_KEY, JSON.stringify(all)); } catch (_) {}
      }
      render();
    });
  });
}

function init() {
  render();

  document.getElementById('clear-completed-btn')?.addEventListener('click', clearCompleted);

  // Listen for updates from C++ host
  window.addEventListener('ob-download-update', () => render());

  // Poll for updates every 2 seconds when there are active downloads
  setInterval(() => {
    const downloads = loadDownloads();
    const hasActive = downloads.some(d =>
      d.state === 'Downloading' || d.state === 'Pending');
    if (hasActive) render();
  }, 2000);
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
