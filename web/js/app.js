import { api, escapeHtml, showLoading, showToast } from "./api.js";
import { renderUsers } from "./users.js";
import { renderFriendships } from "./friendships.js";
import { renderGroups } from "./groups.js";
import { renderSuggestions } from "./suggestions.js";
import { renderDirectory } from "./directory.js";
import { renderInspector } from "./inspector.js";
import { renderNetwork } from "./network.js";
import { renderPerformance } from "./performance.js";
import { renderDataManagement } from "./data.js";

const page = document.querySelector("#page");
const titles = {
  dashboard: "Dashboard",
  users: "Users",
  friendships: "Friendships",
  groups: "Communities",
  suggestions: "Suggestions",
  network: "Network graph",
  directory: "AVL directory",
  inspector: "Data structure inspector",
  performance: "Performance lab",
  data: "Data management"
};

function setTheme(theme) {
  document.documentElement.dataset.theme = theme;
  localStorage.setItem("connectsphere-theme", theme);
}

async function renderDashboard() {
  page.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Network overview</p><h1>Good to see you</h1><p class="page-subtitle">A live view of users, friendships, and the data structures powering your network.</p></div><button class="button secondary" id="refresh-dashboard">↻ Refresh</button></div>
  <div class="stats-grid" id="dashboard-stats"><div class="loading"><span class="spinner"></span>Reading the network…</div></div>
  <div class="dashboard-grid"><section class="panel"><div class="panel-header"><div><h2>Quick actions</h2><p class="subtle" style="margin:5px 0 0">Keep your network moving</p></div><span class="tag purple">C-powered</span></div>
  <div class="quick-actions"><button class="quick-action" data-go="users"><span class="quick-icon violet">♙</span><strong>Add a new user</strong><small>Grow your directory</small><span class="quick-arrow">↗</span></button>
  <button class="quick-action" data-go="friendships"><span class="quick-icon mint">↔</span><strong>Create a friendship</strong><small>Connect two people</small><span class="quick-arrow">↗</span></button>
  <button class="quick-action" id="dashboard-sample"><span class="quick-icon amber">✳</span><strong>Load sample network</strong><small>Explore 18 sample users</small><span class="quick-arrow">↗</span></button></div></section>
  <section class="panel"><div class="panel-header"><div><h2>Data structure stack</h2><p class="subtle" style="margin:5px 0 0">Algorithms run inside the persistent C engine</p></div></div>
  <div class="stack-list"><div><span class="stack-badge violet">UF</span><span><b>Disjoint sets</b><small>Friend group connectivity</small></span><i>O(α(n))</i></div><div><span class="stack-badge blue">H</span><span><b>Separate chaining</b><small>User lookup and rehashing</small></span><i>O(1) avg</i></div><div><span class="stack-badge mint">AVL</span><span><b>AVL directory</b><small>Sorted names and range search</small></span><i>O(log n)</i></div><div><span class="stack-badge amber">PQ</span><span><b>Binary heap</b><small>Ranked network suggestions</small></span><i>O(log n)</i></div><div><span class="stack-badge rose">MS</span><span><b>Merge sort</b><small>Stable community ranking</small></span><i>O(n log n)</i></div></div></section></div>
  <section class="panel recent-panel"><div class="panel-header"><div><h2>Network snapshot</h2><p class="subtle" style="margin:5px 0 0">A few people from the current alphabetical directory</p></div><button class="text-button" data-go="users">View directory →</button></div><div id="recent-users" class="recent-users"><div class="loading"><span class="spinner"></span>Loading directory…</div></div></section>`;
  const load = async () => {
    const stats = page.querySelector("#dashboard-stats");
    const recent = page.querySelector("#recent-users");
    showLoading(stats);
    showLoading(recent);
    try {
      const [users, groups, graph, hash, tree] = await Promise.all([
        api("/users"), api("/groups/ranked"), api("/graph"), api("/structures/hash"), api("/directory/tree")
      ]);
      if (!page.contains(stats)) return;
      const edges = graph.edges.length;
      const largest = groups.reduce((max, group) => Math.max(max, group.size), 0);
      const values = [
        ["Total users", users.length, "Across the directory", "♙", "violet"],
        ["Friendships", edges, "Undirected connections", "↔", "mint"],
        ["Friend groups", groups.length, `${largest} people in largest group`, "◎", "amber"],
        ["Hash load factor", Number(hash.id.loadFactor).toFixed(3), `${hash.id.bucketCount} ID buckets`, "⌗", "blue"],
        ["AVL tree height", tree?.height || 0, "Balanced directory index", "⌕", "rose"]
      ];
      stats.innerHTML = values.map(([label, value, note, icon, tone]) => `<article class="stat-card"><div class="stat-top"><span class="stat-icon ${tone}">${icon}</span><span class="stat-menu">···</span></div><div class="stat-value">${escapeHtml(value)}</div><div class="stat-label">${escapeHtml(label)}</div><div class="stat-note">${escapeHtml(note)}</div></article>`).join("");
      recent.innerHTML = users.length ? users.slice(0, 5).map((user) => `<a class="recent-user" href="#users"><span class="user-avatar">${escapeHtml(user.name.split(/\s+/).slice(0, 2).map((part) => part[0]).join("").toUpperCase())}</span><span><b>${escapeHtml(user.name)}</b><small>${escapeHtml(user.city)}</small></span><span class="recent-count">${user.friendCount} friends</span></a>`).join("") : '<div class="empty-state">No users yet. Load the sample network or add someone.</div>';
      document.querySelector("#engine-status").textContent = "Connected · JSON line";
    } catch (error) {
      if (!page.contains(stats)) return;
      stats.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}<p>Start the Node server and build the C engine.</p></div>`;
      recent.innerHTML = "";
      document.querySelector("#engine-status").textContent = "Unavailable";
      showToast(error.message, true);
    }
  };
  await load();
  const refreshButton = page.querySelector("#refresh-dashboard");
  if (!refreshButton) return;
  refreshButton.addEventListener("click", load);
  page.querySelector("#dashboard-sample").addEventListener("click", async () => {
    if (!window.confirm("Load the sample network and discard unsaved in-memory changes?")) return;
    try {
      await api("/data/sample", { method: "POST" });
      if (!page.querySelector("#dashboard-sample")) return;
      showToast("Sample network loaded.");
      await load();
    } catch (error) { showToast(error.message, true); }
  });
  page.querySelectorAll("[data-go]").forEach((button) =>
    button.addEventListener("click", () => { location.hash = button.dataset.go; }));
}

