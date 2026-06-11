'use strict';

const STORAGE_KEY = 'ob_bookmarks';
let currentFolder = '';
let viewMode      = 'grid';

// ─────────────────────────────────────────────────────────────────────────────
// Data
// ─────────────────────────────────────────────────────────────────────────────

function loadBookmarks() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    if (raw) return JSON.parse(raw);
  } catch (_) { /* ignore */ }
  return { bookmarks: [], folders: [] };
}

function saveData(data) {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(data)); } catch (_) {}
}

function removeBookmark(id) {
  const data = loadBookmarks();
  data.bookmarks = data.bookmarks.filter(b => b.id !== id);
  saveData(data);
  renderBookmarks();
}

// ─────────────────────────────────────────────────────────────────────────────
// Rendering
// ─────────────────────────────────────────────────────────────────────────────

function getFaviconUrl(url) {
  try { return `${new URL(url).origin}/favicon.ico`; } catch (_) { return ''; }
}

function formatDate(ts) {
  if (!ts) return '';
  return new Date(ts * 1000).toLocaleDateString();
}

function renderBookmarks(query = '') {
  const container  = document.getElementById('bookmark-container');
  const emptyState = document.getElementById('empty-state');
  if (!container) return;

  const data = loadBookmarks();
  let items  = data.bookmarks;

  if (currentFolder) {
    items = items.filter(b => b.folder === currentFolder);
  }
  if (query) {
    const lq = query.toLowerCase();
    items = items.filter(b =>
      b.title?.toLowerCase().includes(lq) || b.url?.toLowerCase().includes(lq)
    );
  }

  // Apply view mode class
  container.className = viewMode === 'grid' ? 'bookmarks-grid' : 'bookmarks-list';

  if (items.length === 0) {
    container.innerHTML = '';
    if (emptyState) emptyState.style.display = 'flex';
    return;
  }
  if (emptyState) emptyState.style.display = 'none';

  container.innerHTML = items.map(b => viewMode === 'grid'
    ? renderGridCard(b)
    : renderListRow(b)
  ).join('');

  // Attach delete listeners
  container.querySelectorAll('[data-delete-id]').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.preventDefault();
      e.stopPropagation();
      const id = parseInt(btn.dataset.deleteId, 10);
      if (confirm('Remove this bookmark?')) removeBookmark(id);
    });
  });
}

function renderGridCard(b) {
  const favicon = getFaviconUrl(b.url);
  return `
    <a class="bookmark-card" href="${escHtml(b.url)}" title="${escHtml(b.title || b.url)}">
      <img class="bookmark-favicon" src="${escHtml(favicon)}" alt=""
           onerror="this.style.display='none'" />
      <div class="bookmark-title">${escHtml(b.title || b.url)}</div>
      <div class="bookmark-url">${escHtml(truncateUrl(b.url))}</div>
      <div class="bookmark-actions">
        <button class="action-btn delete" data-delete-id="${b.id}"
                aria-label="Remove bookmark">✕</button>
      </div>
    </a>`;
}

function renderListRow(b) {
  const favicon = getFaviconUrl(b.url);
  return `
    <a class="bookmark-row" href="${escHtml(b.url)}" title="${escHtml(b.title || b.url)}">
      <img class="bookmark-row-favicon" src="${escHtml(favicon)}" alt=""
           onerror="this.style.display='none'" />
      <span class="bookmark-row-title">${escHtml(b.title || b.url)}</span>
      <span class="bookmark-row-url">${escHtml(truncateUrl(b.url))}</span>
      <span class="bookmark-row-date">${escHtml(formatDate(b.created_at))}</span>
      <button class="action-btn delete" data-delete-id="${b.id}"
              aria-label="Remove bookmark">✕</button>
    </a>`;
}

function renderFolders() {
  const list = document.getElementById('folder-list');
  if (!list) return;

  const data = loadBookmarks();

  const folderItems = data.folders.map(f => `
    <li>
      <button class="folder-item ${currentFolder === f.name ? 'active' : ''}"
              data-folder="${escHtml(f.name)}">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"
             stroke-linecap="round" stroke-linejoin="round" aria-hidden="true">
          <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/>
        </svg>
        ${escHtml(f.name)}
      </button>
    </li>`).join('');

  // Preserve "All Bookmarks" item
  const allItem = list.firstElementChild;
  if (allItem) {
    allItem.className = currentFolder === '' ? 'active' : '';
    // Remove all but first
    while (list.children.length > 1) list.removeChild(list.lastChild);
    // Re-insert folder items
    list.insertAdjacentHTML('beforeend', folderItems);
  }

  list.querySelectorAll('.folder-item').forEach(btn => {
    btn.addEventListener('click', () => {
      currentFolder = btn.dataset.folder;
      list.querySelectorAll('.folder-item').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      renderBookmarks(document.getElementById('search-input')?.value || '');
    });
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

function escHtml(str) {
  return String(str ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function truncateUrl(url) {
  try {
    const u = new URL(url);
    return u.hostname + (u.pathname !== '/' ? u.pathname.substring(0, 30) : '');
  } catch (_) { return url.substring(0, 40); }
}

// ─────────────────────────────────────────────────────────────────────────────
// Init
// ─────────────────────────────────────────────────────────────────────────────

function init() {
  renderFolders();
  renderBookmarks();

  document.getElementById('search-input')?.addEventListener('input', (e) => {
    renderBookmarks(e.target.value);
  });

  document.getElementById('grid-view-btn')?.addEventListener('click', () => {
    viewMode = 'grid';
    document.getElementById('grid-view-btn')?.classList.add('active');
    document.getElementById('list-view-btn')?.classList.remove('active');
    renderBookmarks(document.getElementById('search-input')?.value || '');
  });

  document.getElementById('list-view-btn')?.addEventListener('click', () => {
    viewMode = 'list';
    document.getElementById('list-view-btn')?.classList.add('active');
    document.getElementById('grid-view-btn')?.classList.remove('active');
    renderBookmarks(document.getElementById('search-input')?.value || '');
  });

  document.getElementById('folder-all')?.addEventListener('click', () => {
    currentFolder = '';
    document.querySelectorAll('.folder-item').forEach(b => b.classList.remove('active'));
    document.getElementById('folder-all')?.classList.add('active');
    renderBookmarks(document.getElementById('search-input')?.value || '');
  });

  document.getElementById('add-folder-btn')?.addEventListener('click', () => {
    const name = prompt('Folder name:');
    if (!name?.trim()) return;
    const data = loadBookmarks();
    const id = data.folders.length > 0
      ? Math.max(...data.folders.map(f => f.id)) + 1 : 1;
    data.folders.push({ id, name: name.trim(), parent_id: 0 });
    saveData(data);
    renderFolders();
  });
}

if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', init);
} else {
  init();
}
