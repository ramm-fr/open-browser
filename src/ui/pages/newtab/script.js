'use strict';

// ─────────────────────────────────────────────────────────────────────────────
// Constants & defaults
// ─────────────────────────────────────────────────────────────────────────────

const STORAGE_KEY_SHORTCUTS = 'ob_shortcuts';
const STORAGE_KEY_HISTORY   = 'ob_history';
const SEARCH_ENGINE = 'https://search.brave.com/search?q=';

const DEFAULT_SHORTCUTS = [
  { id: 1, name: 'YouTube',  url: 'https://youtube.com',  emoji: '📺' },
  { id: 2, name: 'GitHub',   url: 'https://github.com',   emoji: '🐙' },
  { id: 3, name: 'Wikipedia',url: 'https://wikipedia.org',emoji: '📖' },
  { id: 4, name: 'Reddit',   url: 'https://reddit.com',   emoji: '🤖' },
  { id: 5, name: 'Maps',     url: 'https://maps.google.com', emoji: '🗺️' },
  { id: 6, name: 'Hacker News', url: 'https://news.ycombinator.com', emoji: '🟠' },
  { id: 7, name: 'MDN',      url: 'https://developer.mozilla.org', emoji: '🌐' },
  { id: 8, name: 'Gmail',    url: 'https://mail.google.com', emoji: '✉️' },
];

// ─────────────────────────────────────────────────────────────────────────────
// Greeting
// ─────────────────────────────────────────────────────────────────────────────

function updateGreeting() {
  const hour = new Date().getHours();
  let greeting;
  if (hour < 5)       greeting = 'Good night';
  else if (hour < 12) greeting = 'Good morning';
  else if (hour < 17) greeting = 'Good afternoon';
  else if (hour < 21) greeting = 'Good evening';
  else                greeting = 'Good night';

  const el = document.getElementById('greeting');
  if (el) el.textContent = greeting;
}

function updateDate() {
  const now = new Date();
  const el = document.getElementById('date-display');
  if (!el) return;
  const formatted = now.toLocaleDateString(navigator.language || 'en-US', {
    weekday: 'long',
    month:   'long',
    day:     'numeric',
  });
  el.textContent = formatted;
}

// ─────────────────────────────────────────────────────────────────────────────
// Storage helpers
// ─────────────────────────────────────────────────────────────────────────────

function loadShortcuts() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY_SHORTCUTS);
    if (raw) return JSON.parse(raw);
  } catch (_) { /* ignore */ }
  return DEFAULT_SHORTCUTS.map(s => ({ ...s }));
}

function saveShortcuts(shortcuts) {
  try {
    localStorage.setItem(STORAGE_KEY_SHORTCUTS, JSON.stringify(shortcuts));
  } catch (_) { /* ignore */ }
}

function loadHistory() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY_HISTORY);
    if (raw) return JSON.parse(raw);
  } catch (_) { /* ignore */ }
  return [];
}

// ─────────────────────────────────────────────────────────────────────────────
// Shortcuts rendering
// ─────────────────────────────────────────────────────────────────────────────

function renderShortcuts() {
  const grid = document.getElementById('shortcuts-grid');
  if (!grid) return;

  grid.innerHTML = '';
  const shortcuts = loadShortcuts();

  shortcuts.forEach(shortcut => {
    const link = document.createElement('a');
    link.className = 'shortcut-item';
    link.href = shortcut.url;
    link.title = shortcut.name;

    const iconEl = document.createElement('div');
    iconEl.className = 'shortcut-icon';

    // Try to load favicon; fall back to emoji
    const img = document.createElement('img');
    const faviconUrl = getFaviconUrl(shortcut.url);
    img.src = faviconUrl;
    img.alt = shortcut.name;
    img.width = 32;
    img.height = 32;
    img.onerror = () => {
      iconEl.removeChild(img);
      iconEl.textContent = shortcut.emoji || getInitialEmoji(shortcut.name);
    };
    iconEl.appendChild(img);

    const label = document.createElement('span');
    label.className = 'shortcut-label';
    label.textContent = shortcut.name;

    link.appendChild(iconEl);
    link.appendChild(label);

    // Right-click to remove
    link.addEventListener('contextmenu', (e) => {
      e.preventDefault();
      if (confirm(`Remove "${shortcut.name}" from shortcuts?`)) {
        const current = loadShortcuts();
        const updated = current.filter(s => s.id !== shortcut.id);
        saveShortcuts(updated);
        renderShortcuts();
      }
    });

    grid.appendChild(link);
  });
}

function getFaviconUrl(url) {
  try {
    const u = new URL(url);
    return `${u.origin}/favicon.ico`;
  } catch (_) {
    return '';
  }
}

function getInitialEmoji(name) {
  const emojis = ['🌐','📌','⭐','🔖','🔗','💡','🎯','🛠️','📊','🎵'];
  const idx = name.charCodeAt(0) % emojis.length;
  return emojis[idx];
}

// ─────────────────────────────────────────────────────────────────────────────
// History rendering
// ─────────────────────────────────────────────────────────────────────────────