async function renderPage(name) {
  if (!titles[name]) name = "dashboard";
  document.querySelectorAll("[data-page]").forEach((link) =>
    link.classList.toggle("active", link.dataset.page === name));
  document.querySelector("#page-title").textContent = titles[name];
  document.querySelector("#sidebar").classList.remove("open");
  if (name === "dashboard") return renderDashboard();
  if (name === "users") return renderUsers(page);
  if (name === "friendships") return renderFriendships(page);
  if (name === "groups") return renderGroups(page);
  if (name === "suggestions") return renderSuggestions(page);
  if (name === "directory") return renderDirectory(page);
  if (name === "inspector") return renderInspector(page);
  if (name === "network") return renderNetwork(page);
  if (name === "performance") return renderPerformance(page);
  if (name === "data") return renderDataManagement(page);
}

document.querySelectorAll("[data-page]").forEach((link) =>
  link.addEventListener("click", () => document.querySelector("#sidebar").classList.remove("open")));
window.addEventListener("hashchange", () =>
  renderPage(location.hash.slice(1) || "dashboard"));
document.querySelector("#mobile-menu").addEventListener("click", () =>
  document.querySelector("#sidebar").classList.toggle("open"));
document.querySelector("#theme-toggle").addEventListener("click", () =>
  setTheme(document.documentElement.dataset.theme === "dark" ? "light" : "dark"));
document.querySelector("#top-theme-toggle").addEventListener("click", () =>
  setTheme(document.documentElement.dataset.theme === "dark" ? "light" : "dark"));

setTheme(localStorage.getItem("connectsphere-theme") || "light");
renderPage(location.hash.slice(1) || "dashboard");
