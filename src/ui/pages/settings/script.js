'use strict';

const STORAGE_KEY = 'ob_settings';

const DEFAULTS = {
  theme:                 'dark',
  font_size:             16,
  default_zoom:          100,
  show_bookmarks_bar:    false,
  block_ads:             true,
  block_trackers:        true,
  https_upgrade:         true,
  clear_on_exit:         false,
  search_engine:         'https://search.brave.com/search?q=',
  download_path:         '~/Downloads',
  ask_download_location: false,
  save_passwords:        true,
  autofill_passwords:    true,
  hardware_acceleration: true,
  memory_saver:          false
};

// ── Storage Core Engine Architecture ──────────────────────────────────────
function loadSettings() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? { ...DEFAULTS, ...JSON.parse(raw) } : { ...DEFAULTS };
  } catch (_) { return { ...DEFAULTS }; }
}

function saveSettings(s) {
  try { localStorage.setItem(STORAGE_KEY, JSON.stringify(s)); } catch (_) {}
  showToast();
}

// ── Theme Management ──────────────────────────────────────────────────────
function applyTheme(theme) {
  let effective = theme;
  if (theme === 'system') {
    effective = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
  }
  document.documentElement.setAttribute('data-theme', effective);
  document.body.style.backgroundColor = effective === 'dark' ? '#000000' : '#ffffff';
  document.body.style.color = effective === 'dark' ? '#ffffff' : '#000000';
}

// ── Toast Status Notification Banner ──────────────────────────────────────
let _toastTimer = null;
function showToast() {
  const el = document.getElementById('save-toast');
  if (!el) return;
  el.classList.add('show');
  clearTimeout(_toastTimer);
  _toastTimer = setTimeout(() => el.classList.remove('show'), 1500);
}

// ── Sync Dom Values With Current Settings State ───────────────────────────
function populateForm(s) {
  // Theme Radio Bindings
  document.querySelectorAll('input[name="theme"]').forEach(r => r.checked = (r.value === s.theme));
  
  // Select Dropdowns
  const zoomSelect = document.getElementById('zoom-select');
  if (zoomSelect) zoomSelect.value = String(s.default_zoom);
  
  const searchSelect = document.getElementById('search-engine-select');
  if (searchSelect) searchSelect.value = s.search_engine.includes('custom') ? 'custom' : s.search_engine;

  // Custom Font Range Parameters
  const fi = document.getElementById('font-size-input');
  const fo = document.getElementById('font-size-output');
  if (fi) { fi.value = s.font_size; if (fo) fo.textContent = s.font_size; }

  // Checkbox Switches
  setCheck('bookmarks-bar-toggle',      s.show_bookmarks_bar);
  setCheck('block-ads-toggle',          s.block_ads);
  setCheck('block-trackers-toggle',     s.block_trackers);
  setCheck('https-upgrade-toggle',      s.https_upgrade);
  setCheck('clear-exit-toggle',         s.clear_on_exit);
  setCheck('ask-download-toggle',       s.ask_download_location);
  setCheck('save-passwords-toggle',     s.save_passwords);
  setCheck('autofill-passwords-toggle', s.autofill_passwords);
  setCheck('hw-accel-toggle',           s.hardware_acceleration);
  setCheck('memory-saver-toggle',       s.memory_saver);

  const dlPath = document.getElementById('download-path-input');
  if (dlPath) dlPath.value = s.download_path || '~/Downloads';
}

function setCheck(id, v) {
  const el = document.getElementById(id);
  if (el) el.checked = Boolean(v);
}

// ── Form Content Capture and Compilation Matrix ───────────────────────────
function collectSettings() {
  const s = { ...DEFAULTS };
  const checkedTheme = document.querySelector('input[name="theme"]:checked');
  s.theme                  = checkedTheme ? checkedTheme.value : 'dark';
  s.default_zoom           = parseInt(val('zoom-select'), 10) || 100;
  s.font_size              = parseInt(val('font-size-input'), 10) || 16;
  s.show_bookmarks_bar     = chk('bookmarks-bar-toggle');
  s.block_ads              = chk('block-ads-toggle');
  s.block_trackers         = chk('block-trackers-toggle');
  s.https_upgrade          = chk('https-upgrade-toggle');
  s.clear_on_exit          = chk('clear-exit-toggle');
  s.ask_download_location  = chk('ask-download-toggle');
  s.save_passwords         = chk('save-passwords-toggle');
  s.autofill_passwords     = chk('autofill-passwords-toggle');
  s.hardware_acceleration  = chk('hw-accel-toggle');
  s.memory_saver           = chk('memory-saver-toggle');
  s.download_path          = val('download-path-input');

  const eng = document.getElementById('search-engine-select');
  s.search_engine = (eng && eng.value === 'custom') ? val('custom-engine-input') : val('search-engine-select');
  return s;
}

function val(id) { return document.getElementById(id)?.value ?? ''; }
function chk(id) { return document.getElementById(id)?.checked ?? false; }

// ── Sidebar Active Section Swapping Routines ──────────────────────────────
function initNav() {
  const btns     = document.querySelectorAll('.sidebar-nav-btn');
  const sections = document.querySelectorAll('.settings-section');

  function activate(target) {
    btns.forEach(b => b.classList.toggle('is-active', b.dataset.section === target));
    sections.forEach(s => s.classList.toggle('is-active', s.id === target));
  }

  btns.forEach(b => b.addEventListener('click', () => activate(b.dataset.section)));
}

// ── Form Input Change Event Wire Binding ──────────────────────────────────
function initEvents() {
  // Theme inputs auto-save trigger wire
  document.querySelectorAll('input[name="theme"]').forEach(radio => {
    radio.addEventListener('change', () => {
      applyTheme(radio.value);
      saveSettings(collectSettings());
    });
  });

  // Track regular parameter changes
  document.querySelectorAll('input:not([name="theme"]), select').forEach(el => {
    el.addEventListener('change', () => saveSettings(collectSettings()));
  });

  // Font metrics live text slider track monitor
  const fi = document.getElementById('font-size-input');
  const fo = document.getElementById('font-size-output');
  fi?.addEventListener('input', () => { if (fo) fo.textContent = fi.value; });

  // Custom Search bar tracking conditions
  const engSel = document.getElementById('search-engine-select');
  const custFld = document.getElementById('custom-engine-field');
  function toggleCustom() {
    if (custFld) custFld.style.display = (engSel?.value === 'custom') ? 'flex' : 'none';
  }
  engSel?.addEventListener('change', toggleCustom);
  toggleCustom();

  // Wipe Data Event Method Call
  document.getElementById('clear-data-btn')?.addEventListener('click', () => {
    if (!confirm('Purge system cookies, cached images, and tracking records?')) return;
    localStorage.clear();
    alert('Local system context records wiped cleanly.');
  });

  // Simulator for update buttons updates status check click response
  document.getElementById('check-update-btn')?.addEventListener('click', () => {
    const btn = document.getElementById('check-update-btn');
    if (!btn) return;
    btn.textContent = 'Checking...';
    setTimeout(() => { btn.textContent = "Up to date ✓"; }, 1200);
  });
}

// ── Application Initialization Lifecycle ──────────────────────────────────
function init() {
  const s = loadSettings();
  applyTheme(s.theme);
  populateForm(s);
  initNav();
  initEvents();
  // Render Vector Icon Layer Objects smoothly
  if (window.lucide) { lucide.createIcons(); }
}

document.readyState === 'loading'
  ? document.addEventListener('DOMContentLoaded', init)
  : init();