function renderHistory() {
  const container = document.getElementById('history-grid');
  const section   = document.getElementById('history-section');
  if (!container) return;

  const entries = loadHistory();

  if (entries.length === 0) {
    section.style.display = 'none';
    return;
  }

  section.style.display = '';
  container.innerHTML = '';

  // Show up to 6 most recent
  const recent = entries
    .sort((a, b) => (b.visited_at || 0) - (a.visited_at || 0))
    .slice(0, 6);

  recent.forEach(entry => {
    const link = document.createElement('a');
    link.className = 'history-item';
    link.href = entry.url;

    const favicon = document.createElement('img');
    favicon.className = 'history-favicon';
    favicon.src = getFaviconUrl(entry.url);
    favicon.alt = '';
    favicon.onerror = () => { favicon.style.display = 'none'; };

    const text = document.createElement('div');
    text.className = 'history-text';

    const title = document.createElement('div');
    title.className = 'history-title';
    title.textContent = entry.title || entry.url;

    const urlEl = document.createElement('div');
    urlEl.className = 'history-url';
    urlEl.textContent = entry.url;

    text.appendChild(title);
    text.appendChild(urlEl);

    const timeEl = document.createElement('div');
    timeEl.className = 'history-time';
    timeEl.textContent = entry.visited_at
      ? formatRelativeTime(new Date(entry.visited_at * 1000))
      : '';

    link.appendChild(favicon);
    link.appendChild(text);
    link.appendChild(timeEl);

    container.appendChild(link);
  });
}

function formatRelativeTime(date) {
  const now = new Date();
  const diffMs = now - date;
  const diffMins = Math.floor(diffMs / 60000);

  if (diffMins < 1)   return 'Just now';
  if (diffMins < 60)  return `${diffMins}m ago`;
  const diffHours = Math.floor(diffMins / 60);
  if (diffHours < 24) return `${diffHours}h ago`;
  const diffDays = Math.floor(diffHours / 24);
  if (diffDays < 7)   return `${diffDays}d ago`;
  return date.toLocaleDateString();
}

// ─────────────────────────────────────────────────────────────────────────────
// Search
// ─────────────────────────────────────────────────────────────────────────────

function handleSearch(e) {
  e.preventDefault();
  const input = document.getElementById('search-input');
  const query = (input ? input.value : '').trim();
  if (!query) return;

  // Determine if it's a URL or a search
  const url = resolveInput(query);
  window.location.href = url;
}

function resolveInput(input) {
  // Already a URL with scheme
  if (/^[a-z][a-z0-9+\-.]*:\/\//i.test(input)) return input;

  // Looks like a domain
  if (/^[a-z0-9-]+\.[a-z]{2,}(\/.*)?$/i.test(input)) {
    return 'https://' + input;
  }

  // localhost
  if (/^localhost(:\d+)?(\/.*)?$/.test(input)) {
    return 'http://' + input;
  }

  // Search query
  return SEARCH_ENGINE + encodeURIComponent(input);
}

// ─────────────────────────────────────────────────────────────────────────────
// Add shortcut modal
// ─────────────────────────────────────────────────────────────────────────────

function openModal() {
  const backdrop = document.getElementById('modal-backdrop');
  if (backdrop) {
    backdrop.classList.add('open');
    backdrop.removeAttribute('aria-hidden');
    document.getElementById('shortcut-name')?.focus();
  }
}

function closeModal() {
  const backdrop = document.getElementById('modal-backdrop');
  if (backdrop) {
    backdrop.classList.remove('open');
    backdrop.setAttribute('aria-hidden', 'true');
  }
  // Clear fields
  const nameInput = document.getElementById('shortcut-name');
  const urlInput  = document.getElementById('shortcut-url');
  if (nameInput) nameInput.value = '';
  if (urlInput)  urlInput.value  = '';
}

function saveNewShortcut() {
  const name = document.getElementById('shortcut-name')?.value.trim();
  const url  = document.getElementById('shortcut-url')?.value.trim();

  if (!name || !url) {
    alert('Please fill in both fields.');
    return;
  }

  const normalizedUrl = url.startsWith('http') ? url : 'https://' + url;

  const shortcuts = loadShortcuts();
  const newId = shortcuts.length > 0
    ? Math.max(...shortcuts.map(s => s.id)) + 1
    : 1;

  shortcuts.push({ id: newId, name, url: normalizedUrl, emoji: '🔗' });
  saveShortcuts(shortcuts);
  renderShortcuts();
  closeModal();
}

// ─────────────────────────────────────────────────────────────────────────────
// Event wiring
// ─────────────────────────────────────────────────────────────────────────────

function init() {
  updateGreeting();
  updateDate();
  renderShortcuts();
  renderHistory();

  // Search form
  const form = document.getElementById('search-form');
  form?.addEventListener('submit', handleSearch);

  // Keyboard shortcut: slash focuses search bar
  document.addEventListener('keydown', (e) => {
    if (e.key === '/' && document.activeElement !== document.getElementById('search-input')) {
      e.preventDefault();
      document.getElementById('search-input')?.focus();
    }
    if (e.key === 'Escape') closeModal();
  });

  // Add shortcut button
  document.getElementById('add-shortcut-btn')?.addEventListener('click', openModal);

  // Modal buttons
  document.getElementById('modal-cancel')?.addEventListener('click', closeModal);
  document.getElementById('modal-save')?.addEventListener('click', saveNewShortcut);

  // Close modal on backdrop click
  document.getElementById('modal-backdrop')?.addEventListener('click', (e) => {
    if (e.target === e.currentTarget) closeModal();
  });

  // Allow Enter in modal inputs to save
  document.getElementById('shortcut-url')?.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') saveNewShortcut();
  });

  // Update clock every minute
  setInterval(() => {
    updateGreeting();
    updateDate();
  }, 60 * 1000);
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
