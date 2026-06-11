'use strict';

const STORAGE_KEY = 'ob_settings';
const THEME_KEY   = 'ob_theme';

const DEFAULTS = {
  theme:                 'light',
  font_size:             16,
  default_zoom:          100,
  show_bookmarks_bar:    false,
  block_ads:             true,
  block_trackers:        true,
  https_upgrade:         true,
  clear_on_exit:         false,
  search_engine:         'https://search.brave.com/search?q=',
  download_path:         '',
  ask_download_location: false,
  save_passwords:        true,
  autofill_passwords:    true,
  hardware_acceleration: true,
  sleeping_tabs:         true,
  memory_saver:          false,
  smooth_scrolling:      true,
};

// ── Storage ──────────────────────────────────────────────────────────────
function loadSettings() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    const s   = raw ? { ...DEFAULTS, ...JSON.parse(raw) } : { ...DEFAULTS };
    // Merge persisted theme
    const savedTheme = localStorage.getItem(THEME_KEY);
    if (savedTheme) s.theme = savedTheme;
    return s;
  } catch (_) { return { ...DEFAULTS }; }
}

function saveSettings(s) {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(s)); } catch (_) {}
  if (window.browserSettings?.save) window.browserSettings.save(s);
  showToast();
}

// ── Theme ─────────────────────────────────────────────────────────────────
// applyTheme: sets data-theme on <html>, persists it, notifies C++ bridge,
// and syncs every other open tab through window.webkit.messageHandlers.
function applyTheme(theme) {
  const effective = resolveTheme(theme);
  document.documentElement.setAttribute('data-theme', effective);
  try { localStorage.setItem(THEME_KEY, theme); } catch (_) {}

  // Notify C++ bridge → it broadcasts JS to every other tab
  try {
    window.webkit.messageHandlers.obBridge.postMessage('theme:' + effective);
  } catch (_) { /* no bridge in dev */ }
}

function resolveTheme(theme) {
  if (theme === 'system') {
    return window.matchMedia('(prefers-color-scheme: dark)').matches
      ? 'dark' : 'light';
  }
  return theme;
}

function syncThemeUI(theme) {
  const effective = resolveTheme(theme);
  // Radio cards
  document.querySelectorAll('input[name="theme"]').forEach(r => {
    r.checked = r.value === theme;
  });
  // Sidebar dark toggle
  const toggle = document.getElementById('global-dark-toggle');
  if (toggle) toggle.checked = effective === 'dark';
}

// ── Toast ─────────────────────────────────────────────────────────────────
let _toastTimer = null;
function showToast() {
  const el = document.getElementById('save-toast');
  if (!el) return;
  el.classList.add('show');
  clearTimeout(_toastTimer);
  _toastTimer = setTimeout(() => el.classList.remove('show'), 1800);
}

// ── Populate form ─────────────────────────────────────────────────────────
function populateForm(s) {
  setSelect('zoom-select',          String(s.default_zoom));
  setSelect('search-engine-select', s.search_engine);

  const fi = document.getElementById('font-size-input');
  const fo = document.getElementById('font-size-output');
  if (fi) { fi.value = s.font_size; if (fo) fo.textContent = s.font_size; }

  setCheck('bookmarks-bar-toggle',      s.show_bookmarks_bar);
  setCheck('block-ads-toggle',          s.block_ads);
  setCheck('block-trackers-toggle',     s.block_trackers);
  setCheck('https-upgrade-toggle',      s.https_upgrade);
  setCheck('clear-exit-toggle',         s.clear_on_exit);
  setCheck('ask-download-toggle',       s.ask_download_location);
  setCheck('save-passwords-toggle',     s.save_passwords);
  setCheck('autofill-passwords-toggle', s.autofill_passwords);
  setCheck('hw-accel-toggle',           s.hardware_acceleration);
  setCheck('sleeping-tabs-toggle',      s.sleeping_tabs);
  setCheck('memory-saver-toggle',       s.memory_saver);
  setCheck('smooth-scroll-toggle',      s.smooth_scrolling);

  const dlPath = document.getElementById('download-path-input');
  if (dlPath) dlPath.value = s.download_path || '~/Downloads';
}

function setSelect(id, value) {
  const el = document.getElementById(id);
  if (!el) return;
  el.value = Array.from(el.options).some(o => o.value === value)
    ? value : 'custom';
}
function setCheck(id, v) {
  const el = document.getElementById(id);
  if (el) el.checked = Boolean(v);
}

