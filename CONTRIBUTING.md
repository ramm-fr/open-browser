# Contributing to Open Browser

Thank you for your interest in contributing to Open Browser. This guide covers everything you need to get started.

---

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Code Style](#code-style)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)
- [Commit Messages](#commit-messages)
- [Reporting Bugs](#reporting-bugs)
- [Feature Requests](#feature-requests)

---

## Code of Conduct

This project follows the [Contributor Covenant](https://www.contributor-covenant.org/version/2/1/code_of_conduct/). Be respectful, constructive, and collaborative.

---

## Getting Started

1. Fork the repository on GitHub.
2. Clone your fork: `git clone https://github.com/YOUR_USERNAME/open-browser.git`
3. Create a branch: `git checkout -b feature/my-feature` or `git checkout -b fix/issue-123`
4. Set up your build environment (see [docs/build.md](docs/build.md)).
5. Make your changes.
6. Run the test suite: `cd build && ctest --output-on-failure`
7. Push and open a pull request.

---

## Code Style

### C++

- **Standard**: C++20
- **Naming**:
  - Classes: `PascalCase`
  - Functions and methods: `snake_case`
  - Member variables: `trailing_underscore_`
  - Constants: `kCamelCase`
  - Namespaces: `snake_case`
- **Formatting**: Use `clang-format` with the project's `.clang-format` file before committing.
- **Headers**: Use `#pragma once`. Group includes: system headers, GTK/WebKit headers, project headers.
- **RAII**: Prefer smart pointers and RAII wrappers over manual memory management.
- **Error handling**: Use return values or exceptions. Never silently swallow errors.
- **Comments**: Write comments for *why*, not *what*. Keep them up to date.

```cpp
// Good
class BrowserWindow {
public:
    explicit BrowserWindow(GtkApplication* app);
    void navigate(const std::string& url);

private:
    GtkApplication* app_;
    GtkWidget* window_;
};

// Bad
class browserwindow {
public:
    browserwindow(GtkApplication* App);
    void Navigate(std::string URL);
};
```

### HTML / CSS / JavaScript

- 2-space indentation
- Use `const` and `let`, never `var`
- ES2020+ features are acceptable
- CSS variables for all theme values
- Semantic HTML elements

---

## Pull Request Process

1. **One PR per feature or fix.** Keep PRs focused and small where possible.
2. **Describe your change** clearly in the PR description: what it does, why it's needed, and how to test it.
3. **Link related issues** using `Fixes #123` or `Closes #123` in the PR body.
4. **All CI checks must pass** before a PR can be merged.
5. **Request a review** from at least one maintainer.
6. **Address review feedback** within a reasonable time or the PR may be closed.
7. **Squash or rebase** before merging to keep the history clean.

### PR Title Format

```
type(scope): short description

Examples:
feat(tabs): add tab preview on hover
fix(downloads): prevent duplicate entries on resume
chore(ci): update Ubuntu matrix to 24.04
docs(build): clarify Fedora dependency names
```

Types: `feat`, `fix`, `chore`, `docs`, `refactor`, `test`, `style`, `perf`

---

## Testing Requirements

- All new features must include unit tests.
- All bug fixes must include a regression test.
- Tests live in `tests/` and use Google Test.
- Run the full suite before submitting: `ctest --test-dir build -V`
- Aim for coverage on all public API methods.
- UI logic that can be extracted into testable units should be.

---

## Commit Messages

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<scope>): <short description>

[optional body]

[optional footer(s)]
```

- Use present tense: "add feature" not "added feature"
- Keep the subject line under 72 characters
- Reference issues in the footer: `Fixes #42`

---

## Reporting Bugs

Use the GitHub issue tracker. Include:

- Open Browser version (`open-browser --version`)
- Linux distribution and version
- GTK and WebKitGTK versions (`pkg-config --modversion gtk4 webkit2gtk-4.1`)
- Steps to reproduce
- Expected vs actual behavior
- Any relevant logs from `~/.local/share/open-browser/logs/`

---

## Feature Requests

Open a GitHub Discussion before implementing significant features. This lets the community discuss the approach before you invest time in an implementation.

For small, clearly scoped features, a PR with a description is fine.

---

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.
