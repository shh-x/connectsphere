import { api, escapeHtml, initials, showLoading, showToast, userOptions } from "./api.js";

function heapTree(items) {
  if (!items.length) return '<div class="empty-state">The heap is empty for this user.</div>';
  const levels = [];
  items.forEach((item, index) => {
    const level = Math.floor(Math.log2(index + 1));
    if (!levels[level]) levels[level] = [];
    levels[level].push({ item, index });
  });
  return `<div class="heap-visual">${levels.map((level, depth) => `<div class="heap-level" style="--level:${depth}">${level.map(({ item, index }) => `<div class="heap-node"><strong>${item.score}</strong><small>#${item.id}</small><i>index ${index}</i></div>`).join("")}</div>`).join("")}</div>`;
}

export async function renderSuggestions(container) {
  container.innerHTML = `<div class="page-heading"><div><p class="eyebrow">Mutual connections</p><h1>Friend suggestions</h1><p class="page-subtitle">Explore friends-of-friends ranked with a C max-heap.</p></div><span class="tag purple">Heap priority queue</span></div>
    <div class="suggestion-layout"><section class="panel"><div class="panel-header"><div><h2>Find suggestions</h2><p class="subtle" style="margin:5px 0 0">Candidates are not already direct friends.</p></div></div><div class="panel-body">
      <form id="suggestion-form" class="suggestion-controls"><div class="form-field"><label for="suggest-user">Choose a person</label><select id="suggest-user" required><option value="">Loading users…</option></select></div><div class="form-field k-field"><label for="suggest-k">Top K</label><input id="suggest-k" type="number" min="1" max="100" value="5" required></div><button class="button" type="submit">Find people →</button></form>
      <div id="suggestion-results"><div class="empty-state">Choose a user to find people they may know.</div></div></div></section>
      <section class="panel"><div class="panel-header"><div><h2>Top K popular</h2><p class="subtle" style="margin:5px 0 0">Ranked by current friend count</p></div><label class="tag">Heap</label></div><div class="panel-body"><form id="popular-form" class="popular-controls"><label class="subtle" for="popular-k">Top K</label><input id="popular-k" type="number" min="1" max="100" value="5" required><button class="button small secondary" type="submit">Update</button></form><div id="popular-results"><div class="loading"><span class="spinner"></span>Ranking users…</div></div></div></section></div>
      <section class="panel heap-panel"><div class="panel-header"><div><h2>Suggestion heap</h2><p class="subtle" style="margin:5px 0 0">Array layout from the last C heap build · each node shows mutual-friend score</p></div><span class="tag purple">Max-heap</span></div><div class="panel-body" id="heap-results"><div class="empty-state">Run a suggestion query to inspect its priority queue.</div></div></section>`;
  try {
    const users = await api("/users");
    const select = container.querySelector("#suggest-user");
    if (!select) return;
    select.innerHTML = `<option value="">Select a user</option>${userOptions(users)}`;
    if (users.length) select.value = String(users[0].id);
  } catch (error) { showToast(error.message, true); }

  if (!container.querySelector("#popular-form")) return;
  const runPopular = async (event) => {
    if (event) event.preventDefault();
    const k = Number(container.querySelector("#popular-k").value);
    const target = container.querySelector("#popular-results");
    showLoading(target);
    try {
      const users = await api(`/popular?k=${k}`);
      target.innerHTML = users.length ? users.map((entry, index) => `<div class="popular-row"><span class="popular-rank">${String(index + 1).padStart(2, "0")}</span><div class="user-avatar">${escapeHtml(initials(entry.user.name))}</div><div class="popular-name"><b>${escapeHtml(entry.user.name)}</b><small>${entry.user.friendCount} friends</small></div><span class="popular-score">${entry.score}</span></div>`).join("") : '<div class="empty-state">No users available.</div>';
    } catch (error) {
      target.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  };
  container.querySelector("#popular-form").addEventListener("submit", runPopular);
  await runPopular();

  if (!container.querySelector("#suggestion-form")) return;
  container.querySelector("#suggestion-form").addEventListener("submit", async (event) => {
    event.preventDefault();
    const id = Number(container.querySelector("#suggest-user").value);
    const k = Number(container.querySelector("#suggest-k").value);
    const target = container.querySelector("#suggestion-results");
    showLoading(target);
    try {
      const suggestions = await api(`/users/${id}/suggestions?k=${k}`);
      if (!suggestions.length) {
        target.innerHTML = '<div class="empty-state"><div class="empty-icon">✳</div><strong>No suggestions yet</strong><p>This person has no eligible friends-of-friends in the current graph.</p></div>';
      } else {
        target.innerHTML = suggestions.map((suggestion, index) => `<article class="suggestion-card"><span class="suggestion-rank">${String(index + 1).padStart(2, "0")}</span><div class="user-avatar">${escapeHtml(initials(suggestion.user.name))}</div><div class="suggestion-info"><strong>${escapeHtml(suggestion.user.name)}</strong><small>${escapeHtml(suggestion.user.city)} · user #${suggestion.user.id}</small><div class="mutual-list">${suggestion.mutualFriends.map((name) => `<span class="tag">${escapeHtml(name)}</span>`).join("")}</div></div><div class="mutual-score"><b>${suggestion.score}</b><small>mutual ${suggestion.score === 1 ? "friend" : "friends"}</small></div></article>`).join("");
      }
      const heap = await api("/structures/heap");
      container.querySelector("#heap-results").innerHTML = heapTree(heap);
    } catch (error) {
      target.innerHTML = `<div class="empty-state">${escapeHtml(error.message)}</div>`;
      showToast(error.message, true);
    }
  });
}