// ── Collect form ──────────────────────────────────────────────────────────
function collectSettings() {
  const s = { ...DEFAULTS };
  const checkedTheme = document.querySelector('input[name="theme"]:checked');
  s.theme                  = checkedTheme?.value ?? 'light';
  s.default_zoom           = parseInt(val('zoom-select'), 10)      || 100;
  s.font_size              = parseInt(val('font-size-input'), 10)   || 16;
  s.show_bookmarks_bar     = chk('bookmarks-bar-toggle');
  s.block_ads              = chk('block-ads-toggle');
  s.block_trackers         = chk('block-trackers-toggle');
  s.https_upgrade          = chk('https-upgrade-toggle');
  s.clear_on_exit          = chk('clear-exit-toggle');
  s.ask_download_location  = chk('ask-download-toggle');
  s.save_passwords         = chk('save-passwords-toggle');
  s.autofill_passwords     = chk('autofill-passwords-toggle');
  s.hardware_acceleration  = chk('hw-accel-toggle');
  s.sleeping_tabs          = chk('sleeping-tabs-toggle');
  s.memory_saver           = chk('memory-saver-toggle');
  s.smooth_scrolling       = chk('smooth-scroll-toggle');
  s.download_path          = val('download-path-input');
  const eng = document.getElementById('search-engine-select');
  s.search_engine = eng?.value === 'custom'
    ? (val('custom-engine-input') || DEFAULTS.search_engine)
    : (val('search-engine-select') || DEFAULTS.search_engine);
  return s;
}
function val(id) { return document.getElementById(id)?.value ?? ''; }
function chk(id) { return document.getElementById(id)?.checked ?? false; }

// ── Sidebar navigation ────────────────────────────────────────────────────
function initNav() {
  const btns     = document.querySelectorAll('.nav-btn');
  const sections = document.querySelectorAll('.section');

  function activate(target) {
    btns.forEach(b => b.classList.toggle('active', b.dataset.section === target));
    sections.forEach(s => s.classList.toggle('active', s.id === target));
  }

  btns.forEach(b => b.addEventListener('click', () => activate(b.dataset.section)));

  const hash = window.location.hash.slice(1);
  if (hash && document.getElementById(hash)) activate(hash);
}

// ── Event wiring ──────────────────────────────────────────────────────────
function initEvents() {
  // ── Theme radio cards ────────────────────────────────────────────────
  document.querySelectorAll('input[name="theme"]').forEach(radio => {
    radio.addEventListener('change', () => {
      applyTheme(radio.value);
      // Sync sidebar toggle
      const t = document.getElementById('global-dark-toggle');
      if (t) t.checked = resolveTheme(radio.value) === 'dark';
      saveSettings(collectSettings());
    });
  });

  // ── Sidebar dark toggle ──────────────────────────────────────────────
  document.getElementById('global-dark-toggle')?.addEventListener('change', (e) => {
    const theme = e.target.checked ? 'dark' : 'light';
    applyTheme(theme);
    // Sync radio cards
    document.querySelectorAll('input[name="theme"]').forEach(r => {
      r.checked = r.value === theme;
    });
    saveSettings(collectSettings());
  });

  // ── All other inputs: auto-save ──────────────────────────────────────
  document.querySelectorAll('input:not([name="theme"]):not(#global-dark-toggle), select')
    .forEach(el => el.addEventListener('change', () => saveSettings(collectSettings())));

  // ── Font size range: live display ────────────────────────────────────
  const fi = document.getElementById('font-size-input');
  const fo = document.getElementById('font-size-output');
  fi?.addEventListener('input', () => { if (fo) fo.textContent = fi.value; });

  // ── Custom search engine field ───────────────────────────────────────
  const engSel  = document.getElementById('search-engine-select');
  const custDiv = document.getElementById('custom-divider');
  const custFld = document.getElementById('custom-engine-field');
  function toggleCustom() {
    const show = engSel?.value === 'custom';
    if (custDiv) custDiv.style.display = show ? '' : 'none';
    if (custFld) custFld.style.display = show ? '' : 'none';
  }
  engSel?.addEventListener('change', toggleCustom);
  toggleCustom();

  // ── Clear data ───────────────────────────────────────────────────────
  document.getElementById('clear-data-btn')?.addEventListener('click', () => {
    if (!confirm('Delete all cookies, cache, history and saved data?')) return;
    if (window.browserActions?.clearData) {
      window.browserActions.clearData();
    } else {
      localStorage.clear();
      sessionStorage.clear();
      alert('Data cleared. Restart the browser to apply fully.');
    }
  });

  // ── View passwords ───────────────────────────────────────────────────
  document.getElementById('view-passwords-btn')?.addEventListener('click', () => {
    const card = document.getElementById('passwords-list-card');
    const btn  = document.getElementById('view-passwords-btn');
    if (!card || !btn) return;
    const visible = card.style.display === 'block';
    card.style.display = visible ? 'none' : 'block';
    btn.textContent = visible ? 'View' : 'Hide';
  });

  // ── Check for updates ────────────────────────────────────────────────
  document.getElementById('check-update-btn')?.addEventListener('click', () => {
    const btn = document.getElementById('check-update-btn');
    if (!btn) return;
    btn.textContent = 'Checking…';
    btn.disabled = true;
    setTimeout(() => {
      btn.textContent = "You're up to date ✓";
      btn.disabled = false;
    }, 1500);
  });

  // ── System theme change ──────────────────────────────────────────────
  window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', () => {
    const s = loadSettings();
    if (s.theme === 'system') applyTheme('system');
  });
}

// ── Init ──────────────────────────────────────────────────────────────────
function init() {
  const s = loadSettings();
  applyTheme(s.theme);
  syncThemeUI(s.theme);
  populateForm(s);
  initNav();
  initEvents();
}

document.readyState === 'loading'
  ? document.addEventListener('DOMContentLoaded', init)
  : init();
