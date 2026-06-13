#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#ifdef USE_WEBKIT2GTK
#include <webkit2/webkit2.h>
#else
#include <webkit/webkit.h>
#endif

namespace open_browser {

enum class DownloadState {
  Pending,
  Downloading,
  Paused,
  Completed,
  Failed,
  Cancelled
};

struct Download {
  int64_t id = 0;
  std::string url;
  std::string filename;
  std::string destination;
  int64_t total_bytes = 0;
  int64_t received_bytes = 0;
  DownloadState state = DownloadState::Pending;
  std::string mime_type;
  WebKitDownload *webkit_download = nullptr;
};

// Singleton download manager. Wraps WebKitDownload objects and exposes
// pause/resume/cancel operations with progress callbacks.
class DownloadManager {
public:
  static DownloadManager &instance();

  // Called by the WebView's "download" signal with the new WebKitDownload.
  // `destination` is optional; if empty, the configured download path is used.
  void start_download(WebKitDownload *download,
                      const std::string &destination = "");

  void pause_download(int64_t id);
  void resume_download(int64_t id);
  void cancel_download(int64_t id);
  void remove_download(int64_t id);
  void clear_completed();

  std::vector<Download> get_all() const;
  std::vector<Download> get_active() const;

  // Optional callbacks fired from WebKit signal handlers.
  void set_progress_callback(std::function<void(const Download &)> cb);
  void set_completion_callback(std::function<void(const Download &)> cb);

private:
  DownloadManager() = default;
  DownloadManager(const DownloadManager &) = delete;
  DownloadManager &operator=(const DownloadManager &) = delete;

  Download *find_by_id(int64_t id);
  Download *find_by_webkit(WebKitDownload *wd);

  // WebKit signal callbacks
  static void on_received_data(WebKitDownload *wd, guint64 length,
                               gpointer user_data);
  static void on_finished(WebKitDownload *wd, gpointer user_data);
  static void on_failed(WebKitDownload *wd, GError *error, gpointer user_data);

  std::vector<Download> downloads_;
  int64_t next_id_ = 1;
  std::function<void(const Download &)> progress_cb_;
  std::function<void(const Download &)> completion_cb_;
};

} // namespace open_browser
