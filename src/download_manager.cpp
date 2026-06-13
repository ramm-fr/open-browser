// Open Browser — download_manager.cpp

#include "download_manager.h"

#include "settings_manager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace open_browser {

// ─────────────────────────────────────────────────────────────────────────────

DownloadManager &DownloadManager::instance() {
  static DownloadManager inst;
  return inst;
}

// ─────────────────────────────────────────────────────────────────────────────
// Start
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::start_download(WebKitDownload *wd,
                                     const std::string &destination) {
  Download dl;
  dl.id = next_id_++;
  dl.webkit_download = wd;
  dl.state = DownloadState::Downloading;

  // URL
  WebKitURIRequest *request = webkit_download_get_request(wd);
  if (request) {
    const char *uri = webkit_uri_request_get_uri(request);
    if (uri)
      dl.url = uri;
  }

  // Filename from response suggested filename
  const char *suggested = nullptr;
  WebKitURIResponse *early_response = webkit_download_get_response(wd);
  if (early_response) {
    suggested = webkit_uri_response_get_suggested_filename(early_response);
  }
  dl.filename = suggested ? suggested : "download";

  // Destination path
  if (!destination.empty()) {
    dl.destination = destination + "/" + dl.filename;
  } else {
    dl.destination =
        SettingsManager::instance().download_path() + "/" + dl.filename;
  }

  // Ensure the destination directory exists
  std::filesystem::create_directories(
      std::filesystem::path(dl.destination).parent_path());

  webkit_download_set_destination(wd, dl.destination.c_str());

  // Connect signals
  g_signal_connect(wd, "received-data", G_CALLBACK(on_received_data), this);
  g_signal_connect(wd, "finished", G_CALLBACK(on_finished), this);
  g_signal_connect(wd, "failed", G_CALLBACK(on_failed), this);

  downloads_.push_back(std::move(dl));
}

// ─────────────────────────────────────────────────────────────────────────────
// Controls
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::pause_download(int64_t id) {
  Download *dl = find_by_id(id);
  if (!dl || !dl->webkit_download)
    return;
  if (dl->state == DownloadState::Downloading) {
    dl->state = DownloadState::Paused;
    // WebKitGTK 2.x does not expose a direct pause API; cancellation is
    // the closest equivalent. A real implementation would use a custom
    // stream or content-length range request to resume.
  }
}

void DownloadManager::resume_download(int64_t id) {
  Download *dl = find_by_id(id);
  if (!dl)
    return;
  if (dl->state == DownloadState::Paused) {
    dl->state = DownloadState::Downloading;
  }
}

void DownloadManager::cancel_download(int64_t id) {
  Download *dl = find_by_id(id);
  if (!dl || !dl->webkit_download)
    return;
  webkit_download_cancel(dl->webkit_download);
  dl->state = DownloadState::Cancelled;
}

void DownloadManager::remove_download(int64_t id) {
  auto it = std::find_if(downloads_.begin(), downloads_.end(),
                         [id](const Download &d) { return d.id == id; });
  if (it != downloads_.end())
    downloads_.erase(it);
}

void DownloadManager::clear_completed() {
  downloads_.erase(
      std::remove_if(downloads_.begin(), downloads_.end(),
                     [](const Download &d) {
                       return d.state == DownloadState::Completed ||
                              d.state == DownloadState::Cancelled ||
                              d.state == DownloadState::Failed;
                     }),
      downloads_.end());
}

// ─────────────────────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Download> DownloadManager::get_all() const {
  return downloads_;
}

std::vector<Download> DownloadManager::get_active() const {
  std::vector<Download> result;
  for (const auto &dl : downloads_) {
    if (dl.state == DownloadState::Downloading ||
        dl.state == DownloadState::Paused ||
        dl.state == DownloadState::Pending) {
      result.push_back(dl);
    }
  }
  return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::set_progress_callback(
    std::function<void(const Download &)> cb) {
  progress_cb_ = std::move(cb);
}

void DownloadManager::set_completion_callback(
    std::function<void(const Download &)> cb) {
  completion_cb_ = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

Download *DownloadManager::find_by_id(int64_t id) {
  auto it = std::find_if(downloads_.begin(), downloads_.end(),
                         [id](const Download &d) { return d.id == id; });
  return it != downloads_.end() ? &(*it) : nullptr;
}

Download *DownloadManager::find_by_webkit(WebKitDownload *wd) {
  auto it =
      std::find_if(downloads_.begin(), downloads_.end(),
                   [wd](const Download &d) { return d.webkit_download == wd; });
  return it != downloads_.end() ? &(*it) : nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// WebKit signal handlers
// ─────────────────────────────────────────────────────────────────────────────

void DownloadManager::on_received_data(WebKitDownload *wd, guint64 /*length*/,
                                       gpointer user_data) {
  auto *self = static_cast<DownloadManager *>(user_data);
  Download *dl = self->find_by_webkit(wd);
  if (!dl)
    return;

  dl->received_bytes =
      static_cast<int64_t>(webkit_download_get_received_data_length(wd));
  // Total size may not be known until headers arrive
  WebKitURIResponse *response = webkit_download_get_response(wd);
  if (response) {
    const gint64 len = webkit_uri_response_get_content_length(response);
    if (len > 0)
      dl->total_bytes = len;
    const char *mime = webkit_uri_response_get_mime_type(response);
    if (mime)
      dl->mime_type = mime;
  }

  if (self->progress_cb_)
    self->progress_cb_(*dl);
}

void DownloadManager::on_finished(WebKitDownload *wd, gpointer user_data) {
  auto *self = static_cast<DownloadManager *>(user_data);
  Download *dl = self->find_by_webkit(wd);
  if (!dl)
    return;

  dl->state = DownloadState::Completed;
  dl->received_bytes = dl->total_bytes;

  if (self->completion_cb_)
    self->completion_cb_(*dl);
}

void DownloadManager::on_failed(WebKitDownload *wd, GError *error,
                                gpointer user_data) {
  auto *self = static_cast<DownloadManager *>(user_data);
  Download *dl = self->find_by_webkit(wd);
  if (!dl)
    return;

  if (error && error->domain == WEBKIT_DOWNLOAD_ERROR &&
      error->code == WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER) {
    dl->state = DownloadState::Cancelled;
  } else {
    dl->state = DownloadState::Failed;
    if (error) {
      std::cerr << "[DownloadManager] Download failed: " << error->message
                << "\n";
    }
  }

  if (self->completion_cb_)
    self->completion_cb_(*dl);
}

} // namespace open_browser
