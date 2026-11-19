export async function api(path, options = {}) {
  const response = await fetch(`/api${path}`, {
    ...options,
    headers: {
      ...(options.body ? { "Content-Type": "application/json" } : {}),
      ...(options.headers || {})
    }
  });
  let payload;
  try {
    payload = await response.json();
  } catch {
    throw new Error(`Server returned an invalid response (${response.status})`);
  }
  if (!response.ok || payload.ok === false) {
    throw new Error(payload.error || `Request failed (${response.status})`);
  }
  return payload.data;
}

export function escapeHtml(value) {
  return String(value ?? "").replace(/[&<>"']/g, (character) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[character]);
}

export function initials(name) {
  return String(name || "?").trim().split(/\s+/).slice(0, 2)
    .map((part) => part[0] || "").join("").toUpperCase();
}

export function showToast(message, isError = false) {
  const region = document.querySelector("#toast-region");
  const toast = document.createElement("div");
  toast.className = `toast${isError ? " error" : ""}`;
  toast.textContent = message;
  region.append(toast);
  setTimeout(() => toast.remove(), 3600);
}

export function showLoading(container) {
  container.innerHTML = '<div class="loading"><span class="spinner"></span>Loading from the C engine…</div>';
}

export function userOptions(users, selected = "") {
  return users.map((user) =>
    `<option value="${user.id}" ${String(user.id) === String(selected) ? "selected" : ""}>${escapeHtml(user.name)} · #${user.id}</option>`
  ).join("");
}
