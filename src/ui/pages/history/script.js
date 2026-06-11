'use strict';

const STORAGE_KEY = 'ob_history';

function loadHistory() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch (_) {}
  return [];
}

function saveHistory(entries) {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(entries)); } catch (_) {}
}

function removeEntry(id) {
  const entries = loadHistory().filter(e => e.id !== id);
  saveHistory(entries);
  render(document.getElementById('search-input')?.value || '');
}

function clearAll() {
  saveHistory([]);
  render('');
}

function getFaviconUrl(url) {
  try { return `${new URL(url).origin}/favicon.ico`; } catch (_) { return ''; }
}

function formatTime(ts) {
  if (!ts) return '';
  const d = new Date(ts * 1000);
  return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

function getDayLabel(ts) {
  const d = new Date(ts * 1000);
  const today = new Date();
  const yesterday = new Date(today);
  yesterday.setDate(today.getDate() - 1);

  if (d.toDateString() === today.toDateString())     return 'Today';
  if (d.toDateString() === yesterday.toDateString()) return 'Yesterday';
  return d.toLocaleDateString(undefined, { weekday: 'long', month: 'long', day: 'numeric' });
}

function escHtml(str) {
  return String(str ?? '')
    .replace(/&/g, '&amp;').replace(/</g, '&lt;')
    .replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}

function groupByDay(entries) {
  const groups = new Map();
  for (const entry of entries) {
    const d = new Date((entry.visited_at || 0) * 1000);
    const key = d.toDateString();
    if (!groups.has(key)) groups.set(key, { label: getDayLabel(entry.visited_at), items: [] });
    groups.get(key).items.push(entry);
  }
  return groups;
}

function render(query = '') {
  const container  = document.getElementById('history-container');
  const emptyState = document.getElementById('empty-state');
  if (!container) return;

  let entries = loadHistory().sort((a, b) =>
    (b.visited_at || 0) - (a.visited_at || 0));

  if (query) {
    const lq = query.toLowerCase();
    entries = entries.filter(e =>
      e.title?.toLowerCase().includes(lq) || e.url?.toLowerCase().includes(lq));
  }

  if (entries.length === 0) {
    container.innerHTML = '';
    if (emptyState) emptyState.style.display = 'flex';
    return;
  }
  if (emptyState) emptyState.style.display = 'none';

  const groups = groupByDay(entries);
  let html = '';

  for (const [, group] of groups) {
    html += `<div class="date-group">
      <div class="date-label">${escHtml(group.label)}</div>
      <div class="history-list">`;

    for (const entry of group.items) {
      const favicon = getFaviconUrl(entry.url);
      const time    = formatTime(entry.visited_at);
      html += `
        <a class="history-item" href="${escHtml(entry.url)}"
           title="${escHtml(entry.title || entry.url)}">
          <img class="history-favicon" src="${escHtml(favicon)}" alt=""
               onerror="this.style.display='none'" />
          <div class="history-text">
            <div class="history-title">${escHtml(entry.title || entry.url)}</div>
            <div class="history-url">${escHtml(entry.url)}</div>
          </div>
          <div class="history-time">${escHtml(time)}</div>
          <button class="item-delete" data-delete-id="${entry.id}"
                  aria-label="Remove from history" title="Remove">✕</button>
        </a>`;
    }
    html += '</div></div>';
  }

  container.innerHTML = html;

  container.querySelectorAll('[data-delete-id]').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      removeEntry(parseInt(btn.dataset.deleteId, 10));
    });
  });
}

function init() {
  render();

  document.getElementById('search-input')?.addEventListener('input', (e) => {
    render(e.target.value);
  });

  document.getElementById('clear-all-btn')?.addEventListener('click', () => {
    if (confirm('Delete all browsing history?')) clearAll();
  });
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
